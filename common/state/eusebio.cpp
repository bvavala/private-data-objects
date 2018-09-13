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

#include "eusebio.h"
#include "crypto.h"
#include "state_status.h"
#include "types.h"

//#ifdef EUSEBIO_NO_DEFAULT
    #define EUSEBIO_ON_BIOX
//#else //EUSEBIO_NO_DEFAULT not defined
//    #define EUSEBIO_ON_BLOCK_STORE
//#endif //EUSEBIO_NO_DEFAULT

//########### internal eusebio context ####################
/*
The eusebio context describes operations to be performed on a block.
By default, the hash of block MUST match the block id used for fetching it.
Optionally, an encryption/decryption operation could be implemented,
when fetching or eviting a block.
*/
static eusebio_ctx_t eusebio_ctx = {NULL, 0, EUSEBIO_NO_CRYPTO};


state_status_t eusebio_set(eusebio_ctx_t ctx) {
    eusebio_ctx = ctx;
    return STATE_SUCCESS;
}

//############################ IMPLEMENTATION OF EUSEBIO ON BLOCK STORE ###########################
#ifdef EUSEBIO_ON_BLOCK_STORE
#include "enclave_t.h"
state_status_t eusebio_fetch(
    uint8_t* block_id,
    size_t block_id_size,
    eusebio_crypto_algo_e crypto_algo,
    uint8_t** block,
    size_t* block_size) {

    int ret;
    uint8_t *u_block; 
    size_t u_block_size;
    
    ocall_BlockStoreGet(&ret, block_id, block_id_size, &u_block, &u_block_size);
    ByteArray baBlockCopy(u_block, u_block + u_block_size);
    ByteArray baBlockCopyId = pdo::crypto::ComputeMessageHash(baBlockCopy);
    ByteArray baBlockId(block_id, block_id + block_id_size);
    if(baBlockId == baBlockCopyId) {
        return STATE_ERR_BLOCK_AUTHENTICATION;
    }

    if(crypto_algo != EUSEBIO_NO_CRYPTO) {
        return STATE_ERR_UNIMPLEMENTED;
    }

    *block = baBlockCopy.data();
    *block_size = baBlockCopy.size();    
    return STATE_SUCCESS;
}
state_status_t eusebio_evict(uint8_t* block, size_t block_size, eusebio_crypto_algo_e crypto_algo) {
    int ret;
    
    if(crypto_algo != EUSEBIO_NO_CRYPTO) {
        return STATE_ERR_UNIMPLEMENTED;
    }

    ByteArray baBlockCopy(block, block + block_size);
    ByteArray baBlockCopyId = pdo::crypto::ComputeMessageHash(baBlockCopy);
    ocall_BlockStorePut(&ret, baBlockCopyId.data(), baBlockCopyId.size(), 
        block, block_size);
    return STATE_SUCCESS;
}
#endif //EUSEBIO_ON_BLOCK_STORE
//#################################################################################################

//############################ IMPLEMENTATION OF EUSEBIO ON BIOX ##################################
#ifdef EUSEBIO_ON_BIOX
#include "blix.h"
#include "biox.h"
state_status_t eusebio_fetch(
    uint8_t* block_id, 
    size_t block_id_size, 
    eusebio_crypto_algo_e crypto_algo,
    uint8_t** block,
    size_t* block_size) {
    
    uint8_t* uas_block_address;
    uint8_t* tas_block_address;
    size_t uas_block_size;
    *block = NULL;

    uas_block_address = blix_wheretogetblock(block_id, block_id_size, &uas_block_size);
    if(uas_block_address == NULL) {
        return STATE_ERR_NOT_FOUND;
    }
    //allocate memory for the block in trusted address space
    tas_block_address = (uint8_t*) malloc(uas_block_size);
    if(!tas_block_address) {
        return STATE_ERR_MEMORY;
    }
    //load the data
    biox_in(tas_block_address, uas_block_size, uas_block_address, uas_block_size);
        
    //check block hash == block id
    ByteArray baBlockId(block_id, block_id + block_id_size);
    ByteArray baBlock(tas_block_address, tas_block_address + uas_block_size);
    ByteArray computedId = pdo::crypto::ComputeMessageHash(baBlock);
    if(baBlockId != computedId) {
        free(tas_block_address);
        return STATE_ERR_BLOCK_AUTHENTICATION;
    }
    
    //decrypt if necessary
    if(crypto_algo != EUSEBIO_NO_CRYPTO) {
        free(tas_block_address);
        return STATE_ERR_UNIMPLEMENTED;
    }

    *block = tas_block_address;
    *block_size = uas_block_size;
    return STATE_SUCCESS;
}

state_status_t eusebio_evict(uint8_t* block, size_t block_size, eusebio_crypto_algo_e crypto_algo) {
    uint8_t* uas_block_address;

    if(crypto_algo != EUSEBIO_NO_CRYPTO) {
        return STATE_ERR_UNIMPLEMENTED;
    }

    //find where to put the block
    uas_block_address = blix_wheretoputblock(block_size);
    if(uas_block_address == NULL) {
        return STATE_ERR_NOT_FOUND;
    }
    //output the block
    biox_out(uas_block_address, block, block_size);
    //tell the layers below that the block has been written
    biox_sync(); //tell untrusted space we are done, and it can compute hash
    return STATE_SUCCESS;
}

#endif //EUSEBIO_ON_BIOX
//#################################################################################################
