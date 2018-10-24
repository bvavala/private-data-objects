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

#include "sebio.h"
#include "crypto.h"
#include "state.h"
#include "types.h"
#include "crypto.h"
#include "c11_support.h"
#include "error.h"

#include "packages/block_store/block_store.h"

#ifdef DEBUG
    #define SAFE_LOG(LEVEL, FMT, ...) Log(LEVEL, FMT, ##__VA_ARGS__)
#else // DEBUG not defined
    #define SAFE_LOG(LEVEL, FMT, ...)
#endif // DEBUG

state_status_t sebio_fetch_from_block_store(
    uint8_t* block_id,
    size_t block_id_size,
    sebio_crypto_algo_e crypto_algo,
    uint8_t** block,
    size_t* block_size);
state_status_t sebio_evict_to_block_store(
    uint8_t* block,
    size_t block_size,
    sebio_crypto_algo_e crypto_algo,
    ByteArray& idOnEviction);


//########### internal sebio (Secure Block IO) context ####################
/*
    The sebio context describes operations to be performed on a block.
    By default, the hash of block MUST match the block id used for fetching it.
    Optionally, an encryption/decryption operation could be implemented,
    when fetching or eviting a block.
*/
static sebio_ctx_t sebio_ctx = {
    {},
    SEBIO_NO_CRYPTO,
    &sebio_fetch_from_block_store,
    &sebio_evict_to_block_store
};
//########################################################################

/*
    Set the context for the secure block IO
*/
state_status_t sebio_set(sebio_ctx_t ctx) {
    sebio_ctx = ctx;
    if(sebio_ctx.f_sebio_fetch == NULL || sebio_ctx.f_sebio_evict == NULL) {
        sebio_ctx.f_sebio_fetch = &sebio_fetch_from_block_store;
        sebio_ctx.f_sebio_evict = sebio_evict_to_block_store;
    }
    return STATE_SUCCESS;
}

state_status_t sebio_fetch(
    uint8_t* block_id,
    size_t block_id_size,
    sebio_crypto_algo_e crypto_algo,
    uint8_t** block,
    size_t* block_size)
{
    return sebio_ctx.f_sebio_fetch(block_id, block_id_size, crypto_algo, block, block_size);
}

state_status_t sebio_evict(
    uint8_t* block,
    size_t block_size,
    sebio_crypto_algo_e crypto_algo,
    ByteArray& idOnEviction)
{
    return sebio_ctx.f_sebio_evict(block, block_size, crypto_algo, idOnEviction);
}

/*
    The fetch function gets a block from the block store.
    It requests first the size of a block,
    then it allocates memory to contain it and loads it,
    then it hashes the block and checks that it matches the id/hash given by the caller,
    finally it decrypts the block if the caller specified that and set a context.

    CONVENTION: the memory allocated for the block MUST be freed by the caller.
*/
state_status_t sebio_fetch_from_block_store(
    uint8_t* block_id,
    size_t block_id_size,
    sebio_crypto_algo_e crypto_algo,
    uint8_t** block,
    size_t* block_size) {

    ByteArray baBlockId(block_id, block_id + block_id_size);
    ByteArray baBlock;
    uint8_t* tas_block_address;
    size_t tas_block_size;
    *block = NULL;

    //load the data
    pdo_err_t ret;
    ret = pdo::block_store::BlockStoreGet(baBlockId, baBlock);
    if(ret != PDO_SUCCESS) {
        SAFE_LOG(PDO_LOG_ERROR, "sebio error, BlockStoreGet returned %d\n", ret);
        return STATE_ERR_NOT_FOUND;
    }

    //check block hash == block id
    ByteArray computedId = pdo::crypto::ComputeMessageHash(baBlock);
    if(baBlockId != computedId) {
        return STATE_ERR_BLOCK_AUTHENTICATION;
    }

    //decrypt if necessary
    switch(crypto_algo) {
        case SEBIO_NO_CRYPTO: {
            tas_block_size = baBlock.size();
            tas_block_address = (uint8_t*) malloc(tas_block_size);
            if(!tas_block_address) {
                SAFE_LOG(PDO_LOG_DEBUG, "sebio error, out of memory");
                return STATE_ERR_MEMORY;
            }
            memcpy_s(tas_block_address, tas_block_size, baBlock.data(), baBlock.size());
            break;
        }
        case SEBIO_AES_GCM: {
            pdo::error::ThrowIf<pdo::error::RuntimeError>(
                    sebio_ctx.crypto_algo != crypto_algo, "sebio_fetch, crypto-algo does not match");
            ByteArray decryptedState;
            decryptedState = pdo::crypto::skenc::DecryptMessage(sebio_ctx.key, baBlock);
            tas_block_size = decryptedState.size();
            tas_block_address = (uint8_t*) malloc(tas_block_size);
            if(!tas_block_address) {
                SAFE_LOG(PDO_LOG_DEBUG, "sebio error, out of memory");
                return STATE_ERR_MEMORY;
            }
            memcpy_s(tas_block_address, tas_block_size, decryptedState.data(), decryptedState.size());
            break;
        }
        default:
            return STATE_ERR_UNIMPLEMENTED;
    }

    *block = tas_block_address;
    *block_size = tas_block_size;
    return STATE_SUCCESS;
}

/*
    The evict function puts a block into the block store.
    If the caller specifies an encryption algorithm and a context has been set,
    the block is first encrypted and then sent to the block store.
*/
state_status_t sebio_evict_to_block_store(
    uint8_t* block,
    size_t block_size,
    sebio_crypto_algo_e crypto_algo,
    ByteArray& idOnEviction) {

    ByteArray baBlockCopy(block, block + block_size);
    ByteArray baEncryptedBlock;
    int ret;

    switch(crypto_algo) {
        case SEBIO_NO_CRYPTO: {
            idOnEviction = pdo::crypto::ComputeMessageHash(baBlockCopy);
            ret = pdo::block_store::BlockStorePut(idOnEviction, baBlockCopy);
            break;
        }
        case SEBIO_AES_GCM: {
            //check initialization
            pdo::error::ThrowIf<pdo::error::RuntimeError>(
                    sebio_ctx.crypto_algo != crypto_algo, "sebio_evict, crypto-algo does not match");
            baEncryptedBlock = pdo::crypto::skenc::EncryptMessage(sebio_ctx.key, baBlockCopy);
            //compute block id before it is evicted
            //Notice: since the block may have been encrypted, we propagate this id to the upper layers
            idOnEviction = pdo::crypto::ComputeMessageHash(baEncryptedBlock);
            ret = pdo::block_store::BlockStorePut(idOnEviction, baEncryptedBlock);
            break;
        }
        default:
            return STATE_ERR_UNIMPLEMENTED;
    }

    if(ret != 0) {
        SAFE_LOG(PDO_LOG_ERROR, "sebio error, block store put returned %d\n", ret);
        return STATE_ERR_UNKNOWN;
    }
    SAFE_LOG(PDO_LOG_DEBUG, "sebio evicted id: %s\n", ByteArrayToHexEncodedString(idOnEviction).c_str());
    return STATE_SUCCESS;
}
