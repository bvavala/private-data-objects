#!/bin/bash

# Copyright 2018 Intel Corporation
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

# -----------------------------------------------------------------
# -----------------------------------------------------------------
SCRIPTDIR="$(dirname $(readlink --canonicalize ${BASH_SOURCE}))"
SRCDIR="$(realpath ${SCRIPTDIR}/../..)"

source ${SRCDIR}/bin/lib/common.sh
check_python_version

PDO_LOG_LEVEL=${PDO_LOG_LEVEL:-info}

# -----------------------------------------------------------------
# -----------------------------------------------------------------
if [ "${PDO_INTERPRETER}" == "wawaka-aot" ]; then
    die Automated tests for the wawaka-aot interpreter are currently not supported.
fi

# -----------------------------------------------------------------
# -----------------------------------------------------------------
SCRIPTDIR="$(dirname $(readlink --canonicalize ${BASH_SOURCE}))"
SRCDIR="$(realpath ${SCRIPTDIR}/../..)"

: "${PDO_HOME:-$(die Missing environment variable PDO_HOME)}"
: "${PDO_LEDGER_URL:-$(die Missing environment variable PDO_LEDGER_URL)}"

SAVE_FILE=$(mktemp /tmp/pdo-test.XXXXXXXXX)
ESDB_FILE=$(mktemp /tmp/pdo-test.XXXXXXXXX)

declare -i NUM_SERVICES=5 # must be at least 3 for pconntract update test to work
function cleanup {
    yell "shutdown services"
    ${PDO_HOME}/bin/ps-stop.sh --count ${NUM_SERVICES} > /dev/null
    ${PDO_HOME}/bin/es-stop.sh --count ${NUM_SERVICES} > /dev/null
    ${PDO_HOME}/bin/ss-stop.sh --count ${NUM_SERVICES} > /dev/null
    rm -f ${SAVE_FILE} ${ESDB_FILE}
}

trap cleanup EXIT

# -----------------------------------------------------------------
# some checks to make sure we are ready to run
# -----------------------------------------------------------------
if [ "${PDO_LEDGER_TYPE}" == "ccf" ]; then
    if [ ! -f "${PDO_LEDGER_KEY_ROOT}/networkcert.pem" ]; then
        die "CCF ledger keys are missing, please copy and try again"
    fi
fi

# -----------------------------------------------------------------
yell start enclave and provisioning services
# -----------------------------------------------------------------
try ${PDO_HOME}/bin/ss-start.sh --loglevel ${PDO_LOG_LEVEL} --count ${NUM_SERVICES} > /dev/null
try ${PDO_HOME}/bin/ps-start.sh --loglevel ${PDO_LOG_LEVEL} --count ${NUM_SERVICES} --ledger ${PDO_LEDGER_URL} --clean > /dev/null
try ${PDO_HOME}/bin/es-start.sh --loglevel ${PDO_LOG_LEVEL} --count ${NUM_SERVICES} --ledger ${PDO_LEDGER_URL} --clean > /dev/null

cd ${SRCDIR}/build


## -----------------------------------------------------------------
yell start tests with provisioning and enclave services
## -----------------------------------------------------------------
say run unit tests for eservice database
cd ${SRCDIR}/python/pdo/test
try python servicedb.py --logfile $PDO_HOME/logs/client.log --loglevel ${PDO_LOG_LEVEL} \
    --eservice-db ${ESDB_FILE} \
    --url http://localhost:7101/ http://localhost:7102/ http://localhost:7103/ \
    --ledger ${PDO_LEDGER_URL}
try rm -f ${ESDB_FILE}

cd ${SRCDIR}/build

say create the eservice database using database CLI
try pdo-eservicedb --loglevel ${PDO_LOG_LEVEL} reset --create
try pdo-eservicedb --loglevel ${PDO_LOG_LEVEL} add -u http://localhost:7101 -n es7101
try pdo-eservicedb --loglevel ${PDO_LOG_LEVEL} add -u http://localhost:7102 -n es7102
try pdo-eservicedb --loglevel ${PDO_LOG_LEVEL} add -u http://localhost:7103 -n es7103
try pdo-eservicedb --loglevel ${PDO_LOG_LEVEL} add -u http://localhost:7104 -n es7104
try pdo-eservicedb --loglevel ${PDO_LOG_LEVEL} add -u http://localhost:7105 -n es7105

# -----------------------------------------------------------------
# -----------------------------------------------------------------
if [[ "$PDO_INTERPRETER" =~ ^"wawaka" ]]; then
    yell run system tests for contracts

    cd ${SRCDIR}/contracts/wawaka
    try make system-test TEST_LOG_LEVEL=${PDO_LOG_LEVEL}
else
    yell no system tests for "${PDO_INTERPRETER}"
fi

yell completed all tests
exit 0
