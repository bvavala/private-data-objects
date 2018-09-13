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

#pragma once

#include "types.h"
#include "state_status.h"
#include "StateBlock.h"
#include "StateUtils.h"

namespace pdo 
{
    namespace state 
    {
        class sal {
        public:
            sal();
            void init(ByteArray &id);
            state_status_t uninit(StateBlockId* rootId);
            state_status_t open(ByteArray& id, void **handle);

            state_status_t read(void* handle, size_t bytes, ByteArray &output_buffer);
            state_status_t write(void* handle, ByteArray &input_buffer);
            state_status_t seek(void* handle, int64_t offset);
            state_status_t close(void **handle, StateBlockId* id);

        protected:        
            bool initialized_ = false;
            StateNode* rootNode;
        };
    }
}

extern pdo::state::sal g_sal;
