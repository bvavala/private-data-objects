#pragma once

#include "types.h"

int biox_in(uint8_t* tas_destination,
            size_t tas_destination_size,
            uint8_t* uas_source,
            size_t uas_source_size);
int biox_out(uint8_t* uas_destination,
             uint8_t* tas_source,
             size_t tas_source_size);
void biox_sync();

