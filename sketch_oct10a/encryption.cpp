#include "encryption.h"
#include "mbedtls/sha256.h"
#include <sstream>
#include <iomanip>
#include <Arduino.h>
#include "mbedtls/gcm.h"

Encryption::Encryption() {
}

String Encryption::get_hash(unsigned long long time, const String &secret, const String &id) const {

  std::string input;
  if (time == 0) {
    input = std::string(id.c_str()) + std::string(secret.c_str());
  } else {
    input = std::string(id.c_str())
            + std::to_string(time)
            + password
            + std::string(secret.c_str());
  }
  unsigned char hash[32];
  mbedtls_sha256_context ctx;
  mbedtls_sha256_init(&ctx);
  mbedtls_sha256_starts(&ctx, 0);  // 0 = SHA-256
  mbedtls_sha256_update(&ctx, (const unsigned char *)input.c_str(), input.length());
  mbedtls_sha256_finish(&ctx, hash);
  mbedtls_sha256_free(&ctx);

  std::stringstream ss;
  for (int i = 0; i < 32; i++)
    ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];

  return String(ss.str().c_str());
}

String Encryption::encrypt(const String &plaintext) const {
  String key;
  String secretBytes = String(password.c_str()) + get_secret();
  key = get_hash(0, secretBytes, "");
  const size_t iv_len = 12;  // длина IV
  unsigned char iv[iv_len];
  esp_fill_random(iv, iv_len);  // генерируем случайный IV

  const size_t tag_len = 16;
  unsigned char tag[tag_len];
  unsigned char output[512];  // буфер под шифртекст (увеличи при необходимости)

  mbedtls_gcm_context ctx;
  mbedtls_gcm_init(&ctx);
  // инициализация AES-256-GCM
  mbedtls_gcm_setkey(&ctx, MBEDTLS_CIPHER_ID_AES,
                     (const unsigned char *)key.c_str(), 256);

  // шифрование
  mbedtls_gcm_crypt_and_tag(&ctx, MBEDTLS_GCM_ENCRYPT,
                            plaintext.length(),
                            iv, iv_len,
                            nullptr, 0,
                            (const unsigned char *)plaintext.c_str(),
                            output,
                            tag_len, tag);

  mbedtls_gcm_free(&ctx);

  // собираем пакет: [IV][TAG][CIPHERTEXT]
  String packet;
  packet.reserve(iv_len + tag_len + plaintext.length());
  packet += String((char *)iv, iv_len);
  packet += String((char *)tag, tag_len);
  packet += String((char *)output, plaintext.length());

  return packet;
}
String Encryption::decrypt(const String &packet) const {

  String key;
  String secretBytes = String(password.c_str()) + get_secret();
  key = get_hash(0, secretBytes, "");


  const size_t iv_len = 12;
  const size_t tag_len = 16;

  if (packet.length() < iv_len + tag_len) return "";

  const unsigned char *iv = (const unsigned char *)packet.c_str();
  const unsigned char *tag = (const unsigned char *)packet.c_str() + iv_len;
  const unsigned char *ct = (const unsigned char *)packet.c_str() + iv_len + tag_len;
  const size_t ct_len = packet.length() - iv_len - tag_len;

  unsigned char *output = (unsigned char *)malloc(ct_len + 1);
  if (!output) return "";

  mbedtls_gcm_context ctx;
  mbedtls_gcm_init(&ctx);

  // устанавливаем ключ AES-256
  mbedtls_gcm_setkey(&ctx, MBEDTLS_CIPHER_ID_AES,
                     (const unsigned char *)key.c_str(), 256);

  // расшифровка с проверкой тега
  int ret = mbedtls_gcm_auth_decrypt(&ctx,
                                     ct_len,
                                     iv, iv_len,
                                     nullptr, 0,
                                     tag, tag_len,
                                     ct,
                                     output);

  mbedtls_gcm_free(&ctx);

  if (ret != 0) {
    Serial.println("[ERR] decrypt failed!");
    free(output);
    return "";
  }

  String plaintext = String((char *)output, ct_len);
  free(output);
  return plaintext;
}