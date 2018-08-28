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

#include <algorithm>
#include <string>
#include <vector>

#include "crypto.h"
#include "error.h"
#include "hex_string.h"
#include "log.h"
#include "pdo_error.h"
#include "types.h"

#include "enclave/enclave.h"
#include "enclave/base.h"

static bool g_IsInitialized = false;
static std::string g_LastError;

// XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
// XX External interface                                             XX
// XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX

// XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
int pdo::enclave_api::base::IsSgxSimulator()
{
#if defined(SGX_SIMULATOR)
#if SGX_SIMULATOR == 1
    return 1;
#else // SGX_SIMULATOR not 1
    return 0;
#endif //  #if SGX_SIMULATOR == 1
#else // SGX_SIMULATOR not defined
    return 0;
#endif // defined(SGX_SIMULATOR)
} // pdo::enclave_api::base::IsSgxSimulator

// XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
void pdo::enclave_api::base::SetLastError(
    const std::string& message
    )
{
    g_LastError = message;
} // pdo::enclave_api::base::SetLastError

// XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
std::string pdo::enclave_api::base::GetLastError(void)
{
    return g_LastError;
} // pdo::enclave_api::base::GetLastErrorMessage

// XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
pdo_err_t pdo::enclave_api::base::Initialize(
    const std::string& inPathToEnclave,
    const HexEncodedString& inSpid,
    pdo_log_t logFunction
    )
{
    pdo_err_t ret = PDO_SUCCESS;

    try {
        if (!g_IsInitialized)
        {
            pdo::SetLogFunction(logFunction);
            for (pdo::enclave_api::Enclave& enc : g_Enclave) {
                enc.SetSpid(inSpid);
                enc.Load(inPathToEnclave);
            }
            g_IsInitialized = true;
        }
    } catch (pdo::error::Error& e) {
        pdo::enclave_api::base::SetLastError(e.what());
        ret = e.error_code();
    } catch(std::exception& e) {
        pdo::enclave_api::base::SetLastError(e.what());
        ret = PDO_ERR_UNKNOWN;
    } catch(...) {
        pdo::enclave_api::base::SetLastError("Unexpected exception");
        ret = PDO_ERR_UNKNOWN;
    }

    return ret;
} // pdo::enclave_api::base::Initialize

// XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
pdo_err_t pdo::enclave_api::base::Terminate()
{
    // Unload the enclave
    pdo_err_t ret = PDO_SUCCESS;

    try {
        if (g_IsInitialized) {
            for (pdo::enclave_api::Enclave& enc : g_Enclave) {
                enc.Unload();
            }
            g_IsInitialized = false;
        }
    } catch (pdo::error::Error& e) {
        pdo::enclave_api::base::SetLastError(e.what());
        ret = e.error_code();
    } catch (std::exception& e) {
        pdo::enclave_api::base::SetLastError(e.what());
        ret = PDO_ERR_UNKNOWN;
    } catch (...) {
        pdo::enclave_api::base::SetLastError("Unexpected exception");
        ret = PDO_ERR_UNKNOWN;
    }

    return ret;
} // pdo::enclave_api::base::Terminate

// XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
size_t pdo::enclave_api::base::GetEnclaveQuoteSize()
{
    return g_Enclave[0].GetQuoteSize();
} // pdo::enclave_api::base::GetEnclaveQuoteSize

// XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
size_t pdo::enclave_api::base::GetSignatureSize()
{
    // this is the size of the byte array required for the signature
    // fixed constant for now until there is one we can get from the
    // crypto library
    return pdo::crypto::constants::MAX_SIG_SIZE;
}

// XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
pdo_err_t pdo::enclave_api::base::GetEpidGroup(
    HexEncodedString& outEpidGroup
    )
{
     pdo_err_t ret = PDO_SUCCESS;

     try {
        // Get the EPID group from the enclave and convert it to big endian
        sgx_epid_group_id_t epidGroup = { 0 };
        g_Enclave[0].GetEpidGroup(&epidGroup);

        std::reverse((uint8_t*)&epidGroup, (uint8_t*)&epidGroup + sizeof(epidGroup));

        // Convert the binary data to a hex string
        outEpidGroup = pdo::BinaryToHexString((const uint8_t*)&epidGroup, sizeof(epidGroup));
    } catch (pdo::error::Error& e) {
        pdo::enclave_api::base::SetLastError(e.what());
        ret = e.error_code();
    } catch (std::exception& e) {
        pdo::enclave_api::base::SetLastError(e.what());
        ret = PDO_ERR_UNKNOWN;
    } catch (...) {
        pdo::enclave_api::base::SetLastError("Unexpected exception");
        ret = PDO_ERR_UNKNOWN;
    }

    return ret;
} // pdo::enclave_api::base::GetEpidGroup

// XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
pdo_err_t pdo::enclave_api::base::GetEnclaveCharacteristics(
    HexEncodedString& outMrEnclave,
    HexEncodedString& outEnclaveBasename
    )
{
    pdo_err_t ret = PDO_SUCCESS;

    try {
        // Get the enclave characteristics and then convert the binary data to
        // hex strings and copy them to the caller's buffers.
        sgx_measurement_t enclaveMeasurement;
        sgx_basename_t enclaveBasename;

        g_Enclave[0].GetEnclaveCharacteristics(
            &enclaveMeasurement,
            &enclaveBasename);

        outMrEnclave = pdo::BinaryToHexString(
            enclaveMeasurement.m,
            sizeof(enclaveMeasurement.m));

        outEnclaveBasename = pdo::BinaryToHexString(
            enclaveBasename.name,
            sizeof(enclaveBasename.name));

        for (pdo::enclave_api::Enclave& enc : g_Enclave) {

            sgx_enclave_id_t enclaveid = enc.GetEnclaveId();
            Log(PDO_LOG_DEBUG, "Enclave_ID:  %ld ", (long)enclaveid);

            enc.GetEnclaveCharacteristics(
                &enclaveMeasurement,
                &enclaveBasename);

            HexEncodedString logMrEnclave = pdo::BinaryToHexString(
                enclaveMeasurement.m,
                sizeof(enclaveMeasurement.m));

            HexEncodedString logEnclaveBasename = pdo::BinaryToHexString(
                enclaveBasename.name,
                sizeof(enclaveBasename.name));

            Log(PDO_LOG_DEBUG, "[%u]enclaveMeasurement:  %s ", enc, logMrEnclave.c_str());
            Log(PDO_LOG_DEBUG, "[%u]enclaveBasename:  %s ", enc, logEnclaveBasename.c_str());
        }



    } catch (pdo::error::Error& e) {
        pdo::enclave_api::base::SetLastError(e.what());
        ret = e.error_code();
    } catch (std::exception& e) {
        pdo::enclave_api::base::SetLastError(e.what());
        ret = PDO_ERR_UNKNOWN;
    } catch (...) {
        pdo::enclave_api::base::SetLastError("Unexpected exception");
        ret = PDO_ERR_UNKNOWN;
    }

    return ret;
} // pdo::enclave_api::base::GetEnclaveCharacteristics

// XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
pdo_err_t pdo::enclave_api::base::SetSignatureRevocationList(
    const std::string& inSignatureRevocationList
    )
{
    pdo_err_t ret = PDO_SUCCESS;

    try {
        for (pdo::enclave_api::Enclave& enc : g_Enclave) {
            enc.SetSignatureRevocationList(inSignatureRevocationList);
        }
    } catch (pdo::error::Error& e) {
        pdo::enclave_api::base::SetLastError(e.what());
        ret = e.error_code();
    } catch (std::exception& e) {
        pdo::enclave_api::base::SetLastError(e.what());
        ret = PDO_ERR_UNKNOWN;
    } catch (...) {
        pdo::enclave_api::base::SetLastError("Unexpected exception");
        ret = PDO_ERR_UNKNOWN;
    }

    return ret;
} // pdo::enclave_api::base::SetSignatureRevocationList
