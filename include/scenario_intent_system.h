#pragma once
#include <vector>
#include <map>
#include "variant"
enum class IntentState
{
    ACTIVE,   // работает нормально
    PAUSED,   // временно остановлен
    DISABLED, // полностью выключен логикой системы
    DONE      // выполнен (для ONESHOT)
};

enum class ActionType
{
    ON,  // включить пин/устройство
    OFF, // выключить пин/устройство

    DIM,  // установить яркость/мощность
    FADE, // плавно изменить яркость за время

    PUSH_SCENARIO,
    CLEAR_SCENARIO,

    DISABLE, // временно запретить выполнение (блокировка логики)
    ENABLE   // снять запрет, восстановить выполнение правил
};

enum class LifetimeType
{
    ONESHOT, // событие выполняется один раз и удаляется после срабатывания
    REPEAT // событие выполняется каждый день/по расписанию 
};

enum class TargetType
{
    PIN,   // физический исполнитель
    DEVICE // источник событий (железо)
};


struct DimPayload
{
    uint8_t value; // целевая яркость (0–255)
};
struct FadePayload
{
    std::optional<uint8_t> from; // стартовая яркость
    std::optional<uint8_t> to; // конечная яркость
    std::optional<uint32_t> durationMs; // время перехода 
};

struct TargetRef {
    TargetType type{};
    uint16_t id{};
};

using IntentPayload = std::variant<
    std::monostate,   // пустой payload (для действий без параметров: ON / OFF / ENABLE / DISABLE)

    DimPayload,       // установка фиксированной яркости 

    FadePayload      // плавное изменение значения (from → to за duration)
>;
// структура намериний, что мы хотим
struct Intent
{
    TargetRef target{};     // от кого хотим  
    ActionType type{};    // что хотим
    
    IntentPayload payload{}; // действия намериния 
};

// время когда выполнять намериние
struct Schedule
{
    uint64_t startTime;
    uint64_t endTime; 
};

struct ScheduledIntent
{
    Intent intent{};
    Schedule schedule{};
    LifetimeType life{}; // время жизни
    IntentState state{}; // текущий статус
};


// отвечает за добавление и удаление намериний
class IntentStore
{
public:
    void add(Intent &intent); // добавить намерение
    void remove(uint64_t id); // удалить по id
    const std::map<uint64_t, Intent> &IntentStore::all() const;

private:
    std::map<uint64_t, Intent> data;
    uint64_t nextId{1};
};

// обрабатывает намериния пользователя
class Scheduler
{
public:
    void update();

private:
    IntentStore *store{};
};