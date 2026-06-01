#pragma once
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <variant>
// содержит индефикатор типа и его id
using TargetRefID = uint32_t;
using timeMS = uint64_t;
using ScheduledIntentID = uint16_t;

// текущий статус намериния, его состояние
enum class IntentState
{
    // CREATED, // намерение создано , но еще не обрабатывалось

    ACTIVE, // намерение активно и ожидает выполнения

    RUNNING, // выполнить

    RUNNING_ACTIVE, // намериние уже выполняется и ждет завершения

    RETRY_RUNNING, // намериние  пробовали запустить

    FAILED_TIMEOUT, // время действия закончилось

    PAUSED, // выполнение временно приостановлено

    STOP, // намерение полностью отключено системой/логикой

    DONE, // намерение успешно выполнено

    FAILED, // выполнение завершилось ошибкой

    TO_DELETE // готово для удаления
};
enum class ExecuteResult : uint8_t
{

    NONE,

    SUCCESS, // выполнили

    OVERRIDE_EQUAL_PRIORITY, // заменён равным приоритетом

    OVERRIDE_LOWER_PRIORITY, // вытеснил более слабый intent

    BLOCKED_BY_HIGHER_PRIORITY, // заблокирован более сильным intent

    INVALID_PAYLOAD, // payload не соответствует ActionType

    DRIVER_NOT_FOUND, // отсутствует драйвер

    UNSUPPORTED_ACTION, // действие не поддерживается

};
enum class IntentFailArbitrator
{
    NONE,

    // время жизни intent истекло до первой попытки выполнения
    TIME_EXPIRED_BEFORE_FIRST_RUN,

    // intent уже пытался выполняться , но время жизни intent истекло
    TIME_EXPIRED_AFTER_ATTEMPTS,

    // не выполнен из-за блокировки другим intent (конфликт приоритетов)
    BLOCKED_BY_OTHER_INTENT,

    // заменил intent равного приоритета
    OVERRIDE_EQUAL_PRIORITY,

    // действие не поддерживается
    UNSUPPORTED_ACTION
};
// информация - что конкретно мы должны сделать
enum class ActionType
{
    ON,        // включить пин/устройство
    OFF,       // выключить пин/устройство
    TOGGLE,    // инвертировать состояние
    FADE,      // плавно изменить яркость за время
    ONandFADE, // включить с яркостью

    DISCONNECT,

    PUSH,
    CLEAR,
    CONNECT,

    DISABLE, // временно запретить выполнение (блокировка логики)
    ENABLE   // снять запрет, восстановить выполнение правил
};
// жизненный цикл события
enum class LifetimeType
{
    ONESHOT, // событие выполняется один раз и удаляется после срабатывания
    REPEAT   // событие выполняется каждый день/по расписанию
};

/*
информация о том , кто должен исполнить событие.
если событие применяется к  событию,
то оно должно  исполнятся всегда,
если цель создана не  IntentSource::SYSTEM
*/
enum class TargetType
{
    PIN,    // физический исполнитель
    DEVICE, // источник событий (железо)
    INTENT  // намериние ScheduledIntent
};

// настройки плавного увеличения яркости
struct FadePayload
{
    uint8_t from{};        // стартовая яркость
    uint8_t to{};          // конечная яркость
    uint32_t durationMs{}; // время перехода
};
// создать коннект
struct ConnectPayload
{
    uint32_t obj{}; // ID
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
enum class IntentSource
{
    IntentDEFAULT,
    SENSOR,   // создал датчик
    SCENARIO, // создал сценарий
    USER,     // создал пользователь
    SYSTEM    // системное намерение
};

// уровень важности события
enum class IntentUrgency
{
    NORMAL,
    INCREASED,
    CRITICAL,
    EMERGENCY
};

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
};
// время когда выполнять намериние
struct Schedule
{
    timeMS startTime{};
    timeMS endTime{};
    bool operator==(const Schedule &other) const;
};
// IntentFailArbitrator заполняется только если Arbitrator не пропустил
// blockingIntentID — всегда источник внешнего конфликта
// ExecuteResult — всегда итог действия устройства, даже если оно частичное
struct ExecuteMeta
{
    ExecuteResult rezult{};
    ScheduledIntentID blockingIntentID{};
    IntentFailArbitrator reason{};
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
};

// отвечает за добавление и удаление намериний
class ScheduledIntentStore
{
public:
    ScheduledIntentStore();

    // сорируется при вставке
    // если priority одинаковый
    // более новый intent выигрывает
    ScheduledIntentID add(ScheduledIntent &intent); // добавить намерение
    void update();                                  // обновить состояние  магазина
    bool setState(ScheduledIntentID id, IntentState state, ExecuteMeta rezult);
    // продлевает время жизни
    bool extend(const ScheduledIntentID &id, timeMS time);
    // может вернуть nullptr
    const ScheduledIntent *get(ScheduledIntentID id) const;
    // цель и отсортированный ( по Priority от min -> max ) вектор ID
    const std::unordered_map<TargetRefID, std::vector<ScheduledIntentID>> &all() const;
    std::unordered_set<ScheduledIntentID> get_running() const;
    // получает приоритет события, на основе кто создал и уровня важности события
    uint8_t resolvePriority(const IntentSource &source, const IntentUrgency &urgency) const;
    size_t size() const;
    void clear();
    void printI();

private:
    std::unordered_map<ScheduledIntentID, ScheduledIntent> store{};

    /*
    ключ - кто цель исполнения
    vector id отсортирован по Priority
    std::vector<ScheduledIntentID> - что для него будет сделано
    */
    std::unordered_map<TargetRefID, std::vector<ScheduledIntentID>> scheduler{};

    std::unordered_set<ScheduledIntentID> running{};
    // вставка события для его выполнения
    void push_running(const ScheduledIntentID &id);

    ScheduledIntentID nextId{1};
};
