#pragma once
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <variant>
#include <ArduinoJson.h>
#include <atomic>

// содержит индефикатор типа и его id
using TargetRefID = uint32_t;
using timeMS = uint64_t;
using ScheduledIntentID = uint16_t;
template <typename T>
inline uint8_t to_u8(T v)
{
    return static_cast<uint8_t>(v);
}

template <typename T>
inline bool enum_from_u8(uint8_t v, T &out, uint8_t max)
{
    if (v > max)
        return false;

    out = static_cast<T>(v);
    return true;
}
// текущий статус намериния, его состояние
enum class IntentState : uint8_t
{
    // CREATED, // намерение создано , но еще не обрабатывалось

    ACTIVE, // намерение активно и ожидает выполнения

    RUNNING, // выполнить

    PAUSED, // выполнение временно приостановлено

    STOP, // намерение полностью отключено системой/логикой

    DONE, // намерение успешно выполнено

    FAILED, // выполнение завершилось ошибкой

    TO_DELETE // готово для удаления
};
inline const char *toString(IntentState v);

enum class ExecuteResult : uint8_t
{

    NONE,

    SUCCESS, // выполнили

    SUCCESS_OVERRIDE_EQUAL_PRIORITY, // заменил intent равного приоритета

    SUCCESS_OVERRIDE_LOWER_PRIORITY, // заменил intent более низкого приоритета

    BLOCKED_BY_HIGHER_PRIORITY, // заблокирован более сильным intent

    INVALID_PAYLOAD, // payload не соответствует ActionType

    DRIVER_NOT_FOUND, // отсутствует драйвер

    UNSUPPORTED_ACTION, // действие не поддерживается

    FAIL_CAST,

    ERROR_LIFE,

    ERROR_TARGET

};
inline const char *toString(ExecuteResult v);
enum class IntentCommand : uint8_t
{

    NONE,

    ACTIVE, // намерение активно и ожидает выполнения

    PAUSED, // выполнение временно приостановлено

    TO_DELETE // готово для удаления

};
inline const char *toString(IntentCommand v);
enum class IntentFailArbitrator : uint8_t
{
    NONE,

    // время жизни intent истекло
    TIME_EXPIRED_BEFORE,

    // intent уже пытался выполняться , но время жизни intent истекло
    TIME_EXPIRED_AFTER_ATTEMPTS,

    // не выполнен из-за блокировки другим intent (конфликт приоритетов)
    BLOCKED_BY_OTHER_INTENT,

    // заменил intent равного приоритета
    OVERRIDE_EQUAL_PRIORITY,

    // отложен арбитром
    DEFERRED_BY_ARBITRATOR,

    // действие не поддерживается
    UNSUPPORTED_ACTION

};
inline const char *toString(IntentFailArbitrator v);
// информация - что конкретно мы должны сделать
enum class ActionType : uint8_t
{
    ON,     // включить пин/устройство
    OFF,    // выключить пин/устройство
    TOGGLE, // инвертировать состояние
    FADE,   // плавно изменить яркость за время

    CONNECT,
    DISCONNECT,

    PUSH,
    ERASE,

    DISABLE_TOGGLE,
    DISABLE_FADE,
    ENABLE_TOGGLE,
    ENABLE_FADE
};
inline const char *toString(ActionType v);
// жизненный цикл события
enum class LifetimeType : uint8_t
{
    ONCE_TRY, // ровно одна попытка исполнения, независимо от результата
    ONESHOT,  // событие выполняется один раз по времени
    REPEAT,   // событие выполняется каждый день/по расписанию
    UNENDING  // бесконечное
};
inline const char *toString(LifetimeType v);
/*
информация о том , кто должен исполнить событие.
если событие применяется к  событию,
то оно должно  исполнятся всегда
*/
enum class TargetType : uint8_t
{
    PIN,    // физический исполнитель
    DEVICE, // источник событий (железо)
    INTENT  // намериние ScheduledIntent
};
inline const char *toString(TargetType v);
// настройки плавного увеличения яркости
struct FadePayload
{
    uint8_t from{};        // стартовая яркость
    uint8_t to{};          // конечная яркость
    uint32_t durationMs{}; // время перехода
    void fill_json(JsonObject &obj) const;
    bool fill_from_json(const JsonObject &obj);
};
// создать коннект
struct ConnectPayload
{
    uint32_t obj{}; // ID
    void fill_json(JsonObject &obj) const;
};

/*
извлечение данных из TargetRefID
получить  конкретный тип TargetType getType()
получить его id getId()
*/
struct TargetRef
{
    static uint32_t make(TargetType type, uint16_t id);
    static TargetType getType(const TargetRefID &value);
    static uint16_t getId(const TargetRefID &value);
};
// кто создал намериние
enum class IntentSource : uint8_t
{
    IntentDEFAULT,
    SENSOR,   // создал датчик
    SCENARIO, // создал сценарий
    USER,     // создал пользователь
    SYSTEM    // системное намерение
};
inline const char *toString(IntentSource v);
// уровень важности события
enum class IntentUrgency : uint8_t
{
    NORMAL,
    INCREASED,
    CRITICAL,
    EMERGENCY
};
inline const char *toString(IntentUrgency v);
/*
содержит необходимые данные для выполнения действия
DimPayload содержет целевую яркость
FadePayload содержит данные о плавности набора яркости
*/
using IntentPayload = std::variant<
    std::monostate, // пустой payload (для действий без параметров: ON / OFF / ENABLE / DISABLE)

    FadePayload, // плавное изменение значения (from → to за duration)

    ConnectPayload // создание коннекта
    >;
// структура намериний, что мы хотим
struct Intent
{
    // от кого хотим, содеражит в себе данные о типе и его id
    // TargetRef::
    TargetRefID targetID{};
    ActionType type{}; // что сделать

    IntentPayload payload{}; // данные необходимые для выполнения
    void fill_json(JsonObject &obj) const;
    bool fill_from_json(const JsonObject &obj);
};
// время когда выполнять намериние
struct Schedule
{
    timeMS startTime{};
    timeMS endTime{};
    bool operator==(const Schedule &other) const;
    void fill_json(JsonObject &obj) const;
    bool fill_from_json(const JsonObject &obj);
};
// IntentFailArbitrator заполняется только если Arbitrator не пропустил
// blockingIntentID — всегда источник внешнего конфликта
// ExecuteResult — всегда итог действия устройства, даже если оно частичное
struct ExecuteMeta
{

    ExecuteResult rezult{};
    ScheduledIntentID blockingIntentIDResult{};

    IntentCommand command;
    ScheduledIntentID initiatorID{};

    IntentFailArbitrator reason{};
    ScheduledIntentID blockingIntentIDArbitrator{};

    bool operator==(const ExecuteMeta &other) const;
    bool operator!=(const ExecuteMeta &other) const;
    void fill_json(JsonObject &obj) const;
};

struct ScheduledIntent
{
    Intent intent{};         // что сделать
    Schedule schedule{};     // когда выполнять
    LifetimeType life{};     // одноразовое или повторяемое
    IntentSource source{};   // кто создал
    IntentState state{};     // текущее состояние
    IntentUrgency urgency{}; // насколько важно
    timeMS createdAt{};      // когда создано
    timeMS executedAt{};     // когда выполнено
    timeMS updatedAt{};      // последнее изменение

    ScheduledIntentID id{};
    ExecuteMeta rezult{};
    void printF() const;
    void fill_json(JsonArray &arr) const;
    bool fill_from_json(const JsonObject &obj);
    uint32_t version{1};
};

// отвечает за добавление и удаление намериний
// Методы управления состоянием Intent.
// Предназначены только для использования Arbitrator.
class ScheduledIntentStore
{
    friend class Arbitrator;

public:
    ScheduledIntentStore();

    // сорируется при вставке
    // если priority одинаковый
    // более новый intent выигрывает
    ScheduledIntentID add(ScheduledIntent &intent); // добавить намерение
    void update();
    // Проверяет, является ли состояние финальным.
    // Финальное состояние означает, что intent больше не может изменяться
    // и не участвует в дальнейшей обработке жизненного цикла (кроме удаления).
    bool isFinalState(IntentState state) const;
    // продлевает время жизни
    bool extend(const ScheduledIntentID &id, timeMS time);
    std::unordered_set<ScheduledIntentID> get_running() const;
    // получает приоритет события, на основе кто создал и уровня важности события
    uint8_t resolvePriority(const IntentSource &source, const IntentUrgency &urgency) const;
    uint32_t get_version(ScheduledIntentID id) const;
    uint32_t get_version() const;

    std::vector<ScheduledIntentID> get_list_id() const;

    bool setStateExecutor(ScheduledIntentID id, ExecuteResult res, ScheduledIntentID blockingIntentID = 0);

    bool setIntentCommand(ScheduledIntentID id, IntentCommand res, ScheduledIntentID initiatorID);
    // может вернуть  std::nullopt;
    std::optional<ScheduledIntent> get(ScheduledIntentID id) const;
    void fill_json(JsonArray &arr) const;
    size_t size() const;

    void printI();

protected:
    // Методы управления состоянием Intent.
    // Предназначены только для использования Arbitrator.

    // обновить статус намериния
    bool setState(ScheduledIntentID id, IntentState state, ExecuteMeta rezult);

    // переключает на следущие сутки
    bool moveToNextDay(const ScheduledIntentID &id);
    // цель и отсортированный ( по Priority от min -> max ) вектор ID
    const std::unordered_map<TargetRefID, std::vector<ScheduledIntentID>> all() const;

private:
    std::unordered_map<ScheduledIntentID, ScheduledIntent> store{};

    /*
    ключ - кто цель исполнения
    vector id отсортирован по Priority
    std::vector<ScheduledIntentID> - что для него будет сделано
    */
    std::unordered_map<TargetRefID, std::vector<ScheduledIntentID>> scheduler{};

    std::unordered_set<ScheduledIntentID> running{};

    ScheduledIntentID nextId{1};
    uint32_t version{1};
};
