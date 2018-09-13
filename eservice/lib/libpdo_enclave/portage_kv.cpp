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

#include "portage_kv.h"
#include "keivers.h"

//TODO move the definition somewhere else
#define PORTAGE_ON_MONKV

#ifdef PORTAGE_ON_MONKV
#include "monkv.h"
#endif

pdo::state::Portage::Portage(ByteArray& id) : Keivers(id) {
#ifdef PORTAGE_ON_MONKV
    Monkv* monkv = new pdo::state::Monkv(id);
    kv_ = monkv;
#endif
}

ByteArray pdo::state::Portage::Get(ByteArray& key) {
    return kv_->Get(key);
}

void pdo::state::Portage::Put(ByteArray& key, ByteArray& value) {
    kv_->Put(key, value);
}
