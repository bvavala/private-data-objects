#include "StateUtils.h"
#include "StateBlock.h"
#include "crypto.h"
#include "error.h"

pdo::state::StateNode::StateNode(StateBlockId& blockId, StateBlock& stateBlock) {
    blockId_ = &blockId;
    stateBlock_ = &stateBlock;
}

bool pdo::state::StateNode::Valid() {
    StateBlockId computedBlockId = pdo::crypto::ComputeMessageHash(*stateBlock_);
    if(computedBlockId == *blockId_) {
        return true;
    }
    return false;
}

bool pdo::state::StateNode::Valid(bool throwEx) {
    pdo::error::ThrowIf<pdo::error::ValueError>(
        Valid() != true,
        "state node not valid");
    //else
    return true;    
}

void pdo::state::StateNode::ReIdentify() {
    *blockId_ = pdo::crypto::ComputeMessageHash(*stateBlock_);
}

StateBlockId& pdo::state::StateNode::GetBlockId() {
    return *blockId_;
}

StateBlock& pdo::state::StateNode::GetBlock() {
    return *stateBlock_;
}

void pdo::state::StateNode::AppendChild(StateBlockId& blockId) {
    ChildrenArray_.push_back(blockId);
}

void pdo::state::StateNode::BlockifyChildren() {
    //rebuild block
    stateBlock_->clear();
    //put children num first
    if(ChildrenArray_.size() > UINT8_MAX) {
        std::string msg("Too many children in state node");
        throw pdo::error::ValueError(msg);
    }
    uint8_t childrenNum = ChildrenArray_.size();
    stateBlock_->push_back(childrenNum);    
    //put children
    while(!ChildrenArray_.empty()) {
        //append first child to block
        stateBlock_->insert(stateBlock_->end(), ChildrenArray_[0].begin(), ChildrenArray_[0].end());
        //remove first child from array
        ChildrenArray_.erase(ChildrenArray_.begin());
    }
}

void pdo::state::StateNode::UnBlockifyChildren() {
    //check that's not empty
    if(stateBlock_->empty()) {
        std::string msg("Can't unblockify state node, block is empty");
        throw pdo::error::ValueError(msg);           
    }
    //check that there are the expected number of children/bytes
    uint8_t childrenNum = (*stateBlock_)[0];
    size_t expectedSize = 1+(childrenNum * SHA256_DIGEST_LENGTH);
    if(expectedSize != stateBlock_->size()) {
        std::string msg("Can't unblockify state node, children bytes do not match");
        throw pdo::error::ValueError(msg);
    }
    //remove children num
    stateBlock_->erase(stateBlock_->begin());    
    //get the children
    ChildrenArray_.clear();
    while(!stateBlock_->empty()) {
        StateBlockId childId(stateBlock_->begin(), stateBlock_->begin() + SHA256_DIGEST_LENGTH);
        ChildrenArray_.push_back(childId);
        stateBlock_->erase(stateBlock_->begin(), stateBlock_->begin() + SHA256_DIGEST_LENGTH);
    }
}

StateBlockArray pdo::state::StateNode::GetChildrenBlocks() {
    return ChildrenArray_;
}
