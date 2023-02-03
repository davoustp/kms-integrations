// Copyright 2023 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "kmsp11/test/common_setup.h"

#include <fstream>

#include "fakekms/cpp/fakekms.h"
#include "kmsp11/test/resource_helpers.h"
#include "kmsp11/util/crypto_utils.h"

namespace kmsp11 {

std::string CreateConfigFileWithOneKeyring(fakekms::Server* fake_server) {
  kms_v1::KeyRing kr;
  return CreateConfigFileWithOneKeyring(fake_server, &kr);
}

std::string CreateConfigFileWithOneKeyring(fakekms::Server* fake_server,
                                           kms_v1::KeyRing* kr) {
  auto client = fake_server->NewClient();
  *kr = CreateKeyRingOrDie(client.get(), kTestLocation, RandomId(), *kr);

  std::string config_file = std::tmpnam(nullptr);
  std::ofstream(config_file)
      << absl::StrFormat(R"(
tokens:
  - key_ring: "%s"
    label: "foo"
kms_endpoint: "%s"
use_insecure_grpc_channel_credentials: true
)",
                         kr->name(), fake_server->listen_addr());
  return config_file;
}

CK_C_INITIALIZE_ARGS InitArgs(const char* config_file) {
  CK_C_INITIALIZE_ARGS init_args = {0};
  init_args.flags = CKF_OS_LOCKING_OK;
  init_args.pReserved = const_cast<char*>(config_file);
  return init_args;
}

}  // namespace kmsp11
