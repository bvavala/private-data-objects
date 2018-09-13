/* Copyright 2018 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "sal.h"
#include "types.h"
#include "state.h"
#include "eusebio.h"
#include "state_status.h"
#include "pdo_error.h"
#include "enclave_utils.h"

#include "StateUtils.h"

/* 
Global variable for the State Abstraction layer.
This is used by any code developed above the abstraction to access
(i.e., open, close, read, write) named content (i.e., and id is required).
*/
pdo::state::sal g_sal;

//##################### INTERNAL CLASS
/*
The sal_handle is an internal data structure used to maintain information about
the currently accessed state, e.g., 
*/
class sal_handle {
public:
    uint64_t cursor;
    ByteArray* block;
    StateNode* node;
    sal_handle(StateNode& stateNode) {
        cursor = 0;
        node = &stateNode;
        block = &(node->GetBlock());
    }
};

//###################### STATE ABSTRACTION LAYER
/*
The SAL is initialized using the state root (for extrinsic state) in the contract state.
Then, other code that uses the SAL (e.g., a key value store) is assumed to have the id
(i.e., sha256 hash) of the data it wants to open and read/write.
*/
pdo::state::sal::sal() {
    initialized_ = false;
    rootNode = NULL;
}

void pdo::state::sal::init(ByteArray& rootId) {
    if(rootId.empty()) { //no id, create state root
        rootNode = new pdo::state::StateNode(*new StateBlockId(), *new StateBlock());
        //initialize block with 0 children (as if we just retrieved it)
        rootNode->BlockifyChildren();
    }
    else { //retrieve main state block
        uint8_t* block;
        size_t block_size;
        state_status_t ret;
        SAFE_LOG(PDO_LOG_INFO, "SAL init: root id: %s\n", ByteArrayToHexEncodedString(rootId).c_str());
        ret = eusebio_fetch(rootId.data(), rootId.size(), EUSEBIO_NO_CRYPTO, &block, &block_size);
        pdo::error::ThrowIf<pdo::error::ValueError>(
            ret != STATE_SUCCESS, "eusebio returned an error");
        rootNode = new pdo::state::StateNode(*new StateBlockId(rootId), *new StateBlock(block, block + block_size));
        free(block); //allocated by eusebio
    }

    rootNode->UnBlockifyChildren();
    initialized_ = true;
}

/*
The SAL uninitialization returns a copy of the identity (i.e., root hash).
This is the root of the extrinsic state.
*/
state_status_t pdo::state::sal::uninit(StateBlockId* rootId) {
    rootNode->BlockifyChildren();
    rootNode->ReIdentify();
    StateBlock b = rootNode->GetBlock();
    state_status_t ret = eusebio_evict(b.data(), b.size(), EUSEBIO_NO_CRYPTO);
    pdo::error::ThrowIf<pdo::error::ValueError>(
            ret != STATE_SUCCESS, "eusebio returned an error");
    *rootId = rootNode->GetBlockId();
    SAFE_LOG(PDO_LOG_INFO, "SAL uninit: root id: %s\n", ByteArrayToHexEncodedString(*rootId).c_str());
    delete rootNode;
    initialized_ = false;
    return STATE_SUCCESS;
}

/*
The open function takes as input an id.
If the id is empty, a new child is created (e.g., the root hash of a KV store that has just been created).
If the id is not empty, then this should match a child of the current node in the hierarchy.
In case of success, a handle is returned, and can be used for reading and writing data.
*/
state_status_t pdo::state::sal::open(ByteArray& id, void **handle) {
    *handle = NULL;
    StateNodeRef nodeRef;

    if(id.empty()) { //empty id -> create it
        nodeRef = new StateNode(*new StateBlockId(), *new StateBlock());
        rootNode->AppendChild(*nodeRef);
    }
    else { //non-empty id -> find it
        StateBlockIdRef childIdRef = rootNode->LookupChild(id);
        if(!childIdRef) { //error, no child found            
            return STATE_ERR_NOT_FOUND;
        }
        //it's a child, so retrieve block
        uint8_t* block;
        size_t block_size;
        state_status_t ret;
        ret = eusebio_fetch(id.data(), id.size(), EUSEBIO_NO_CRYPTO, &block, &block_size);
        pdo::error::ThrowIf<pdo::error::ValueError>(
            ret != STATE_SUCCESS, "eusebio returned an error");
        pdo::error::ThrowIfNull(block, "eusebio returned null block");
        nodeRef = new StateNode(*childIdRef, *new StateBlock(block, block + block_size));
        nodeRef->SetHasParent();
        free(block);//free buffer from eusebio                
    }
    
    *handle = new sal_handle(*nodeRef);
    return STATE_SUCCESS;
}

/*
The read operation appends up to 'bytes' of data into the output buffer, starting from the current cursor.
if the read operation overflows, it still appends as many bytes as possible and returns an EOD (End Of Data) value.
*/
state_status_t pdo::state::sal::read(void* handle, size_t bytes, ByteArray &output_buffer) {
    sal_handle* h = (sal_handle*)handle;
    state_status_t ret;
    uint64_t new_cursor = h->cursor + bytes;
    StateBlock::const_iterator first = h->block->begin() + h->cursor;
    StateBlock::const_iterator last;
    StateBlock::const_iterator expected_last = h->block->begin() + new_cursor;
    if(expected_last > h->block->end()) {
        last = h->block->end();
        h->cursor = h->block->size();
        ret = STATE_EOD;
    }
    else {
        last = expected_last;
        h->cursor = new_cursor;
        ret = STATE_SUCCESS;
    }
    output_buffer.insert(output_buffer.end(), first, last);
    return ret;
}

/*
The write operation takes an input buffer an writes the data in the opened state, starting from the current cursor.
*/
state_status_t pdo::state::sal::write(void* handle, ByteArray &input_buffer) {
    sal_handle* h = (sal_handle*)handle;
    h->block->insert(h->block->begin() + h->cursor, input_buffer.begin(), input_buffer.end());
    h->cursor += input_buffer.size();
    return STATE_SUCCESS;
}

/*
The seek operation moves the cursor backward or forward of a specified offset.
The highest negative (resp. positive) offset is interpreted as the first (resp. last) byte of the state.
*/
state_status_t pdo::state::sal::seek(void* handle, int64_t offset) {
    sal_handle* h = (sal_handle*)handle;

    switch(offset) {
        case INT64_MIN:
            h->cursor = 0; // beginning
            break;
        case INT64_MAX:
            h->cursor = h->block->size();
            break;
        default:
            if(h->cursor + offset <= h->block->size()) {
                h->cursor += offset;
                break;
            }
            else {
                return STATE_ERR_OVERFLOW;
            }
    }
    return STATE_SUCCESS;
}

/*
The close operation frees resources, evicts the blocks that are now unnecessary and returns
*/
state_status_t pdo::state::sal::close(void **handle, StateBlockId* id) {
    sal_handle* h = *(sal_handle**)handle;
    state_status_t ret;

    //get root identity to be returned
    h->node->ReIdentify();
    *id = h->node->GetBlockId();
    //evict unnecessary block
    
    StateBlock& b = h->node->GetBlock();
    ret = eusebio_evict(b.data(), b.size(), EUSEBIO_NO_CRYPTO);
    pdo::error::ThrowIf<pdo::error::ValueError>(
            ret != STATE_SUCCESS, "eusebio returned an error");
    //free resources
    delete h->node;
    delete h;
    *handle = NULL;
    return STATE_SUCCESS;
}
