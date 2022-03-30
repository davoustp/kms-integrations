/*
 * Copyright 2021 Google LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef KMSP11_OPERATION_CRYPTER_INTERFACES_H_
#define KMSP11_OPERATION_CRYPTER_INTERFACES_H_

#include "absl/status/statusor.h"
#include "kmsp11/object.h"
#include "kmsp11/util/kms_client.h"

namespace kmsp11 {

class EncrypterInterface {
 public:
  virtual absl::StatusOr<absl::Span<const uint8_t>> Encrypt(
      KmsClient* client, absl::Span<const uint8_t> plaintext) = 0;

  virtual ~EncrypterInterface() {}
};

class DecrypterInterface {
 public:
  virtual absl::StatusOr<absl::Span<const uint8_t>> Decrypt(
      KmsClient* client, absl::Span<const uint8_t> ciphertext) = 0;

  virtual ~DecrypterInterface() {}
};

class SignerInterface {
 public:
  virtual size_t signature_length() = 0;
  virtual Object* object() = 0;

  virtual absl::Status Sign(KmsClient* client, absl::Span<const uint8_t> data,
                            absl::Span<uint8_t> signature) = 0;
  virtual absl::Status SignUpdate(KmsClient* client,
                                  absl::Span<const uint8_t> data) = 0;
  virtual absl::Status SignFinal(KmsClient* client,
                                 absl::Span<uint8_t> signature) = 0;

  virtual ~SignerInterface() {}
};

class VerifierInterface {
 public:
  virtual Object* object() = 0;

  virtual absl::Status Verify(KmsClient* client, absl::Span<const uint8_t> data,
                              absl::Span<const uint8_t> signature) = 0;
  virtual absl::Status VerifyUpdate(KmsClient* client,
                                    absl::Span<const uint8_t> data) = 0;
  virtual absl::Status VerifyFinal(KmsClient* client,
                                   absl::Span<const uint8_t> signature) = 0;

  virtual ~VerifierInterface() {}
};

}  // namespace kmsp11

#endif  // KMSP11_OPERATION_CRYPTER_INTERFACES_H_
