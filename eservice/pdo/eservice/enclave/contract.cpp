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

#include <stdlib.h>
#include <string>
#include <map>

#include "error.h"
#include "pdo_error.h"
#include "swig_utils.h"
#include "types.h"
#include "jsonvalue.h"
#include "packages/parson/parson.h"

#include "contract.h"

#include "enclave/base.h"
#include "enclave/contract.h"
#include "enclave/block_store.h"

// XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
std::map<std::string, std::string> contract_verify_secrets(
    const std::string& sealed_signup_data,
    const std::string& contract_id,
    const std::string& contract_creator_id,
    const std::string& serialized_secret_list
    )
{

    JsonValue dataValue(json_value_init_object());
    JSON_Object* dataObject = json_value_get_object(dataValue);
    json_object_dotset_string(
        dataObject, "method", "contract_verify_secrets");
    json_object_dotset_string(
        dataObject, "sealed_signup_data", sealed_signup_data.c_str());
    json_object_dotset_string(
        dataObject, "contract_id", contract_id.c_str());
    json_object_dotset_string(
        dataObject, "contract_creator_id", contract_creator_id.c_str());
    json_object_dotset_string(
        dataObject, "serialized_secret_list", serialized_secret_list.c_str());

    size_t serializedSize = json_serialization_size(dataValue);
    std::vector<char> serialized_buffer;
    serialized_buffer.resize(serializedSize + 1);
    json_serialize_to_buffer(dataValue, &serialized_buffer[0], serializedSize);

    std::string inCallBuffer = &serialized_buffer[0];
    std::string outResultBuffer;

     // Create the signup data
    pdo_err_t presult = pdo::enclave_api::contract::DoECall(
        inCallBuffer,
        outResultBuffer);
    ThrowPDOError(presult);


    // Parse the outResultBuffer
    JsonValue resultBuffer(json_parse_string(outResultBuffer.c_str()));
    pdo::error::ThrowIfNull(resultBuffer.value, "Failed to parse the enclave info, badly formed JSON");

    JSON_Object* resultBuffer_object = json_value_get_object(resultBuffer);
    pdo::error::ThrowIfNull(resultBuffer_object, "Invalid resultBuffer, expecting object");

    const char* svalue = nullptr;

    svalue = json_object_dotget_string(resultBuffer_object, "encrypted_contract_key");
    pdo::error::ThrowIfNull(svalue, "Invalid encrypted_contract_key");
    const std::string encrypted_contract_key = svalue;

    svalue = json_object_dotget_string(resultBuffer_object, "signature");
    pdo::error::ThrowIfNull(svalue, "Invalid signature");
    const std::string signature = svalue;

    std::map<std::string, std::string> result;
    result["encrypted_state_encryption_key"] = encrypted_contract_key;
    result["signature"] = signature;

    return result;
}

// XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
std::string contract_handle_contract_request(
    const std::string& sealed_signup_data,
    const std::string& encrypted_session_key,
    const std::string& serialized_request
    )
{
    pdo_err_t presult;

    uint32_t response_identifier;
    size_t response_size;

    JsonValue dataValue(json_value_init_object());
    JSON_Object* dataObject = json_value_get_object(dataValue);
    json_object_dotset_string(
        dataObject, "method", "contract_handle_contract_request");
    json_object_dotset_string(
        dataObject, "sealed_signup_data", sealed_signup_data.c_str());
    json_object_dotset_string(
        dataObject, "encrypted_session_key", encrypted_session_key.c_str());
    json_object_dotset_string(
        dataObject, "serialized_request", serialized_request.c_str());


    size_t serializedSize = json_serialization_size(dataValue);
    std::vector<char> serialized_buffer;
    serialized_buffer.resize(serializedSize + 1);
    json_serialize_to_buffer(dataValue, &serialized_buffer[0], serializedSize);

    std::string inCallBuffer = &serialized_buffer[0];
    std::string outResultBuffer;

     // Create the signup data
    presult = pdo::enclave_api::contract::DoECall(
        inCallBuffer,
        outResultBuffer);
    ThrowPDOError(presult);

    // Parse the outResultBuffer
    JsonValue resultBuffer(json_parse_string(outResultBuffer.c_str()));
    pdo::error::ThrowIfNull(resultBuffer.value, "Failed to parse the enclave info, badly formed JSON");

    JSON_Object* resultBuffer_object = json_value_get_object(resultBuffer);
    pdo::error::ThrowIfNull(resultBuffer_object, "Invalid resultBuffer, expecting object");

    const char* svalue = nullptr;

    svalue = json_object_dotget_string(resultBuffer_object, "response");
    pdo::error::ThrowIfNull(svalue, "Invalid response");
    const std::string response = svalue;

    return response;
}

