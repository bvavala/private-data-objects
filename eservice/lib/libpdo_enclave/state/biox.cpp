#include "state_status.h"
#include "enclave_t.h"
#include <string>
#include <vector>
#include "crypto.h"
#include "zero.h"

/*
BIOX is the Block I/O interface

Notation:
    uas: untrusted address space
    tas: trusted address space
*/

int biox_in(uint8_t* tas_destination,
            size_t tas_destination_size,
            uint8_t* uas_source,
            size_t uas_source_size) {
    if(uas_source_size > tas_destination_size)
        return STATE_ERR_OVERFLOW;
   
    int r; 
    r = memcpy_s(tas_destination, tas_destination_size, uas_source, uas_source_size);
    if(r!=0)
        return STATE_ERR_UNKNOWN;

    return STATE_SUCCESS;       
}

int biox_out(uint8_t* uas_destination,
             uint8_t* tas_source,
             size_t tas_source_size) {
    //ASSUMPTION: enough memory has been allocated at destination
    int r;
    size_t uas_destination_size = tas_source_size;
    r = memcpy_s(uas_destination, uas_destination_size, tas_source, tas_source_size);
    if(r!=0)
        return STATE_ERR_UNKNOWN;

    return STATE_SUCCESS;    
}

void biox_sync() {
    test_avalanche_sync();
}

