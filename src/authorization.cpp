#include "authorization.h"
#include <globals.h>

Authorization::Authorization(WiFiClient &client, CLOCK &myclock) : client(client), myclock{myclock}
{
}

bool Authorization::authorize()
{

    // Ждем ответ сервера на запрос авторизации
    if (requestAuthorization && client.available())
    {
        Serial.println("[Authorization] Клиент выслал данные.");
        connect = authorization();
    }

    // Авторизация успешна
    if (connect && client.connected())
    {
        requestAuthorization = false;
        return true;
    }

    // Пока не истек таймаут ожидания ответа
    if (millis() - authRetryTimer < authPause)
    {
        return false;
    }

    authRetryTimer = millis();
    // WiFi обязателен
    if (WiFi.status() != WL_CONNECTED)
    {
        Serial.println("❌ Wi-Fi не подключен!");
        return false;
    }
    if (!myclock)
    {
        Serial.println("❌ Ошибка синхронизации времени!");
        return false;
    }

    client.stop();
    requestAuthorization = false;
    connect = false;
    // Повторная попытка подключения
    if (client.connect(adr.c_str(), port))
    {
        Serial.println("✅ Соединение с сервером установлено");

        client.print(bodyAuthorization());
        client.flush();
        requestAuthorization = true;
    }
    else
    {
        Serial.println("[ERR]❌ Не удалось подключиться к серверу");
    }
    return false;
}

void Authorization::reset()
{
    client.stop();
    requestAuthorization = false;
    connect = false;
    authRetryTimer = 0;
}

void Authorization::set_adr(String adr)
{
    this->adr = adr;
}

void Authorization::set_port(uint16_t port)
{
    this->port - port;
}

String Authorization::bodyAuthorization()
{
    Encryption enc{};
    Serial.println("[LOG] Отправка авторизации серверу");
    String body = Skeleton::commands[Skeleton::idESP];
    // TODO может возникнуть проблема, если размер ID < 10 или > 99
    // переделать под 1 байт который указывает размер ID 0-256 символов
    // или дописать логику закрытия данных через скелетон
    int sizeID = Skeleton::id.length();
    body += String((int)sizeID);
    Serial.print("[LOG] 🔗 idEsp-> ");
    Serial.println(Skeleton::id);
    body += Skeleton::id;

    unsigned long long epoch = myclock.getEpoch_hash();

    String hash = enc.get_hash(epoch, Skeleton::id, enc.get_key());
    body += hash;
    return body;
}

char *Authorization::read_buffer(size_t &buffer_size)
{
    size_t maxSize = 4800;
    buffer_size = 0;
    char *buffer = new char[maxSize + 1];
    unsigned long start = millis();
    const unsigned long timeout = 100; // максимум 0.5 сек

    while (millis() - start < timeout && buffer_size < maxSize)
    {
        while (client.available() > 0 && buffer_size < maxSize)
        {
            buffer[buffer_size++] = client.read();
            start = millis();
        }
        yield();
    }
    // Если клиент всё ещё шлёт данные — превышен лимит
    if (client.available())
    {
        // очистка входного буфера
        while (client.available())
        {
            client.read();
            yield();
        }
        buffer_size = 0;
        delete[] buffer;
        return nullptr;
    }
    buffer[buffer_size] = '\0';
    return buffer;
}

bool Authorization::authorization()
{
    size_t size{};
    char *data = read_buffer(size);
    if (!data || size == 0)
        return false;
    String packet(data, size);
    delete[] data;
    if (packet == Skeleton::commands[Skeleton::accepted])
    {

        Serial.println("[LOG]✅ Авторизация прошла успешно");
        return true;
    }
    else
    {
        Serial.println("[ERR]❌ Ошибка авторизации c сервером!");
        return false;
    }
}
