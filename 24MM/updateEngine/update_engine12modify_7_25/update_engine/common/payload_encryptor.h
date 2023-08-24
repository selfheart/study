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

#ifndef UPDATE_ENGINE_COMMON_PAYLOAD_ENCRYPTOR_H_
#define UPDATE_ENGINE_COMMON_PAYLOAD_ENCRYPTOR_H_

#include <base/macros.h>
#include <brillo/key_value_store.h>
#include <brillo/secure_blob.h>
#include <openssl/aes.h>

#include <string>
#include <vector>

#include "update_engine/update_metadata.pb.h"

// This class encapsulates methods used for payload signing.
// See update_metadata.proto for more info.

namespace chromeos_update_engine {
#define AES_IV_LEN 16

class PayloadEncryptor {
 public:
  enum EncryptoType {
      kAES_128,
      kAES_192,
      kAES_256,
      kAES_CBC,
      kAES_CBC_256,
      kAES_CBC_128,
      kEncryptoType_invalid
  };

    enum EncryptoFun {
      kInvalid,
      kEncrypto,
      kDecrypto
  };

  // Init
  bool Init(const unsigned char* password, size_t len,
            enum EncryptoType type, enum EncryptoFun fun);

  // Encrypto
  bool Encrypto(const brillo::Blob* input_data,
                brillo::Blob* output_data);

  // Decrypto
  bool Decrypto(const brillo::Blob* input_data,
                brillo::Blob* output_data);
  // Decrypto
  bool DecryptoIv(const brillo::Blob* input_data,
                brillo::Blob* output_data);

  bool SetIV(const unsigned char* iv, size_t iv_len);

 private:
  size_t getAlignSizeByType(EncryptoType type);

  enum EncryptoFun for_decrypto_{kInvalid};
  AES_KEY key_;
  EncryptoType type_{kEncryptoType_invalid};
  unsigned char iv_[AES_IV_LEN] = { 0 };
};

}  // namespace chromeos_update_engine

#endif  // UPDATE_ENGINE_COMMON_PAYLOAD_ENCRYPTOR_H_
