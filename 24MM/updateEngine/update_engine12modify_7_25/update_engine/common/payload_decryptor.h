/*
 * Copyright @ 2017 - 2020 iAUTO(Shanghai) Co., Ltd.
 * All Rights Reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are NOT permitted except as agreed by
 * iAUTO(Shanghai) Co., Ltd.
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 */

// #ifndef UPDATE_ENGINE_COMMON_PAYLOAD_DECRYPTOR_H_
// #define UPDATE_ENGINE_COMMON_PAYLOAD_DECRYPTOR_H_

// #include <base/macros.h>
// #include <brillo/secure_blob.h>

// #include <string>
// #include <vector>

// #include "update_engine/update_metadata.pb.h"
// #include "HsmListener.h"
// #include "HsmProxy.h"

// // This class encapsulates methods used for payload signing.
// // See update_metadata.proto for more info.

// namespace chromeos_update_engine {

// enum KeyName {
//   TMC_ENC_KEY   = 0,
//   TMC_SIG_KEY   = 1,
//   IAUTO_ENC_KEY = 2,
//   IAUTO_SIG_KEY = 3,

//   KEY_MAX,
// };

// class PayloadDecryptor : public nutshell::HsmListener {
//  public:
//   PayloadDecryptor();
//   ~PayloadDecryptor();

//   static void creat();
//   static void destroy();

//   static PayloadDecryptor *getinstance() { return pd_; }

//   virtual void onConnectChanged(bool isconn);
//   virtual void onNotifyPanic();

//   bool decrypt(const KeyName keyid, const std::string &ciphertext,
//                std::string &plaintext);
//   bool verifySignature(const KeyName keyid, const std::string &data,
//                        const std::string &signature);

//  private:
//   android::sp<nutshell::HsmProxy> hsm_;
//   static PayloadDecryptor *pd_;
// };
// }  // namespace chromeos_update_engine

// #endif  // UPDATE_ENGINE_COMMON_PAYLOAD_DECRYPTOR_H_