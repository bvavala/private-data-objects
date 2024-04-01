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


# This script copies the enclave signing key on the host (if any) for the docker container build.
# environment.sh is not imported, as this script is meant to run on the host.
# Note: the key (if any) is copied into the default folder for HW mode builds;
#       so for SIM mode builds, this will have no effect.

if [ $# != 1 ] && [ $# != 2 ]; then
    echo "$(basename $0 '${PDO_SOURCE_ROOT} [ ${PDO_SGX_KEY_ROOT} ]')"
    echo "PDO_SOURCE_ROOT is required, PDO_SGX_KEY_ROOT is optional"
    exit 1
fi

PDO_SOURCE_ROOT=$1
PDO_SGX_KEY_ROOT=$2

source ${PDO_SOURCE_ROOT}/bin/lib/common.sh

# If an enclave signing key is available on the host, copy that under build/keys in the repo
# Note: on the host, the key must be in ${PDO_SGX_KEY_ROOT}/enclave_code_sign.pem,
# and the env variable must be defined.
# Note: in the docker container, the host key (or a new key) will be placed on the same path,
# but the PDO_SGX_KEY_ROOT default value is defined in docker/tools/environment.sh

DEFAULT_KEY_PATH="${PDO_SOURCE_ROOT}/build/keys/sgx_mode_hw/enclave_code_sign.pem"

if [ ! -z "${PDO_SGX_KEY_ROOT}" ]; then
    yell "Enclave signing key: PDO_SGX_KEY_ROOT is defined"
    if [ -e "${PDO_SGX_KEY_ROOT}/enclave_code_sign.pem" ]; then
        yell "Enclave signing key: using host-provided key: ${PDO_SGX_KEY_ROOT}/enclave_code_sign.pem"

        rsync ${PDO_SGX_KEY_ROOT}/enclave_code_sign.pem ${DEFAULT_KEY_PATH}
        yell "Enclave signing key: host-provided key copied to ${DEFAULT_KEY_PATH}"
    else
        yell "Enclave signing key: not found on host, a new one will be generated"
    fi
else
    yell "Enclave signing key: PDO_SGX_KEY_ROOT not defined; if a key is in the default path, it will be used"
fi

exit 0

