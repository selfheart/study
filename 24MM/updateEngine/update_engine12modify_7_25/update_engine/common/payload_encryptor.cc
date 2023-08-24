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

// #ifdef UPDATE_ENGINE_FOR_CAR//TODO ??? jzz

#include "update_engine/common/payload_encryptor.h"

#include <base/logging.h>

#define kAES128Len 16
#define kAES192Len 24
#define kAES256Len 32

#define AESDATALEN 16

namespace chromeos_update_engine {

bool PayloadEncryptor::Init(const unsigned char* password,
                            size_t len,
                            enum EncryptoType type,
                            enum EncryptoFun for_decrypto) {  
  if (password == NULL) {
    LOG(ERROR) << "Password is NULL";  // LCOV_EXCL_BR_LINE
    return false;
  }

  if (len==0) {
    LOG(ERROR) << "Password length is zero";  // LCOV_EXCL_BR_LINE
    return false;
  }

  for_decrypto_ = for_decrypto;
  type_ = type;

  size_t upground_size = getAlignSizeByType(type);
  if (upground_size == 0) {
    LOG(ERROR) << "Encrypto type is out of range.";  // LCOV_EXCL_BR_LINE
    return false;
  }

  brillo::Blob password_ground(upground_size);


  memset(password_ground.data(), 0, upground_size);
  memcpy(password_ground.data(), password, len);

  int ret = -1;
  if (for_decrypto == kDecrypto) {
    ret = AES_set_decrypt_key(password_ground.data(), upground_size * 8, &key_);
    return true;
  } else if (for_decrypto == kEncrypto) {
    ret = AES_set_encrypt_key(password_ground.data(), upground_size * 8, &key_);
    return true;
  } else {
    LOG(ERROR) << "Encrypto function is error:" << for_decrypto;  // LCOV_EXCL_BR_LINE
    return false;
  }
}

// Encrypto
bool PayloadEncryptor::Encrypto(const brillo::Blob* input_data,
                                brillo::Blob* output_data) {
  if (for_decrypto_ != kEncrypto) {
    LOG(ERROR) << "Not for encrypto.";  // LCOV_EXCL_BR_LINE
    return false;
  }
  size_t output_size = 0;
  output_size = input_data->size();
  if (output_size == 0) {
    LOG(ERROR) << "Password length is zero";  // LCOV_EXCL_BR_LINE
    return false;
  }
  output_size = (output_size + AESDATALEN - 1) & (0-AESDATALEN);
  brillo::Blob temp_data(output_size);

  memset(temp_data.data(), 0, input_data->size());
  memcpy(temp_data.data(), input_data->data(), input_data->size());

  if (kAES_CBC_256 == type_ || kAES_CBC_128 == type_) {
    unsigned char iv[AES_IV_LEN] = { 0 };
    memcpy(iv, iv_, AES_IV_LEN);
    AES_cbc_encrypt(input_data->data(), temp_data.data(), input_data->size(), &key_, iv, AES_ENCRYPT);
  } else {
    size_t count = output_size / AESDATALEN;
    for (size_t i = 0; i < count; i++) {
      AES_encrypt(input_data->data()+i*AESDATALEN , temp_data.data()+i*AESDATALEN, &key_);
    }
  }

  output_data->swap(temp_data);
  return true;
}

// Decrypto
bool PayloadEncryptor::Decrypto(const brillo::Blob* input_data,
                                brillo::Blob* output_data) {
  if (for_decrypto_ != kDecrypto) {
    LOG(ERROR) << "Not for decrypto.";  // LCOV_EXCL_BR_LINE
    return false;
  }
  size_t output_size = 0;
  output_size = input_data->size();
  if (output_size % AESDATALEN != 0) {
    LOG(ERROR) << "Data size error.";  // LCOV_EXCL_BR_LINE
    return false;
  }

  brillo::Blob temp_data(output_size);
  memset(temp_data.data(), 0, input_data->size());
  memcpy(temp_data.data(), input_data->data(), input_data->size());

  if (kAES_CBC_256 == type_ || kAES_CBC_128 == type_) {
    unsigned char iv[AES_IV_LEN] = { 0 };
    memcpy(iv, iv_, AES_IV_LEN);
    AES_cbc_encrypt(input_data->data(), temp_data.data(), output_size, &key_, iv, AES_DECRYPT);
  } else {
    for (size_t i = 0; i < output_size/AESDATALEN; i++) {
      AES_decrypt(input_data->data()+ i*AESDATALEN, temp_data.data()+i*AESDATALEN, &key_);
    }
  }

  output_data->swap(temp_data);
  return true;
}

// Decrypto
bool PayloadEncryptor::DecryptoIv(const brillo::Blob* input_data,
                                brillo::Blob* output_data) {
  if (for_decrypto_ != kDecrypto) {
    LOG(ERROR) << "Not for decrypto.";  // LCOV_EXCL_BR_LINE
    return false;
  }
  size_t output_size = 0;
  output_size = input_data->size();
  if (output_size % AESDATALEN != 0) {
    LOG(ERROR) << "Data size error.";  // LCOV_EXCL_BR_LINE
    return false;
  }

  brillo::Blob temp_data(output_size);
  memset(temp_data.data(), 0, input_data->size());
  memcpy(temp_data.data(), input_data->data(), input_data->size());

  if (kAES_CBC_256 == type_ || kAES_CBC_128 == type_) {
    AES_cbc_encrypt(input_data->data(), temp_data.data(), output_size, &key_, iv_, AES_DECRYPT);
  } else {
    for (size_t i = 0; i < output_size/AESDATALEN; i++) {
      AES_decrypt(input_data->data()+ i*AESDATALEN, temp_data.data()+i*AESDATALEN, &key_);
    }
  }

  output_data->swap(temp_data);
  return true;
}

bool PayloadEncryptor::SetIV(const unsigned char* iv, size_t iv_len) {
  if (iv == NULL) {
    LOG(ERROR) << "iv is NULL";
    return false;
  }

  if (iv_len < AES_IV_LEN) {
    LOG(ERROR) << "iv_len < AES_IV_LEN";
    return false;
  }
  memcpy(iv_, iv, AES_BLOCK_SIZE);
  return true;
}


size_t PayloadEncryptor::getAlignSizeByType(EncryptoType type) {
  switch (type)
  {
  case kAES_128:
  case kAES_CBC_128:
    return kAES128Len;
  case kAES_192:
    return kAES192Len;
  case kAES_256:
  case kAES_CBC_256:
    return kAES256Len;
  default:
    return 0;
  }
  return 0;
}

}  // namespace chromeos_update_engine

// #endif // #ifdef UPDATE_ENGINE_FOR_CAR//TODO ??? jzz
