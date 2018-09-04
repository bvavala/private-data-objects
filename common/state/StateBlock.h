#pragma once

#include <openssl/sha.h>
#include "types.h"

#define STATE_BLOCK_ID_LENGTH SHA256_DIGEST_LENGTH

namespace pdo {
    namespace state {
        typedef ByteArray StateBlock;
        typedef ByteArray StateBlockId;
        typedef std::vector<StateBlockId> StateBlockArray;
    }
}
