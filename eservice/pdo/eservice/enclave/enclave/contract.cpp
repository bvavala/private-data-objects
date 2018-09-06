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

#include "enclave_u.h"

#include "pdo_error.h"
#include "error.h"
#include "log.h"
#include "types.h"
#include "zero.h"
#include "jsonvalue.h"
#include "packages/parson/parson.h"

#include "crypto.h"
#include "enclave/enclave.h"
#include "enclave/base.h"
#include "enclave/contract.h"

#include "queue.h"

static bool q_IsInitialized = false;
static EnclaveQueue<int> queue;


static void initiliazeQueue(){
    int i = 0;
    for (pdo::enclave_api::Enclave& enc : g_Enclave) {

        queue.push(i);
        ++i;

    }
    q_IsInitialized = true;
}


// XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
size_t pdo::enclave_api::contract::ContractKeySize(void)
{
    // this is somewhat lucky because we currently fit precisely
    // in the AES block; will need to pad if
    return pdo::crypto::constants::IV_LEN + pdo::crypto::constants::SYM_KEY_LEN + pdo::crypto::constants::TAG_LEN;
}

// TODO: All things in this file should select a random enclave, rather than using the first one

// XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
pdo_err_t pdo::enclave_api::contract::VerifySecrets(
    const Base64EncodedString& inSealedEnclaveData,
    const std::string& inContractId,
    const std::string& inContractCreatorId, /* contract creator's public key */
    const std::string& inSerializedSecretList, /* json */
    Base64EncodedString& outEncryptedContractKey,
    Base64EncodedString& outContractKeySignature,
    int enclaveIndex
    )
{
    pdo_err_t result = PDO_SUCCESS;

    try
    {
        ByteArray sealed_enclave_data = Base64EncodedStringToByteArray(inSealedEnclaveData);
        ByteArray encrypted_contract_key(pdo::enclave_api::contract::ContractKeySize());
        ByteArray contract_key_signature(pdo::enclave_api::base::GetSignatureSize());

        // xxxxx call the enclave

        /// get the enclave id for passing into the ecall
        sgx_enclave_id_t enclaveid = g_Enclave[enclaveIndex].GetEnclaveId();
        Log(PDO_LOG_DEBUG, "VerifySecrets - [%u]Enclave_ID:  %ld ", enclaveIndex, (long)enclaveid);


        pdo_err_t presult = PDO_SUCCESS;
        sgx_status_t sresult =
            g_Enclave[enclaveIndex].CallSgx(
                [
                    enclaveid,
                    &presult,
                    sealed_enclave_data, // not sure why this needs to be passed by reference...
                    inContractId,
                    inContractCreatorId,
                    inSerializedSecretList,
                    &encrypted_contract_key,
                    &contract_key_signature
                ]
                ()
                {
                    sgx_status_t sresult_inner = ecall_VerifySecrets(
                        enclaveid,
                        &presult,
                        sealed_enclave_data.data(),
                        sealed_enclave_data.size(),
                        inContractId.c_str(),
                        inContractCreatorId.c_str(),
                        inSerializedSecretList.c_str(),
                        encrypted_contract_key.data(),
                        encrypted_contract_key.size(),
                        contract_key_signature.data(),
                        contract_key_signature.size());
                    return pdo::error::ConvertErrorStatus(sresult_inner, presult);
                }
                );
        pdo::error::ThrowSgxError(sresult, "SGX enclave call failed (VerifySecrets)");
        g_Enclave[enclaveIndex].ThrowPDOError(presult);

        outEncryptedContractKey = ByteArrayToBase64EncodedString(encrypted_contract_key);
        outContractKeySignature = ByteArrayToBase64EncodedString(contract_key_signature);
    }
    catch (pdo::error::Error& e)
    {
        pdo::enclave_api::base::SetLastError(e.what());
        result = e.error_code();
    }
    catch (std::exception& e)
    {
        pdo::enclave_api::base::SetLastError(e.what());
        result = PDO_ERR_UNKNOWN;
    }
    catch (...)
    {
        pdo::enclave_api::base::SetLastError("Unexpected exception");
        result = PDO_ERR_UNKNOWN;
    }

    return result;
}

// XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
pdo_err_t pdo::enclave_api::contract::HandleContractRequest(
    const Base64EncodedString& inSealedEnclaveData,
    const Base64EncodedString& inEncryptedSessionKey,
    const Base64EncodedString& inSerializedRequest,
    uint32_t& outResponseIdentifier,
    size_t& outSerializedResponseSize,
    int enclaveIndex
    )
{
    pdo_err_t result = PDO_SUCCESS;

    try
    {
        size_t response_size;
        ByteArray sealed_enclave_data = Base64EncodedStringToByteArray(inSealedEnclaveData);
        ByteArray encrypted_session_key = Base64EncodedStringToByteArray(inEncryptedSessionKey);
        ByteArray serialized_request = Base64EncodedStringToByteArray(inSerializedRequest);

        // xxxxx call the enclave

        /// get the enclave id for passing into the ecall
        sgx_enclave_id_t enclaveid = g_Enclave[enclaveIndex].GetEnclaveId();
        Log(PDO_LOG_DEBUG, "HandleContractRequest - [%u]Enclave_ID:  %ld ", enclaveIndex, (long)enclaveid);


        pdo_err_t presult = PDO_SUCCESS;
        sgx_status_t sresult =
            g_Enclave[enclaveIndex].CallSgx(
                [
                    enclaveid,
                    &presult,
                    sealed_enclave_data,
                    encrypted_session_key,
                    serialized_request,
                    &response_size
                ]
                ()
                {
                    sgx_status_t sresult_inner = ecall_HandleContractRequest(
                        enclaveid,
                        &presult,
                        sealed_enclave_data.data(),
                        sealed_enclave_data.size(),
                        encrypted_session_key.data(),
                        encrypted_session_key.size(),
                        serialized_request.data(),
                        serialized_request.size(),
                        &response_size);
                    return pdo::error::ConvertErrorStatus(sresult_inner, presult);
                }
                );
        pdo::error::ThrowSgxError(sresult, "SGX enclave call failed (InitializeContract)");
        g_Enclave[enclaveIndex].ThrowPDOError(presult);

        outSerializedResponseSize = response_size;

    }
    catch (pdo::error::Error& e)
    {
        pdo::enclave_api::base::SetLastError(e.what());
        result = e.error_code();
    }
    catch (std::exception& e)
    {
        pdo::enclave_api::base::SetLastError(e.what());
        result = PDO_ERR_UNKNOWN;
    }
    catch (...)
    {
        pdo::enclave_api::base::SetLastError("Unexpected exception");
        result = PDO_ERR_UNKNOWN;
    }

    return result;
}

// XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
pdo_err_t pdo::enclave_api::contract::GetSerializedResponse(
    const Base64EncodedString& inSealedEnclaveData,
    const uint32_t inResponseIdentifier,
    const size_t inSerializedResponseSize,
    Base64EncodedString& outSerializedResponse,
    int enclaveIndex
    )
{
    pdo_err_t result = PDO_SUCCESS;

    try
    {
        ByteArray serialized_response(inSerializedResponseSize);
        ByteArray sealed_enclave_data = Base64EncodedStringToByteArray(inSealedEnclaveData);

        // xxxxx call the enclave

        /// get the enclave id for passing into the ecall
        sgx_enclave_id_t enclaveid = g_Enclave[enclaveIndex].GetEnclaveId();
        Log(PDO_LOG_DEBUG, "GetSerializedResponse - [%u]Enclave_ID:  %ld ", enclaveIndex, (long)enclaveid);


        pdo_err_t presult = PDO_SUCCESS;
        sgx_status_t sresult =
            g_Enclave[enclaveIndex].CallSgx(
                [
                    enclaveid,
                    &presult,
                    sealed_enclave_data,
                    &serialized_response
                ]
                ()
                {
                    sgx_status_t sresult_inner = ecall_GetSerializedResponse(
                        enclaveid,
                        &presult,
                        sealed_enclave_data.data(),
                        sealed_enclave_data.size(),
                        serialized_response.data(),
                        serialized_response.size());
                    return pdo::error::ConvertErrorStatus(sresult_inner, presult);
                }
                );
        pdo::error::ThrowSgxError(sresult, "SGX enclave call failed (GetSerializedResponse)");
        g_Enclave[enclaveIndex].ThrowPDOError(presult);

        outSerializedResponse = ByteArrayToBase64EncodedString(serialized_response);
    }
    catch (pdo::error::Error& e)
    {
        pdo::enclave_api::base::SetLastError(e.what());
        result = e.error_code();
    }
    catch (std::exception& e)
    {
        pdo::enclave_api::base::SetLastError(e.what());
        result = PDO_ERR_UNKNOWN;
    }
    catch (...)
    {
        pdo::enclave_api::base::SetLastError("Unexpected exception");
        result = PDO_ERR_UNKNOWN;
    }

    return result;
}


// XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
pdo_err_t pdo::enclave_api::contract::DoECall(
        const std::string& inCallJson,
        std::string& outResultJson
    )
{
    pdo_err_t result = PDO_SUCCESS;

    if (!q_IsInitialized){
        initiliazeQueue();
    }


    try {

        // Parse the inCallJson
        JsonValue callJson(json_parse_string(inCallJson.c_str()));
        pdo::error::ThrowIfNull(callJson.value, "Failed to parse the enclave info, badly formed JSON");

        JSON_Object* callJson_object = json_value_get_object(callJson);
        pdo::error::ThrowIfNull(callJson_object, "Invalid callJson, expecting object");

        const char* svalue = nullptr;

        svalue = json_object_dotget_string(callJson_object, "method");
        pdo::error::ThrowIfNull(svalue, "Invalid method");
        std::string method = svalue;

        if (method == "contract_verify_secrets"){

            Log(PDO_LOG_DEBUG, "DoECall - method == contract_verify_secrets");

            svalue = json_object_dotget_string(callJson_object, "sealed_signup_data");
            pdo::error::ThrowIfNull(svalue, "Invalid sealed_signup_data");
            std::string sealed_signup_data = svalue;

            svalue = json_object_dotget_string(callJson_object, "contract_id");
            pdo::error::ThrowIfNull(svalue, "Invalid contract_id");
            std::string contract_id = svalue;

            svalue = json_object_dotget_string(callJson_object, "contract_creator_id");
            pdo::error::ThrowIfNull(svalue, "Invalid contract_creator_id");
            std::string contract_creator_id = svalue;

            svalue = json_object_dotget_string(callJson_object, "serialized_secret_list");
            pdo::error::ThrowIfNull(svalue, "Invalid serialized_secret_list");
            std::string serialized_secret_list = svalue;

            Base64EncodedString encrypted_contract_key_buffer;
            Base64EncodedString signature_buffer;


            int enclaveIndex = queue.pop();
            sgx_enclave_id_t enclaveid = g_Enclave[enclaveIndex].GetEnclaveId();
            Log(PDO_LOG_DEBUG, "DoECall - [%u]Enclave_ID:  %ld ", enclaveIndex, (long)enclaveid);


            pdo_err_t presult = VerifySecrets(
                sealed_signup_data,
                contract_id,
                contract_creator_id,
                serialized_secret_list,
                encrypted_contract_key_buffer,
                signature_buffer,
                enclaveIndex);
            if (presult != PDO_SUCCESS)
                return presult;

            queue.push(enclaveIndex);
            Log(PDO_LOG_DEBUG, "DoECall - [%u]Enclave_ID:  %ld Pushed", enclaveIndex, (long)g_Enclave[enclaveIndex].GetEnclaveId());

            JsonValue dataValue(json_value_init_object());
            JSON_Object* dataObject = json_value_get_object(dataValue);
            json_object_dotset_string(
                dataObject, "method", "contract_verify_secrets");
            json_object_dotset_string(
                dataObject, "encrypted_contract_key", encrypted_contract_key_buffer.c_str());
            json_object_dotset_string(
                dataObject, "signature", signature_buffer.c_str());

            size_t serializedSize = json_serialization_size(dataValue);
            std::vector<char> serialized_string;
            serialized_string.resize(serializedSize + 1);
            json_serialize_to_buffer(dataValue, &serialized_string[0], serializedSize);

            outResultJson.assign(&serialized_string[0]);

        }else if (method == "contract_handle_contract_request"){

            Log(PDO_LOG_DEBUG, "DoECall - method == contract_handle_contract_request");

            svalue = json_object_dotget_string(callJson_object, "sealed_signup_data");
            pdo::error::ThrowIfNull(svalue, "Invalid sealed_signup_data");
            std::string sealed_signup_data = svalue;

            svalue = json_object_dotget_string(callJson_object, "encrypted_session_key");
            pdo::error::ThrowIfNull(svalue, "Invalid encrypted_session_key");
            std::string encrypted_session_key = svalue;

            svalue = json_object_dotget_string(callJson_object, "serialized_request");
            pdo::error::ThrowIfNull(svalue, "Invalid serialized_request");
            std::string serialized_request = svalue;


            uint32_t response_identifier;
            size_t response_size;

            int enclaveIndex = queue.pop();
            sgx_enclave_id_t enclaveid = g_Enclave[enclaveIndex].GetEnclaveId();
            Log(PDO_LOG_DEBUG, "DoECall - [%u]Enclave_ID:  %ld ", enclaveIndex, (long)enclaveid);

            pdo_err_t presult = HandleContractRequest(
                sealed_signup_data,
                encrypted_session_key,
                serialized_request,
                response_identifier,
                response_size,
                enclaveIndex);
            if (presult != PDO_SUCCESS)
                return presult;

            Base64EncodedString response;

            presult = GetSerializedResponse(
                sealed_signup_data,
                response_identifier,
                response_size,
                response,
                enclaveIndex);
            if (presult != PDO_SUCCESS)
                return presult;

            queue.push(enclaveIndex);
            Log(PDO_LOG_DEBUG, "DoECall - [%u]Enclave_ID:  %ld Pushed", enclaveIndex, (long)g_Enclave[enclaveIndex].GetEnclaveId());

            JsonValue dataValue(json_value_init_object());
            JSON_Object* dataObject = json_value_get_object(dataValue);
            json_object_dotset_string(
                dataObject, "method", "contract_verify_secrets");
            json_object_dotset_string(
                dataObject, "response", response.c_str());

            size_t serializedSize = json_serialization_size(dataValue);
            std::vector<char> serialized_string;
            serialized_string.resize(serializedSize + 1);
            json_serialize_to_buffer(dataValue, &serialized_string[0], serializedSize);

            outResultJson.assign(&serialized_string[0]);
        }

    } catch (pdo::error::Error& e) {
        pdo::enclave_api::base::SetLastError(e.what());
        result = e.error_code();
    } catch (std::exception& e) {
        pdo::enclave_api::base::SetLastError(e.what());
        result = PDO_ERR_UNKNOWN;
    } catch (...) {
        pdo::enclave_api::base::SetLastError("Unexpected exception");
        result = PDO_ERR_UNKNOWN;
    }

    return result;
} // pdo::enclave_api::base::UnsealSignupData





