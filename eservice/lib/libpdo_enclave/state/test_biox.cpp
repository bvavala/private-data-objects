#include "enclave_t.h"
#include "biox.h"
#include "pdo_error.h"
#include "enclave_utils.h"
#include "state_status.h"
#include "blix.h"

uint8_t* __create_block(size_t block_size) {
    static char block_index = '0';
    unsigned int i;
    uint8_t* block = (uint8_t*) malloc(block_size);
    if(block == NULL)
        return NULL;
    for(i=0; i<block_size; i++)
        block[i] = (uint8_t)block_index;
    block_index+=1;
    return block;
}

void test_biox() {
    SAFE_LOG(PDO_LOG_INFO, "******************** test_biox *******************\n");
    unsigned int i;
    size_t BLOCK_SIZE = 4096;
    unsigned int BLOCK_NUM = 10;
    int r;
    for(i=0; i<BLOCK_NUM; i++) {
        uint8_t* block_address_destination;
        uint8_t* block = __create_block(BLOCK_SIZE);
        if(!block) {
            SAFE_LOG(PDO_LOG_INFO, "out of memory\n"); 
            break;           
        }
        block_address_destination = blix_wheretoputblock(BLOCK_SIZE);
        r = biox_out(block_address_destination, block, BLOCK_SIZE);
        if(r==STATE_SUCCESS) {
            SAFE_LOG(PDO_LOG_INFO, "block %d biox_out success\n");
        }
        else {
             SAFE_LOG(PDO_LOG_INFO, "block %d biox_out error\n");
        }
    }
    
    SAFE_LOG(PDO_LOG_INFO, "syncing blocks\n");
    biox_sync();
    SAFE_LOG(PDO_LOG_INFO, "*************** end of test_biox *****************\n");
}
    

