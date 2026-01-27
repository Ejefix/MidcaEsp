#include "encryption.h"
#include "mbedtls/sha256.h"
#include <sstream>
#include <iomanip>
#include <Arduino.h>
#include "mbedtls/gcm.h"
#include "globals.h"

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

String Encryption::encrypt(const String& plaintext) const
{
 
  // --- ключ ---
  String secret = String(password.c_str()) + get_secret(); // собираем секрет
  String key = get_hash(0, secret, "");                     // получаем AES-ключ

  // --- IV и TAG (маленькие, можно на стеке) ---
  const size_t iv_len = 12;                          // длина IV
  uint8_t iv[iv_len];                                // IV
  esp_fill_random(iv, iv_len);                       // случайный IV

  const size_t tag_len = 16;                         // длина TAG
  uint8_t tag[tag_len];                              // TAG

  // --- CIPHERTEXT в КУЧЕ ---
  size_t data_len = plaintext.length();              // длина данных
  uint8_t* output = new uint8_t[data_len];           // буфер шифртекста (heap!)

  // --- контекст GCM в КУЧЕ ---
  mbedtls_gcm_context* ctx = new mbedtls_gcm_context; // контекст AES-GCM
  mbedtls_gcm_init(ctx);                              // инициализация

  // --- установка ключа ---
  mbedtls_gcm_setkey(
    ctx,
    MBEDTLS_CIPHER_ID_AES,
    (const uint8_t*)key.c_str(),
    256
  );

  // --- шифрование ---
  mbedtls_gcm_crypt_and_tag(
    ctx,
    MBEDTLS_GCM_ENCRYPT,
    data_len,
    iv, iv_len,
    nullptr, 0,
    (const uint8_t*)plaintext.c_str(),
    output,
    tag_len, tag
  );

  mbedtls_gcm_free(ctx);                              // освобождаем GCM
  delete ctx;                                         // удаляем контекст

  // --- сборка пакета ---
  String packet;
  packet.reserve(iv_len + tag_len + data_len);        // резерв памяти
  packet.concat((char*)iv, iv_len);                   // IV
  packet.concat((char*)tag, tag_len);                 // TAG
  packet.concat((char*)output, data_len);             // CIPHERTEXT

  delete[] output;                                    // освобождаем буфер

  return packet;                                      // возвращаем результат
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