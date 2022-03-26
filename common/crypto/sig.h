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

#pragma once

#include <openssl/ec.h>
#include <openssl/obj_mac.h> //for the NIDs
#include <string>
#include <map>

#ifndef PDO_USE_ECDSA_CURVE
#define PDO_USE_ECDSA_CURVE NID_secp256k1
#endif

namespace pdo
{
namespace crypto
{
    namespace constants
    {
        // Elliptic curve
        const int CURVE = PDO_USE_ECDSA_CURVE;
        const int MAX_SIG_SIZE = 72;
    }

    namespace sig
    {
        enum class SigCurve
        {
            UNDEFINED,
            SECP256K1,
            SECP384R1,
            CURVE_COUNT
        };

        typedef struct {
            SigCurve sigCurve;
            int sslNID;
            void (*SHAFunc)(const unsigned char*, unsigned int, unsigned char hash[]);
            unsigned int shaDigestLength;
            unsigned int maxSigSize;
        } sig_details_t;

        extern const sig_details_t SigDetails[];
        extern const std::map<int, SigCurve> NidToSigCurveMap;

        class Key
        {
        public:
            virtual void Deserialize(const std::string& encoded) = 0;
            virtual void SetSigDetailsFromDeserializedKey();
            virtual std::string Serialize() const = 0;
            virtual unsigned int MaxSigSize() const;

        protected:
            EC_KEY* key_;
            sig_details_t sigDetails_;
        };
    }
}
}
