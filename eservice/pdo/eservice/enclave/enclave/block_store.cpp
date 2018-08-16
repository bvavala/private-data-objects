/* Copyright 2018 Intel CorporationOEU
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

#include "types.h"
#include "log.h"
#include <stdarg.h>
#include <stdio.h>
#include "c11_support.h"
#include "hex_string.h"

namespace pdo {

    int BlockStoreGet(
        const uint8_t* key,
        const size_t keySize,
        uint8_t **value,
        size_t* valueSize
        )
    {
        ByteArray keyArray(key, key + keySize);
        printf("Get: '%s'\n", BinaryToHexString(keyArray));
    }

    int BlockStorePut(
        const uint8_t* key,
        const size_t keySize,
        const uint8_t* value,
        const size_t valueSize
        )
    {
        ByteArray keyArray(key, key + keySize);
        ByteArray valueArray(value, value + valueSize);
        printf("Put: '%s' : '%s'\n", BinaryToHexString(keyArray), BinaryToHexString(valueArray));
    }
} // namespace pdo
