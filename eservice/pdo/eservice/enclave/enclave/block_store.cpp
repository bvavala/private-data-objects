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
#include <string.h>
#include "c11_support.h"
#include "hex_string.h"
#include <bits/stdc++.h>

namespace pdo {
    std::unordered_map<std::string, std::string> map;

    int BlockStoreGet(
        const uint8_t* key,
        const size_t keySize,
        uint8_t **value,
        size_t* valueSize
        )
    {
        std::string keyStr = BinaryToHexString(key, keySize);
        Log(PDO_LOG_DEBUG,
            "Get: '%s'",
            keyStr.c_str());

        if (map.find(keyStr) == map.end()) {
            Log(PDO_LOG_ERROR,
                "Failed to find key in map: '%s'",
                keyStr.c_str());
            *valueSize = 0;
            *value = NULL;
            // TODO: Appropriate error code
            return 1;
        } else {
            std::string valueStr = map[keyStr];
            Log(PDO_LOG_DEBUG,
                "Found key: '%s' -> '%s'",
                keyStr.c_str(),
                valueStr.c_str());

            /*
             * TODO - This leaks memory! There is nothing to clean up this
             * allocated data later
             */
            *valueSize = valueStr.size() / 2;
            *value = (uint8_t *)malloc(*valueSize);
            if (!*value) {
                Log(PDO_LOG_ERROR,
                    "Failed to allocate %zu bytes for get return value.",
                    *valueSize);
                *valueSize = 0;
                // TODO: Appropriate error code
                return 1;
            }

            // Deserialize the data from the cache into the buffer
            HexStringToBinary(*value, *valueSize, valueStr);

            return 0;
        }
    }

    int BlockStorePut(
        const uint8_t* key,
        const size_t keySize,
        const uint8_t* value,
        const size_t valueSize
        )
    {
        int result = 1;

        try{
            std::string keyStr = BinaryToHexString(key, keySize);
            std::string valueStr = BinaryToHexString(value, valueSize);

            Log(PDO_LOG_DEBUG,
                "Put: %zu bytes '%s' -> %zu bytes '%s'",
                keySize,
                keyStr.c_str(),
                valueSize,
                valueStr.c_str());

            map[keyStr] = valueStr;
            return 0;

        }catch(...){
            Log(PDO_LOG_ERROR,
                "Error: Failed to store to Block Store");
        }

        return result;

    }
} // namespace pdo
