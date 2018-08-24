#include "enclave_t.h"

/*
BLIX is the Block locator interface

Notation:
    uas: untrusted address space
    tas: trusted address space
*/
uint8_t* blix_wheretogetblock(uint8_t* block_authentication_id, size_t block_authentication_id_size) {
    uint8_t* address = NULL;
    test_avalanche_wheretoget(block_authentication_id, block_authentication_id_size, &address);        
    return address;
}

uint8_t* blix_wheretoputblock(size_t block_size) {
    uint8_t* address = NULL;
    test_avalanche_wheretoput(block_size, &address);
    return address;
}
