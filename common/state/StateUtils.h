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
