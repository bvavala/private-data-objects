#!/usr/bin/env python

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

"""
Enclave service.
"""

import os
import sys
import argparse

import signal

import pdo.common.config as pconfig
import pdo.common.keys as keys
import pdo.common.logger as plogger

import pdo.eservice.pdo_helper as pdo_enclave_helper
from pdo.common.wsgi import AppWrapperMiddleware
from pdo.eservice.wsgi import *

import logging
logger = logging.getLogger(__name__)

## XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
## XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
from twisted.web import http
from twisted.web.resource import Resource, NoResource
from twisted.web.server import Site
from twisted.python.threadpool import ThreadPool
from twisted.internet import reactor, defer
from twisted.internet.endpoints import TCP4ServerEndpoint
from twisted.web.wsgi import WSGIResource

## ----------------------------------------------------------------
def ErrorResponse(request, error_code, msg) :
    """Generate a common error response for broken requests
    """

    result = ""
    if request.method != 'HEAD' :
        result = msg + '\n'
        result = result.encode('utf8')

    request.setResponseCode(error_code)
    request.setHeader(b'Content-Type', b'text/plain')
    request.setHeader(b'Content-Length', len(result))
    request.write(result)

    try :
        request.finish()
    except :
        logger.exception("exception during request finish")
        raise

    return request

# -----------------------------------------------------------------
# -----------------------------------------------------------------
def __shutdown__(*args) :
    logger.warning('shutdown request received')
    reactor.callLater(1, reactor.stop)

# -----------------------------------------------------------------
# -----------------------------------------------------------------
def StartEnclaveService(config, enclave) :
    try :
        http_port = config['EnclaveService']['HttpPort']
        http_host = config['EnclaveService']['Host']
        storage_url = config['StorageService']['URL']
        worker_threads = config['EnclaveService'].get('WorkerThreads', 8)
        reactor_threads = config['EnclaveService'].get('ReactorThreads', 8)
    except KeyError as ke :
        logger.error('missing configuration for %s', str(ke))
        sys.exit(-1)

    logger.info('enclave service started on %s:%s', http_host, http_port)
    logger.info('verifying_key: %s', enclave.verifying_key)
    logger.info('encryption_key: %s', enclave.encryption_key)
    logger.info('enclave_id: %s', enclave.enclave_id)
    logger.info('storage service: %s', storage_url)

    thread_pool = ThreadPool(minthreads=1, maxthreads=worker_threads)
    thread_pool.start()
    reactor.addSystemEventTrigger('before', 'shutdown', thread_pool.stop)

    root = Resource()
    root.putChild(b'info', WSGIResource(reactor, thread_pool, AppWrapperMiddleware(InfoApp(enclave, storage_url))))
    root.putChild(b'initialize', WSGIResource(reactor, thread_pool, AppWrapperMiddleware(InitializeApp(enclave))))
    root.putChild(b'invoke', WSGIResource(reactor, thread_pool, AppWrapperMiddleware(InvokeApp(enclave))))
    root.putChild(b'verify', WSGIResource(reactor, thread_pool, AppWrapperMiddleware(VerifyApp(enclave))))

    site = Site(root, timeout=60)
    site.displayTracebacks = True

    reactor.suggestThreadPoolSize(reactor_threads)

    signal.signal(signal.SIGQUIT, __shutdown__)
    signal.signal(signal.SIGTERM, __shutdown__)

    endpoint = TCP4ServerEndpoint(reactor, http_port, backlog=32, interface=http_host)
    endpoint.listen(site)

# -----------------------------------------------------------------
# -----------------------------------------------------------------
def RunService() :
    @defer.inlineCallbacks
    def shutdown_twisted():
        logger.info("Stopping Twisted")
        yield reactor.callFromThread(reactor.stop)

    reactor.addSystemEventTrigger('before', 'shutdown', shutdown_twisted)

    try :
        reactor.run()
    except ReactorNotRunning:
        logger.warning('shutdown')
    except :
        logger.warning('shutdown')

    pdo_enclave_helper.shutdown_enclave()
    sys.exit(0)

# -----------------------------------------------------------------
# sealed_data is base64 encoded string
# -----------------------------------------------------------------
def LoadEnclaveData(enclave_config, txn_keys) :
    data_dir = enclave_config['DataPath']
    basename = enclave_config['BaseName']

    try :
        enclave = pdo_enclave_helper.Enclave.read_from_file(basename, data_dir = data_dir, txn_keys = txn_keys)
    except FileNotFoundError as fe :
        logger.warning("enclave information file missing; {0}".format(fe.filename))
        return None
    except Exception as e :
        logger.error("problem loading enclave information; %s", str(e))
        raise e

    return enclave

# -----------------------------------------------------------------
# -----------------------------------------------------------------
def CreateEnclaveData(enclave_config, ledger_config, txn_keys) :
    logger.warning('unable to locate the enclave data; creating new data')

    # create the enclave class
    try :
        enclave = pdo_enclave_helper.Enclave.create_new_enclave(txn_keys = txn_keys)
    except Exception as e :
        logger.error("unable to create a new enclave; %s", str(e))
        raise e

    # save the data to a file
    data_dir = enclave_config['DataPath']
    basename = enclave_config['BaseName']
    try :
        enclave.save_to_file(basename, data_dir = data_dir)
    except Exception as e :
        logger.error("unable to save new enclave; %s", str(e))
        raise e

    # register the enclave
    try :
        enclave.register_enclave(ledger_config)
    except Exception as e:
        logger.error("unable to register the enclave; %s", str(e))
        raise e

    return enclave

# -----------------------------------------------------------------
# -----------------------------------------------------------------
def LocalMain(config) :

    ledger_config = config['Ledger']
    ledger_type = ledger_config.get('LedgerType', os.environ.get('PDO_LEDGER_TYPE'))

    # load and initialize the enclave library
    try :
        logger.debug('initialize the enclave')
        pdo_enclave_helper.initialize_enclave(config)
    except Exception as e :
        logger.exception('failed to initialize enclave; %s', e)
        sys.exit(-1)

    # create the transaction keys needed to register the enclave
    try :
        key_config = config['Key']
        key_file = key_config['FileName']
        key_path = key_config['SearchPath']
        txn_keys = keys.read_transaction_keys_from_file(key_file, search_path = key_path,\
            ledger_type = ledger_type)
    except KeyError as ke :
        logger.error('missing configuration for %s', str(ke))
        sys.exit(-1)
    except Exception as e :
        logger.error('unable to load transaction keys; %s', str(e))
        sys.exit(-1)

    # create or load the enclave data
    try :
        enclave_config = config['EnclaveData']
        enclave = LoadEnclaveData(enclave_config, txn_keys)
        if enclave is None :
            enclave = CreateEnclaveData(enclave_config, ledger_config, txn_keys)
            assert enclave

        enclave.verify_registration(ledger_config)
    except KeyError as ke :
        logger.error('missing configuration for %s', str(ke))
        sys.exit(-1)
    except Exception as e :
        logger.error('failed to initialize the enclave; %s', str(e))
        sys.exit(-1)

    # set up the handlers for the enclave service
    try :
        StartEnclaveService(config, enclave)
    except Exception as e:
        logger.exception('failed to start the enclave service; %s', e)
        sys.exit(-1)

    # and run the service
    RunService()

## XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
## XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX

# -----------------------------------------------------------------
# -----------------------------------------------------------------
def Main() :
    config_map = pconfig.build_configuration_map()

    # parse out the configuration file first
    conffiles = [ 'eservice.toml' ]
    confpaths = [ ".", "./etc", config_map['etc'] ]

    parser = argparse.ArgumentParser()

    parser.add_argument('-b', '--bind', help='Define variables for configuration and script use', nargs=2, action='append')

    parser.add_argument('--config', help='configuration file', nargs = '+')
    parser.add_argument('--config-dir', help='directory to search for configuration files', nargs = '+')

    parser.add_argument('--identity', help='Identity to use for the process', required = True, type = str)

    parser.add_argument('--logfile', help='Name of the log file, __screen__ for standard output', type=str)
    parser.add_argument('--loglevel', help='Logging level', type=str)

    parser.add_argument('--http', help='Port on which to run the http server', type=int)
    parser.add_argument('--ledger', help='Default url for connection to the ledger', type=str)

    parser.add_argument('--block-store', help='Name of the file where blocks are stored', type=str)
    parser.add_argument('--sservice-url', help='URL for the associated storage service', type=str)

    parser.add_argument('--enclave-data', help='Name of the file containing enclave sealed storage', type=str)
    parser.add_argument('--enclave-save', help='Name of the directory where enclave data will be save', type=str)
    parser.add_argument('--enclave-path', help='Directories to search for the enclave data file', type=str, nargs = '+')

    parser.add_argument('--sgx-key-root', help='Path to SGX key root folder', type = str)

    options = parser.parse_args()

    # first process the options necessary to load the default configuration
    if options.config :
        conffiles = options.config

    if options.config_dir :
        confpaths = options.config_dir

    config_map['identity'] = options.identity

    # set up the configuration mapping from the parameters
    if options.bind :
        for (k, v) in options.bind : config_map[k] = v

    # parse the configuration files
    try :
        config = pconfig.parse_configuration_files(conffiles, confpaths, config_map)
    except pconfig.ConfigurationException as e :
        logger.error(str(e))
        sys.exit(-1)

    # set up the logging configuration
    if config.get('Logging') is None :
        config['Logging'] = {
            'LogFile' : '__screen__',
            'LogLevel' : 'INFO'
        }
    if options.logfile :
        config['Logging']['LogFile'] = options.logfile
    if options.loglevel :
        config['Logging']['LogLevel'] = options.loglevel.upper()

    plogger.setup_loggers(config.get('Logging', {}))
    sys.stdout = plogger.stream_to_logger(logging.getLogger('STDOUT'), logging.DEBUG)
    sys.stderr = plogger.stream_to_logger(logging.getLogger('STDERR'), logging.WARN)

    # set up the ledger configuration
    if config.get('Ledger') is None :
        config['Ledger'] = {
            'LedgerURL' : 'http://localhost:6600',
        }
    if options.ledger :
        config['Ledger']['LedgerURL'] = options.ledger

    # set up the enclave service configuration
    if config.get('EnclaveService') is None :
        config['EnclaveService'] = {
            'HttpPort' : 7101,
            'Host' : 'localhost',
            'Identity' : 'enclave'
        }
    if options.http :
        config['EnclaveService']['HttpPort'] = options.http

    if config.get('EnclaveData') is None :
        config['EnclaveData'] = {
            'FileName' : 'enclave.data',
            'SavePath' : './data',
            'SearchPath' : [ '.', './data', config_map['data'] ]
        }
    if options.enclave_data :
        config['EnclaveData']['FileName'] = options.enclave_data
    if options.enclave_save :
        config['EnclaveData']['SavePath'] = options.enclave_save
    if options.enclave_path :
        config['EnclaveData']['SearchPath'] = options.enclave_path

    # set up the enclave service configuration
    if config.get('StorageService') is None :
        config['StorageService'] = {
            'BlockStore' : os.path.join(config_map['data'], options.identity + '.mdb'),
            'URL' : 'http://localhost:7201'
        }
    if options.block_store :
        config['StorageService']['BlockStore'] = options.block_store
    if options.sservice_url :
        config['StorageService']['URL'] = options.sservice_url

    if config['StorageService'].get('URL') is None :
        host = config['StorageService'].get('Host','localhost')
        port = config['StorageService'].get('HttpPort',7201)
        config['StorageService']['URL'] = "http://{0}:{1}".format(host, port)

    # set up the default enclave module configuration (if necessary)
    if config.get('EnclaveModule') is None :
        config['EnclaveModule'] = {
            'NumberOfEnclaves' : 7,
            'ias_url' : 'https://api.trustedservices.intel.com/sgx/dev',
            'sgx_key_root' : os.environ.get('PDO_SGX_KEY_ROOT', '.')
        }

    # override the enclave module configuration (if options are specified)
    if options.sgx_key_root :
        config['EnclaveModule']['sgx_key_root'] = options.sgx_key_root

    # GO!
    LocalMain(config)

## -----------------------------------------------------------------
## Entry points
## -----------------------------------------------------------------
Main()
