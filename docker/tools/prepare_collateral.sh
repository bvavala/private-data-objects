#!/bin/bash

# Copyright 2024 Intel Corporation
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.


# This script prepares the collateral on hte host for the docker container build.
# environment.sh is not imported, as this script is meant to run on the host.

[ ! -z "${PDO_SOURCE_ROOT}" ] || { echo "PDO_SOURCE_ROOT not defined"; exit 1; }
source ${PDO_SOURCE_ROOT}/bin/lib/common.sh

function prepare_buildtime_enclavesigningkey () {
    # If an enclave signing key is available on the host, copy that under build/keys in the repo
    # Note: on the host, the key must be in ${PDO_SGX_KEY_ROOT}/enclave_code_sign.pem,
    # and the env variable must be defined.
    # Note: in the docker container, the host key (or a new key) will be placed on the same path,
    # but the PDO_SGX_KEY_ROOT default value is defined in docker/tools/environment.sh

    DEFAULT_KEY_PATH="${PDO_SOURCE_ROOT}/build/keys/sgx_mode_${SGX_MODE,,}/enclave_code_sign.pem"
    if [ ! -z "${PDO_SGX_KEY_ROOT}" ]; then
        yell "Enclave signing key: PDO_SGX_KEY_ROOT is defined"
        if [ -e "${PDO_SGX_KEY_ROOT}/enclave_code_sign.pem" ]; then
            yell "Enclave signing key: using host-provided key: ${PDO_SGX_KEY_ROOT}/enclave_code_sign.pem"
            # if source and destination are not the same, copy the key
            (test ${PDO_SGX_KEY_ROOT}/enclave_code_sign.pem -ef ${DEFAULT_KEY_PATH} ||
                cp ${PDO_SGX_KEY_ROOT}/enclave_code_sign.pem ${DEFAULT_KEY_PATH})
        else
            yell "Enclave signing key: not found on host, a new one will be generated"
        fi
    else
        yell "Enclave signing key: PDO_SGX_KEY_ROOT not defined; if a key is in the default path, it will be used"
    fi
}

function prepare_runtime_sgxkeys () {
    # check for collateral in PDO_SGX_KEY_ROOT and copy that in xfer
    # or, copy anything in the default folder to xfer

    [ ! -z "${DOCKER_DIR}" ] || die "DOCKER_DIR not defined"

    if [ ! -z "${PDO_SGX_KEY_ROOT}" ]; then
        # PDO_SGX_KEY_ROOT is set
        yell "SGX collateral: checking for source SGX collateral in ${PDO_SGX_KEY_ROOT}"
        if [ ! -f ${PDO_SGX_KEY_ROOT}/sgx_spid_api_key.txt ] ||
            [ ! -f ${PDO_SGX_KEY_ROOT}/sgx_spid.txt ] ||
            [ ! -f ${PDO_SGX_KEY_ROOT}/sgx_ias_key.pem ]; then
                    yell "SGX collateral: missing - check PDO_SGX_KEY_ROOT and SGX collateral in it"
                    exit 1
        fi
        
        yell "SGX collateral: found ... copying it to docker"
        cp ${PDO_SGX_KEY_ROOT}/* ${DOCKER_DIR}/xfer/services/keys/sgx/
        
    else
        yell "SGX collateral: PDO_SGX_KEY_ROOT undefined... rsyncing default folder to docker"
        rsync -r ${PDO_SOURCE_ROOT}/build/keys/sgx_mode_hw/ ${DOCKER_DIR}/xfer/services/keys/sgx/
    fi
    
    #test collateral availability in xfer
    # this succeeds if it was copied above, or if it was already in place
    yell "SGX collateral: checking for SGX collateral in docker"
    if [ ! -f ${DOCKER_DIR}/xfer/services/keys/sgx/sgx_spid_api_key.txt ] ||
        [ ! -f ${DOCKER_DIR}/xfer/services/keys/sgx/sgx_spid.txt ] ||
        [ ! -f ${DOCKER_DIR}/xfer/services/keys/sgx/sgx_ias_key.pem ]; then
            yell "SGX collateral: not found in docker -- set PDO_SGX_KEY_ROOT and check collateral"
            exit 1
    fi
    yell "SGX collateral: docker-ready"
}
