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

//HexEncodedString __get_printable_hex_string(uint8_t* buffer, size_t buffer_size) {
//    ByteArray baBuffer;
//    baBuffer.assign(
//        (char*)buffer,
//        (char*)buffer + buffer_size);
//    return ByteArrayToHexEncodedString(baBuffer);
//}

//########################################################
//########################################################
//########################################################

extern "C" {

void test_avalanche_wheretoget(
    const uint8_t* block_authentication_id,
    size_t block_authentication_id_size,
    uint8_t** address) {
    if(!filled_spots)
        __init();
    
    *address = NULL;
    //go through all blocks searching for the block authentication id
    void* block_at = NULL;
    int i;
    for(i=0; i<AVALANCHE_CACHE_SIZE; i++) {
        if(avalanche_cache[i].unavailable &&
            avalanche_cache[i].block &&
            block_authentication_id_size == avalanche_cache[i].key_size &&
            0 == memcmp(avalanche_cache[i].key, block_authentication_id, block_authentication_id_size)) {
            block_at = (void*)avalanche_cache[i].block;
            
            //string block_key_hex = pdo::BinaryToHexString(avalanche_cache[i].block, avalanche_cache[i].block_size);
            pdo::Log(PDO_LOG_INFO, "Avalanche_Get: %s", pdo::BinaryToHexString(avalanche_cache[i].block, avalanche_cache[i].block_size).c_str());

            break;
        }
    }

    //if not found, try opening the respective file, cache it
    
    //return cached block address
    *address = (uint8_t*)block_at;
}


void test_avalanche_wheretoput(size_t block_size, uint8_t** address) {
    if(!filled_spots)
        __init();
    *address = NULL;
    //find first avialable cache entry
    int i;
    for(i=0; i<AVALANCHE_CACHE_SIZE; i++) {
        if(!avalanche_cache[i].unavailable)
            break;
    }
    if(i==AVALANCHE_CACHE_SIZE)
        return;

    //allocate block_size memory for the block
    avalanche_cache[i].block = (uint8_t*)malloc(block_size);
    if(!avalanche_cache[i].block)
        return;
    //zero the block
    memset(avalanche_cache[i].block, '\0', block_size);
    avalanche_cache[i].block_size = block_size;

    avalanche_cache[i].unavailable = 1;
    filled_spots ++;

    pdo::Log(PDO_LOG_INFO, "Avalanche_Put: %d bytes", block_size);

    //return address to block
    *address = (uint8_t*)avalanche_cache[i].block;
}

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
            avalanche_cache[i].key = block_hash.data();
            avalanche_cache[i].key_size = block_hash.size();
            
            pdo::Log(PDO_LOG_INFO, "Avalanche_sync: %s", ByteArrayToHexEncodedString(block_hash).c_str());
        }
    }
    //write all cached blocks on disk
        //write 4 byte size first
        //then write block
}

}
