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

#include "interpreter_kv.h"
#include "state.h"


pdo::state::Interpreter_KV::Interpreter_KV(ByteArray& id) : Basic_KV_Plus(id) {
}

pdo::state::Interpreter_KV::Interpreter_KV(ByteArray& id, const ByteArray& encryption_key) : Interpreter_KV(id) {
    ByteArray key = encryption_key;
    State_KV* state_kv = new pdo::state::State_KV(id, key);
    kv_ = state_kv;
}

pdo::state::Interpreter_KV::~Interpreter_KV() {
    if(kv_ != NULL) {
        delete kv_;
        kv_ = NULL;
    }
}

void pdo::state::Interpreter_KV::Uninit(ByteArray& id) {
    kv_->Uninit(id);
}

ByteArray pdo::state::Interpreter_KV::Get(ByteArray& key) {
    return kv_->Get(key);
}

void pdo::state::Interpreter_KV::Put(ByteArray& key, ByteArray& value) {
    kv_->Put(key, value);
}

void pdo::state::Interpreter_KV::Delete(ByteArray& key) {
    kv_->Delete(key);
}

ByteArray to_privileged_key(ByteArray& key) {
    uint8_t access_right = 'P';
    ByteArray privileged_key = key;
    privileged_key.insert(privileged_key.begin(), access_right);
    return privileged_key;
}

ByteArray to_unprivileged_key(ByteArray& key) {
    uint8_t access_right = 'p';
    ByteArray unprivileged_key = key;
    unprivileged_key.insert(unprivileged_key.begin(), access_right);
    return unprivileged_key;
}

ByteArray pdo::state::Interpreter_KV::PrivilegedGet(ByteArray& key) {
    ByteArray privileged_key = to_privileged_key(key);
    return kv_->Get(privileged_key);
}

void pdo::state::Interpreter_KV::PrivilegedPut(ByteArray& key, ByteArray& value) {
    ByteArray privileged_key = to_privileged_key(key);
    kv_->Put(privileged_key, value);
}

ByteArray pdo::state::Interpreter_KV::UnprivilegedGet(ByteArray& key) {
    ByteArray unprivileged_key = to_unprivileged_key(key);
    return kv_->Get(unprivileged_key);
}

void pdo::state::Interpreter_KV::UnprivilegedPut(ByteArray& key, ByteArray& value) {
    ByteArray unprivileged_key = to_unprivileged_key(key);
    kv_->Put(unprivileged_key, value);
}
