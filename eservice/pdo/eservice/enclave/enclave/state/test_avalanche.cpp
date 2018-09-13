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

#include "c11_support.h"
#include <string>
#include <string.h>
#include "crypto.h"
#include "hex_string.h"
#include "log.h"

typedef struct {
    int unavailable;
    uint8_t* key;
    size_t key_size;
    uint8_t* block;
    size_t block_size;
} cached_block_t;

#define AVALANCHE_CACHE_SIZE 1024
cached_block_t avalanche_cache[AVALANCHE_CACHE_SIZE];
unsigned int filled_spots = 0;

//########################################################
// internal functions ####################################
//########################################################

void __init() {
    memset(avalanche_cache, '\0', sizeof(avalanche_cache));
}

//########################################################
//########################################################
//########################################################

extern "C" {

/*
The wheretoget operation returns the address (and size) in untrusted address space of the buffer
that contains the requested block.
*/
void test_avalanche_wheretoget(
    const uint8_t* block_authentication_id,
    size_t block_authentication_id_size,
    uint8_t** address,
    size_t* block_size) {
    if(!filled_spots)
        __init();

    pdo::Log(PDO_LOG_DEBUG, "Avalanche_Get find %s\n",
        pdo::BinaryToHexString(block_authentication_id, block_authentication_id_size).c_str());
    
    *address = NULL;
    //go through all blocks searching for the block authentication id
    void* block_at = NULL;
    size_t bsize;
    int i;
    for(i=0; i<AVALANCHE_CACHE_SIZE; i++) {
        if(avalanche_cache[i].unavailable &&
            avalanche_cache[i].block &&
            block_authentication_id_size == avalanche_cache[i].key_size &&
            0 == memcmp(avalanche_cache[i].key, block_authentication_id, block_authentication_id_size)) {
            block_at = (void*)avalanche_cache[i].block;
            bsize = avalanche_cache[i].block_size;
            //string block_key_hex = pdo::BinaryToHexString(avalanche_cache[i].block, avalanche_cache[i].block_size);
            pdo::Log(PDO_LOG_DEBUG, "Avalanche_Get: index %d size %lu\n", i, avalanche_cache[i].block_size);
            break;
        }
    }

    //return cached block address and size
    *address = (uint8_t*)block_at;
    *block_size = bsize;
}

/*
The wheretoput operation return an address in untrusted address space where the data block is expected to be copied.
The code is not aware of when such copy is done and terminates, it just makes enough memory available.
A separate sync call is used to signal the completion of the write operation.
*/
void test_avalanche_wheretoput(size_t block_size, uint8_t** address) {
    if(!filled_spots)
        __init();
    *address = NULL;
    if(filled_spots == AVALANCHE_CACHE_SIZE) {
        pdo::Log(PDO_LOG_ERROR, "Cache is full\n");
        return;
    }
    //find first avialable cache entry
    int i;
    for(i=0; i<AVALANCHE_CACHE_SIZE; i++) {
        if(!avalanche_cache[i].unavailable)
            break;
    }
    //allocate block_size memory for the block
    avalanche_cache[i].block = (uint8_t*)malloc(block_size);
    if(!avalanche_cache[i].block)
        return;
    avalanche_cache[i].block_size = block_size;
    avalanche_cache[i].unavailable = 1;
    filled_spots ++;
    //zero the block
    memset(avalanche_cache[i].block, '\0', block_size);
    pdo::Log(PDO_LOG_DEBUG, "Avalanche_Put: %d bytes allocated at %x\n",
        avalanche_cache[i].block_size, avalanche_cache[i].block);

    //return address to block
    *address = (uint8_t*)avalanche_cache[i].block;
}

/*
The sync operation is essentially used to signal that the data has been copied in untrusted address space.
*/
void test_avalanche_sync() {
    //update all cached blocks with zero bytes key, by computing the key
    int i;
    for(i=0; i<AVALANCHE_CACHE_SIZE; i++) {
        if(avalanche_cache[i].unavailable && avalanche_cache[i].key_size == 0) {
            ByteArray block;
            block.assign(
                (char*)avalanche_cache[i].block,
                (char*)avalanche_cache[i].block + avalanche_cache[i].block_size);
            ByteArray block_hash = pdo::crypto::ComputeMessageHash(block);
            avalanche_cache[i].key = (uint8_t*)malloc(block_hash.size());
            memcpy(avalanche_cache[i].key, block_hash.data(), block_hash.size());
            avalanche_cache[i].key_size = block_hash.size(); 
            pdo::Log(PDO_LOG_DEBUG, "Avalanche_sync (block size %lu): %s\n",
                avalanche_cache[i].block_size, ByteArrayToHexEncodedString(block_hash).c_str());
        }
    }
}

} // extern "C"
