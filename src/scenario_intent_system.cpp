#include "scenario_intent_system.h"
#include <algorithm>
#include "globals.h"
const uint64_t one_day = 24ULL * 60ULL * 60ULL * 1000ULL;

ScheduledIntentStore::ScheduledIntentStore()
{
    store.reserve(500);
    running.reserve(100);
    scheduler.reserve(40);
}

ScheduledIntentID ScheduledIntentStore::add(ScheduledIntent &intent)
{

    if (intent.schedule.startTime > intent.schedule.endTime || intent.state != IntentState::ACTIVE)
    {
        return {};
    }

    intent.id = nextId;
    ++nextId;
    if (intent.createdAt == 0)
    {
        intent.createdAt = myclock.getEpochMillis();
    }
    ++intent.versionStore;
    store[intent.id] = intent;
    auto &vec = scheduler[intent.intent.targetID];
    if (vec.capacity() - vec.size() < 10)
        vec.reserve(vec.capacity() + 30);
    vec.push_back(intent.id);

    std::sort(vec.begin(), vec.end(),
              [this](const ScheduledIntentID &a, const ScheduledIntentID &b)
              {
                  auto first = get(a);
                  auto second = get(b);

                  if (!first || !second)
                      return false;

                  auto priorityFirst =
                      resolvePriority(first->source, first->urgency);

                  auto prioritySecond =
                      resolvePriority(second->source, second->urgency);

                  // сначала priority
                  if (priorityFirst != prioritySecond)
                  {
                      return priorityFirst > prioritySecond;
                  }

                  // если priority одинаковый
                  // более новый intent выигрывает
                  return first->createdAt > second->createdAt;
              });
    return intent.id;
}

bool ScheduledIntentStore::setState(ScheduledIntentID id, IntentState state, ExecuteMeta rezult)
{
    auto it = store.find(id);

    if (it != store.end())
    {
        if (isFinalState(it->second.state))
        {
            Serial.print("[ScheduledIntentStore::setState] Намериние ScheduledIntentID ");
            Serial.print(id);
            Serial.println(" не верный статус для обновления");
            return false;
        }
        it->second.updatedAt = myclock.getEpochMillis();
        it->second.state = state;
        it->second.rezult = rezult;
        it->second.versionStore++;
        switch (state)
        {
        case IntentState::RUNNING:
            //     Serial.println(" установлен статус RUNNING");
            running.emplace(id);
            break;
        case IntentState::ACTIVE:
            break;
        case IntentState::DONE:
        case IntentState::PAUSED:
        case IntentState::STOP:
        case IntentState::FAILED:
        case IntentState::TO_DELETE:
        default:
        {
            auto intent = get(id);
            if (intent)
            {
                auto it_scheduler = scheduler.find(intent->intent.targetID);
                if (it_scheduler != scheduler.end())
                {
                    auto &vec = it_scheduler->second;
                    for (auto i = vec.begin(); i != vec.end(); ++i)
                    {
                        if (*i == id)
                        {
                            vec.erase(i);
                            break;
                        }
                    }
                }
            }
            running.erase(id);
        }
        }
        return true;
    }

    return false;
}

bool ScheduledIntentStore::extend(const ScheduledIntentID &id, timeMS time)
{
    auto it = store.find(id);
    if (time <= myclock.getEpochMillis())
        return false;
    if (isFinalState(it->second.state))
    {
        Serial.print("[ScheduledIntentStore::extend] Намериние ScheduledIntentID ");
        Serial.print(id);
        Serial.println(" не верный статус для обновления");
        return false;
    }
    if (it != store.end())
    {
        Serial.print("[ScheduledIntentStore] Намериние ScheduledIntentID ");
        Serial.print(id);
        Serial.println(" успешно обновил время дейсвия");
        it->second.updatedAt = myclock.getEpochMillis();
        it->second.schedule.endTime = time;
        return true;
    }
    return false;
}

bool ScheduledIntentStore::moveToNextDay(const ScheduledIntentID &id)
{
    auto it = store.find(id);

    if (it != store.end())
    {
        if (isFinalState(it->second.state))
        {
            Serial.print("[ScheduledIntentStore::moveToNextDay] Намериние ScheduledIntentID ");
            Serial.print(id);
            Serial.println(" не верный статус для обновления");
            return false;
        }
        Serial.print("[ScheduledIntentStore] Намериние ScheduledIntentID ");
        Serial.print(id);
        Serial.println(" успешно обновил время дейсвия");
        it->second.updatedAt = myclock.getEpochMillis();
        it->second.versionStore++;
        while (it->second.schedule.endTime < it->second.updatedAt)
        {
            it->second.schedule.startTime += one_day;
            it->second.schedule.endTime += one_day;
        }
        return true;
    }
    return false;
}

std::optional<ScheduledIntent> ScheduledIntentStore::get(ScheduledIntentID id) const
{
    auto it = store.find(id);

    if (it == store.end())
        return std::nullopt;

    return it->second; // копия
}

const std::unordered_map<TargetRefID, std::vector<ScheduledIntentID>> ScheduledIntentStore::all() const
{
    return scheduler;
}

std::unordered_set<ScheduledIntentID> ScheduledIntentStore::get_running() const
{
    return running;
}

bool ScheduledIntentStore::isFinalState(IntentState state) const
{
    if (state == IntentState::FAILED ||
        state == IntentState::STOP ||
        state == IntentState::DONE ||
        state == IntentState::TO_DELETE)
    {
        return true;
    }
    return false;
}

uint8_t ScheduledIntentStore::resolvePriority(const IntentSource &source, const IntentUrgency &urgency) const
{
    if (urgency == IntentUrgency::EMERGENCY)
        return 200;
    uint16_t score = 0;
    switch (source)
    {
    case IntentSource::IntentDEFAULT:
        return 1;
    case IntentSource::SENSOR:
        score = 15;
        break;
    case IntentSource::SCENARIO:
        score = 30;
        break;
    case IntentSource::USER:
        score = 60;
        break;
    case IntentSource::SYSTEM:
        score = 90;
        break;
    }

    switch (urgency)
    {
    case IntentUrgency::NORMAL:
        score += 0;
        break;
    case IntentUrgency::INCREASED:
        score += 35;
        break;
    case IntentUrgency::CRITICAL:
        score += 50;
        break;
    default:
        break;
    }

    return score;
}

uint32_t ScheduledIntentStore::get_versionStore(ScheduledIntentID &id)
{
    auto it = store.find(id);

    if (it != store.end())
    {
        return it->second.versionStore;
    }
    return {};
}

bool ScheduledIntentStore::setStateExecutor(ScheduledIntentID id, ExecuteResult res, ScheduledIntentID blockingIntentID)
{
    auto it = store.find(id);

    if (it != store.end())
    {
        if (isFinalState(it->second.state))
        {
            Serial.print("[ScheduledIntentStore::setStateExecutor] Намериние ScheduledIntentID ");
            Serial.print(id);
            Serial.println(" не верный статус для обновления");
            return false;
        }
        it->second.updatedAt = myclock.getEpochMillis();
        it->second.rezult.rezult = res;
        it->second.rezult.blockingIntentIDResult = blockingIntentID;
        it->second.executedAt = myclock.getEpochMillis();
        it->second.versionStore++;
        return true;
    }
    return false;
}

bool ScheduledIntentStore::setIntentCommand(ScheduledIntentID id, IntentCommand res, ScheduledIntentID initiatorID)
{
    auto it = store.find(id);

    if (it != store.end())
    {
        if (isFinalState(it->second.state))
        {
            Serial.print("[ScheduledIntentStore::setIntentCommand] Намериние ScheduledIntentID ");
            Serial.print(id);
            Serial.println(" не верный статус для обновления");
            return false;
        }
        it->second.updatedAt = myclock.getEpochMillis();
        it->second.rezult.command = res;
        it->second.rezult.initiatorID = initiatorID;
        it->second.versionStore++;
        return true;
    }
    return false;
}
size_t ScheduledIntentStore::size() const
{
    return store.size();
}

void ScheduledIntentStore::printI()
{
    for (auto it = store.begin(); it != store.end(); ++it)
    {
        it->second.printF();
    }
}

uint32_t TargetRef::make(TargetType type, uint16_t id)
{
    return (uint32_t(type) << 16) | id;
}

TargetType TargetRef::getType(const TargetRefID &value)
{
    return TargetType(value >> 16);
}

uint16_t TargetRef::getId(const TargetRefID &value)
{
    return uint16_t(value & 0xFFFF);
}

bool Schedule::operator==(const Schedule &other) const
{
    return startTime == other.startTime && endTime == other.endTime;
}

void ScheduledIntent::printF() const
{
    Serial.println("===== ScheduledIntent ====="); // начало блока

    Serial.print("ID: ");
    Serial.println(id); // идентификатор intent

    Serial.print("State: ");
    Serial.println(toString(state));

    Serial.print("Source: ");
    Serial.println(toString(source));

    Serial.print("Life: ");
    Serial.println(toString(life));

    Serial.print("CreatedAt: ");
    Serial.println(createdAt); // время создания

    Serial.print("UpdatedAt: ");
    Serial.println(updatedAt); // последнее обновление

    Serial.print("ExecutedAt: ");
    Serial.println(executedAt); // время выполнения

    Serial.print("ExecuteResult: ");
    Serial.println(toString(rezult.rezult));

    Serial.print("blockingIntentID Result: ");
    Serial.println(rezult.blockingIntentIDResult);

    Serial.print("IntentFailArbitrator: ");
    Serial.println(toString(rezult.reason));

    Serial.print("blockingIntentID Arbitrator: ");
    Serial.println(rezult.blockingIntentIDArbitrator); // время выполнения
    Serial.println("===========================");     // конец блока
    delay(200);                                        // пауза 200 мс
}

void ScheduledIntentStore::fill_json(JsonArray &arr) const
{
    for (auto it = store.begin(); it != store.end(); ++it)
    {
        const auto &intent = it->second;
        intent.fill_json(arr);
    }
}
void ScheduledIntent::fill_json(JsonArray &arr) const
{
    JsonObject obj = arr.add<JsonObject>();

    obj["id"] = id; // ID намерения
    if (to_u8(life) != 0)
        obj["li"] = to_u8(life); // тип жизни
    if (to_u8(source) != 0)
        obj["so"] = to_u8(source); // источник
    if (to_u8(state) != 0)
        obj["st"] = to_u8(state); // состояние
    if (to_u8(urgency) != 0)
        obj["ur"] = to_u8(urgency); // важность
    if (createdAt != 0)
        obj["cr"] = createdAt; // время создания
    if (executedAt != 0)
        obj["ex"] = executedAt; // время выполнения
    if (updatedAt != 0)
        obj["up"] = updatedAt; // последнее изменение
    schedule.fill_json(obj);
    intent.fill_json(obj);
    rezult.fill_json(obj);
}

void ExecuteMeta::fill_json(JsonObject &obj) const
{
    if (to_u8(rezult) != 0)
        obj["Res"] = to_u8(rezult);
    if (blockingIntentIDResult != 0)
        obj["IDR"] = blockingIntentIDResult;
    if (to_u8(command) != 0)
    {
        obj["com"] = to_u8(command);
        obj["ini"] = initiatorID;
    }
    if (to_u8(reason) != 0)
        obj["Arb"] = to_u8(reason);
    if (blockingIntentIDArbitrator != 0)
        obj["IDA"] = blockingIntentIDArbitrator;
}

void Intent::fill_json(JsonObject &obj) const
{

    obj["Target"] = targetID;
    if (to_u8(type) != 0)
        obj["Action"] = to_u8(type);
    auto fade = std::get_if<ConnectPayload>(&payload);
    if (fade)
    {
        JsonArray arrJ = obj["Conn"].to<JsonArray>();
        fade->fill_json(arrJ);
        return;
    }
    auto fade_ = std::get_if<FadePayload>(&payload);
    if (fade_)
    {
        JsonArray arrJ = obj["Fade"].to<JsonArray>();
        fade_->fill_json(arrJ);
        return;
    }
}

void FadePayload::fill_json(JsonArray &arr) const
{

    JsonObject obj = arr.add<JsonObject>();
    if (from != to)
    {
        obj["from"] = from;
        obj["to"] = to;
    }
    if (durationMs != 0)
        obj["time"] = durationMs;
}

void ConnectPayload::fill_json(JsonArray &arr) const
{
    JsonObject objJ = arr.add<JsonObject>();
    objJ["obj"] = obj;
}
void Schedule::fill_json(JsonObject &obj) const
{
    if (startTime != endTime)
    {
        obj["start"] = startTime;
        obj["end"] = endTime;
    }
}

inline const char *toString(IntentState v)
{
    switch (v)
    {
    case IntentState::ACTIVE:
        return "ACTIVE";
    case IntentState::RUNNING:
        return "RUNNING";
    case IntentState::PAUSED:
        return "PAUSED";
    case IntentState::STOP:
        return "STOP";
    case IntentState::DONE:
        return "DONE";
    case IntentState::FAILED:
        return "FAILED";
    case IntentState::TO_DELETE:
        return "TO_DELETE";
    default:
        return "UNKNOWN";
    }
}

inline const char *toString(ExecuteResult v)
{
    switch (v)
    {
    case ExecuteResult::NONE:
        return "NONE";
    case ExecuteResult::SUCCESS:
        return "SUCCESS";

    case ExecuteResult::SUCCESS_OVERRIDE_EQUAL_PRIORITY:
        return "SUCCESS_OVERRIDE_EQUAL_PRIORITY";

    case ExecuteResult::SUCCESS_OVERRIDE_LOWER_PRIORITY:
        return "SUCCESS_OVERRIDE_LOWER_PRIORITY";

    case ExecuteResult::BLOCKED_BY_HIGHER_PRIORITY:
        return "BLOCKED_BY_HIGHER_PRIORITY";

    case ExecuteResult::INVALID_PAYLOAD:
        return "INVALID_PAYLOAD";

    case ExecuteResult::DRIVER_NOT_FOUND:
        return "DRIVER_NOT_FOUND";

    case ExecuteResult::UNSUPPORTED_ACTION:
        return "UNSUPPORTED_ACTION";

    case ExecuteResult::FAIL_CAST:
        return "FAIL_CAST";

    case ExecuteResult::ERROR_LIFE:
        return "ERROR_LIFE";

    case ExecuteResult::ERROR_TARGET:
        return "ERROR_TARGET";

    default:
        return "UNKNOWN";
    }
}

inline const char *toString(IntentCommand v)
{
    switch (v)
    {
    case IntentCommand::NONE:
        return "NONE";
    case IntentCommand::ACTIVE:
        return "ACTIVE";
    case IntentCommand::PAUSED:
        return "PAUSED";
    case IntentCommand::TO_DELETE:
        return "TO_DELETE";
    default:
        return "UNKNOWN";
    }
}

inline const char *toString(IntentFailArbitrator v)
{
    switch (v)
    {
    case IntentFailArbitrator::NONE:
        return "NONE";

    case IntentFailArbitrator::TIME_EXPIRED_BEFORE:
        return "TIME_EXPIRED_BEFORE";

    case IntentFailArbitrator::TIME_EXPIRED_AFTER_ATTEMPTS:
        return "TIME_EXPIRED_AFTER_ATTEMPTS";

    case IntentFailArbitrator::BLOCKED_BY_OTHER_INTENT:
        return "BLOCKED_BY_OTHER_INTENT";

    case IntentFailArbitrator::OVERRIDE_EQUAL_PRIORITY:
        return "OVERRIDE_EQUAL_PRIORITY";

    case IntentFailArbitrator::DEFERRED_BY_ARBITRATOR:
        return "DEFERRED_BY_ARBITRATOR";

    case IntentFailArbitrator::UNSUPPORTED_ACTION:
        return "UNSUPPORTED_ACTION";

    default:
        return "UNKNOWN";
    }
}

inline const char *toString(ActionType v)
{
    switch (v)
    {
    case ActionType::ON:
        return "ON";
    case ActionType::OFF:
        return "OFF";
    case ActionType::TOGGLE:
        return "TOGGLE";
    case ActionType::FADE:
        return "FADE";

    case ActionType::CONNECT:
        return "CONNECT";
    case ActionType::DISCONNECT:
        return "DISCONNECT";

    case ActionType::PUSH:
        return "PUSH";
    case ActionType::ERASE:
        return "ERASE";

    case ActionType::DISABLE_TOGGLE:
        return "DISABLE_TOGGLE";
    case ActionType::DISABLE_FADE:
        return "DISABLE_FADE";
    case ActionType::ENABLE_TOGGLE:
        return "ENABLE_TOGGLE";
    case ActionType::ENABLE_FADE:
        return "ENABLE_FADE";

    default:
        return "UNKNOWN";
    }
}

inline const char *toString(LifetimeType v)
{
    switch (v)
    {
    case LifetimeType::ONCE_TRY:
        return "ONCE_TRY";
    case LifetimeType::ONESHOT:
        return "ONESHOT";
    case LifetimeType::REPEAT:
        return "REPEAT";
    case LifetimeType::UNENDING:
        return "UNENDING";
    default:
        return "UNKNOWN";
    }
}

inline const char *toString(TargetType v)
{
    switch (v)
    {
    case TargetType::PIN:
        return "PIN";
    case TargetType::DEVICE:
        return "DEVICE";
    case TargetType::INTENT:
        return "INTENT";
    default:
        return "UNKNOWN";
    }
}

inline const char *toString(IntentSource v)
{
    switch (v)
    {
    case IntentSource::IntentDEFAULT:
        return "DEFAULT";
    case IntentSource::SENSOR:
        return "SENSOR";
    case IntentSource::SCENARIO:
        return "SCENARIO";
    case IntentSource::USER:
        return "USER";
    case IntentSource::SYSTEM:
        return "SYSTEM";
    default:
        return "UNKNOWN";
    }
}

inline const char *toString(IntentUrgency v)
{
    switch (v)
    {
    case IntentUrgency::NORMAL:
        return "NORMAL";
    case IntentUrgency::INCREASED:
        return "INCREASED";
    case IntentUrgency::CRITICAL:
        return "CRITICAL";
    case IntentUrgency::EMERGENCY:
        return "EMERGENCY";
    default:
        return "UNKNOWN";
    }
}
