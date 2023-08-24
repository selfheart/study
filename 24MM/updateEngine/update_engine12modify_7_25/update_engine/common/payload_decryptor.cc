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
// #ifdef UPDATE_ENGINE_FOR_CAR

// #include "update_engine/common/payload_decryptor.h"

// #include <base/logging.h>

// namespace chromeos_update_engine {

// const char* keyname[KEY_MAX] = {
//   "S011",
//   "A002",
//   "S801",
//   "A802"
// };

// PayloadDecryptor *PayloadDecryptor::pd_ = NULL;

// PayloadDecryptor::PayloadDecryptor() {
//   hsm_ = new nutshell::HsmProxy();
//   // Go to hell, will cased crash
//   // if (hsm_ != NULL) {
//   //   hsm_->registerListener(this);
//   // }
// }

// PayloadDecryptor::~PayloadDecryptor() {
//   // hsm_->unregisterListener();
// }

// void PayloadDecryptor::creat() {
//   if (pd_ == NULL) {
//     pd_ = new PayloadDecryptor();
//   }
// }

// void PayloadDecryptor::destroy() {
//   delete pd_;
//   pd_ = NULL;
// }

// void PayloadDecryptor::onConnectChanged(bool isconn) {
//   LOG(INFO) << "onConnectChanged:" << isconn;
// }
// void PayloadDecryptor::onNotifyPanic() { LOG(INFO) << "onNotifyPanic"; }

// bool PayloadDecryptor::decrypt(const KeyName keyid, const std::string &ciphertext,
//                                std::string &plaintext) {
//   if (IHsmManagerResult::IHsmManagerResult_OK == hsm_->decrypt(keyname[keyid], ciphertext, plaintext)){
//     return true;
//   }
//   return false;
// }

// bool PayloadDecryptor::verifySignature(const KeyName keyid,
//                                        const std::string &data,
//                                        const std::string &signature) {
//   if (IHsmManagerResult::IHsmManagerResult_OK == hsm_->verifySignature(keyname[keyid], data, signature)) {
//     return true;
//   }
//   return false;
// }
// }

// #endif // #ifdef UPDATE_ENGINE_FOR_CAR
