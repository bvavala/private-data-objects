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

#include "enclave_t.h"
#include "enclave_utils.h"
/*
BLIX is the Block locator interface

Notation:
    uas: untrusted address space
    tas: trusted address space
*/
uint8_t* blix_wheretogetblock(uint8_t* block_authentication_id, size_t block_authentication_id_size, size_t* block_size) {
    uint8_t* address = NULL;
    test_avalanche_wheretoget(block_authentication_id, block_authentication_id_size, &address, block_size);
    return address;
}

uint8_t* blix_wheretoputblock(size_t block_size) {
    uint8_t* address = NULL;
    test_avalanche_wheretoput(block_size, &address);
    return address;
}
