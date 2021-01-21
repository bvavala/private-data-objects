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

#include "TestEnclave.h"
#include "TestEnclave_t.h"
#include "testCrypto.h"
#include "pdo_error.h"
void trusted_wrapper_ocall_Log(pdo_log_level_t level, const char* message)
{
    ocall_Log(level, message);
}

// Test ECALL
int test()
{
    return pdo::crypto::testCrypto();
}

extern int tc_posix_memalign(void **,size_t,size_t);

#ifdef __cplusplus
extern "C" {
#endif

int posix_memalign(void **memptr, size_t alignment, size_t size)
{
    void *p = memalign(alignment, size);
    if(!p)
        return 1;

    *memptr = p;
    return 0;
    //return tc_posix_memalign(memptr,alignment,size);
}

void __assert_fail(const char * assertion, const char * file, unsigned int line, const char * function)
{
    int n= 1;
    return;
}

int printf ( const char * format, ... )
{
    return 1;
}


int __printf_chk(int flag, const char * format)
{
    return 1;
}

#ifdef __cplusplus
}
#endif
