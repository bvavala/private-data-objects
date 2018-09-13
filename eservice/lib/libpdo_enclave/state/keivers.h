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

namespace pdo
{
    namespace state
    {
        class Keivers 
        {
        protected:
            void *handle;
        public:
            Keivers(ByteArray& id) {}
            virtual void Uninit(ByteArray& id) = 0;
            virtual ByteArray Get(ByteArray& key) = 0;
            virtual void Put(ByteArray& key, ByteArray& value) = 0;
        };
    }
}
