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

import os
import sys
import json
import urllib.request
import urllib.error

import logging
logger = logging.getLogger(__name__)

import random

class MessageException(Exception) :
    """
    A class to capture communication exceptions when communicating with services
    """
    pass

class GenericServiceClient(object) :

    def __init__(self, url) :
        self.ServiceURL = url.rstrip('/')
        self.ProxyHandler = urllib.request.ProxyHandler({})

        self.Identifier = 'session{0:05d}'.format(random.randint(0,1<<16))
        logger.debug('starting session %s', self.Identifier)

    def _postmsg(self, request) :
        """
        Post a transaction message to the validator, parse the returning JSON and return
        the corresponding dictionary.
        """

        data = json.dumps(request).encode('utf8')
        datalen = len(data)

        url = self.ServiceURL

        logger.debug('post transaction to %s with DATALEN=%d, DATA=<%s>', url, datalen, data)

        try :
            request = urllib.request.Request(url, data, {'Content-Type': 'application/json', 'Content-Length': datalen})
            opener = urllib.request.build_opener(self.ProxyHandler)
            response = opener.open(request, timeout=10)

        except urllib.error.HTTPError as err :
            logger.warning('operation failed with response: %s', err.code)
            raise MessageException('operation failed with resonse: {0}'.format(err.code))

        except urllib.error.URLError as err :
            logger.warning('operation failed: %s', err.reason)
            raise MessageException('operation failed: {0}'.format(err.reason))

        except :
            logger.warning('no response from server')
            raise MessageException('no response from server')

        content = response.read()
        headers = response.info()
        response.close()

        encoding = headers.get('Content-Type')
        if encoding != 'application/json' :
            logger.info('server responds with message %s of type %s', content, encoding)
            return None

        # Attempt to decode the content if it is not already a string
        try:
            content = content.decode('utf-8')
        except AttributeError:
            pass
        value = json.loads(content)
        return value
