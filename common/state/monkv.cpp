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

#include "monkv.h"
#include "keivers.h"
#include "sal.h"

pdo::state::Monkv::Monkv(ByteArray& id) : Keivers(id) {
    g_sal.open(id, &handle); 
}

pdo::state::Monkv::~Monkv() {
    ByteArray id;
    if(handle) {
        g_sal.close(&handle, &id);
    }
}

void pdo::state::Monkv::Uninit(ByteArray& id) {
    if(handle) {
        g_sal.close(&handle, &id);
    }
}

ByteArray pdo::state::Monkv::Get(ByteArray& key) {
    g_sal.seek(handle, INT64_MIN);
    ByteArray ks, vs, k, v, none;
    while(1) {
        if(STATE_EOD == g_sal.read(handle, sizeof(size_t), ks))
            break;
        g_sal.read(handle, sizeof(size_t), vs);
        size_t ksize = *((size_t*)ks.data());
        size_t vsize = *((size_t*)vs.data());
        g_sal.read(handle, ksize, k);
        g_sal.read(handle, vsize, v);
        if(k == key) {            
            return v;    
        }
        ks.clear();
        vs.clear();
        k.clear();
        v.clear();
    }
    return none;
}

void pdo::state::Monkv::Put(ByteArray& key, ByteArray& value) {
    g_sal.seek(handle, INT64_MAX);
    size_t key_size = key.size();
    size_t value_size = value.size();
    ByteArray baks((uint8_t*)&key_size, (uint8_t*)&key_size+sizeof(key_size));
    ByteArray bavs((uint8_t*)&value_size, (uint8_t*)&value_size+sizeof(value_size));
    g_sal.write(handle, baks);
    g_sal.write(handle, bavs);
    g_sal.write(handle, key);
    g_sal.write(handle, value);    
}

