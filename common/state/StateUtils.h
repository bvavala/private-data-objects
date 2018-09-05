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
#include "StateBlock.h"

using namespace pdo::state;

namespace pdo 
{
    namespace state 
    {
        class StateNode
        {
        protected:
            StateBlockId* blockId_;
            StateBlock* stateBlock_;
            StateBlockArray ChildrenArray_ = {};

        public:
            StateNode(StateBlockId& blockId, StateBlock& stateBlock);
            bool Valid();
            bool Valid(bool throwEx);
            void ReIdentify();
            StateBlockId& GetBlockId();
            StateBlock& GetBlock();
            
            void AppendChild(StateBlockId& blockId);
            void BlockifyChildren();
            void UnBlockifyChildren();
            StateBlockArray GetChildrenBlocks();
        };

        typedef StateNode State;

        typedef std::vector<StateNode> StateNodeArray;

        class StateExplorer
        {
        protected:
            StateNodeArray workingBranch;
        public:
            StateExplorer(State state);
        };
    }
}
