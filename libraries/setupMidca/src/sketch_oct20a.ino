#include <WiFi.h>

const char* ssid = "Asterix";      // твоя сеть
const char* password = "09090909"; // твой пароль

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("\n=== БЕЗОПАСНОЕ ПОДКЛЮЧЕНИЕ Wi-Fi ===");

  // 1️⃣ Проверяем, подключен ли Wi-Fi
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Wi-Fi уже подключен!");
    Serial.print("IP: "); Serial.println(WiFi.localIP());
    return;
  }

  // 2️⃣ Разрываем старое соединение
  WiFi.disconnect(true);
  delay(200);

  // 3️⃣ Настраиваем режим клиента
  WiFi.mode(WIFI_STA);

  // 4️⃣ Начинаем подключение
  WiFi.begin(ssid, password);
  Serial.printf("Подключение к сети: %s\n", ssid);

  // 5️⃣ Таймаут ожидания подключения
  unsigned long start = millis();
  const unsigned long timeout = 20000; // 20 сек
  while (WiFi.status() != WL_CONNECTED && millis() - start < timeout) {
    Serial.print(".");
    delay(500);
  }

  // 6️⃣ Результат
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n✅ Wi-Fi подключен!");
    Serial.print("IP: "); Serial.println(WiFi.localIP());
  } else {
    Serial.println("\n❌ Не удалось подключиться к Wi-Fi");
  }
}

void loop() {
  // Проверяем статус каждые 5 секунд
  static unsigned long lastCheck = 0;
  if (millis() - lastCheck > 5000) {
    lastCheck = millis();
    Serial.print("Статус Wi-Fi: "); Serial.println(WiFi.status());
  }
}
