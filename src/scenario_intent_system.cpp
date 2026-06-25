#include "scenario_intent_system.h"
#include <algorithm>
#include "globals.h"
const uint64_t one_day = 24ULL * 60ULL * 60ULL * 1000ULL;

ScheduledIntentStore::ScheduledIntentStore()
{
    store.reserve(100);
    running.reserve(50);
    scheduler.reserve(50);
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
    ++intent.version;
    ++version;
    mutex_store.lock();
    std::lock_guard<std::mutex> lock(mutex_scheduler);

    store[intent.id] = intent;
   // Serial.print("[ScheduledIntentStore::add] Намериние ScheduledIntentID ");
   // Serial.print(intent.id);
   // Serial.println(" добавлено в магазин");
    
    auto &vec = scheduler[intent.intent.targetID];
    if (vec.capacity() - vec.size() < 10)
        vec.reserve(vec.capacity() + 20);
    vec.push_back(intent.id);
    mutex_store.unlock();

    
    std::sort(vec.begin(), vec.end(),
              [this](const ScheduledIntentID &a, const ScheduledIntentID &b)
              {
                  auto first = get(a);
                  auto second = get(b);
                  if (!first || !second)
                  {
                      Serial.println("[ERR]❌❌❌ Ошибка сортировки! Не смогли получить интент. ❌❌❌");
                      return false;
                  }

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
    
   // Serial.println("[ScheduledIntentStore::add] Отсортировано ");
    return intent.id;
}

void ScheduledIntentStore::update()
{
    bool changed = false;
    std::lock_guard<std::mutex> lock(mutex_store);
    for (auto it = store.begin(); it != store.end();)
    {
        auto &intent = it->second;
        auto now = myclock.getEpochMillis();
        if (intent.state != IntentState::TO_DELETE && isFinalState(intent.state) && now - intent.updatedAt > 20000)
        {
            intent.state = IntentState::TO_DELETE;
            intent.updatedAt = now;
            intent.version++;
            changed = true;
        }
        if (intent.state == IntentState::TO_DELETE && now - intent.updatedAt > 10000)
        {
            Serial.print("[ScheduledIntentStore::update] Намериние ScheduledIntentID ");
            Serial.print(intent.id);
            Serial.println(" удалено");
            it = store.erase(it);
            changed = true;
            continue;
        }
        ++it;
    }
    if (changed)
    {
        ++version;
    }
}

bool ScheduledIntentStore::setState(ScheduledIntentID id, IntentState state, ExecuteMeta rezult)
{
    auto it = store.find(id);

    if (it != store.end())
    {
        if (isFinalState(it->second.state))
        {
            /*
            Serial.print("[ScheduledIntentStore::setState] Намериние ScheduledIntentID ");
            Serial.print(id);
            Serial.println(" не верный статус для обновления");
            */
            return false;
        }
        if (it->second.state == state && it->second.rezult == rezult)
        {
            return true;
        }
        it->second.updatedAt = myclock.getEpochMillis();
        it->second.state = state;
        it->second.rezult = rezult;
        it->second.version++;
        ++version;
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
            std::lock_guard<std::mutex> lock(mutex_scheduler);
           
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
        it->second.version++;
        ++version;
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
        it->second.version++;
        ++version;
        while (it->second.schedule.endTime < it->second.updatedAt)
        {
            it->second.schedule.startTime += one_day;
            it->second.schedule.endTime += one_day;
        }
        return true;
    }
    return false;
}

std::optional<ScheduledIntent> ScheduledIntentStore::get(ScheduledIntentID id)
{
    std::lock_guard<std::mutex> lock(mutex_store);
    auto it = store.find(id);

    if (it == store.end())
        return std::nullopt;

    return it->second; // копия
}

const std::unordered_map<TargetRefID, std::vector<ScheduledIntentID>> ScheduledIntentStore::all() 
{
    std::lock_guard<std::mutex> lock(mutex_scheduler);
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

uint32_t ScheduledIntentStore::get_version(ScheduledIntentID id) const
{
    auto it = store.find(id);

    if (it != store.end())
    {
        return it->second.version;
    }
    return {};
}

uint32_t ScheduledIntentStore::get_version() const
{
    return version;
}

std::vector<ScheduledIntentID> ScheduledIntentStore::get_list_id()
{
    std::lock_guard<std::mutex> lock(mutex_store);
    std::vector<ScheduledIntentID> vec{};
    vec.reserve(store.size());
    for (auto it = store.begin(); it != store.end(); ++it)
    {
        vec.emplace_back(it->first);
    }
    return vec;
}

bool ScheduledIntentStore::setStateExecutor(ScheduledIntentID id, ExecuteResult res, ScheduledIntentID blockingIntentID)
{
    auto it = store.find(id);

    if (it != store.end())
    {
        if (isFinalState(it->second.state))
        {
            // Serial.print("[ScheduledIntentStore::setStateExecutor] Намериние ScheduledIntentID ");
            //  Serial.print(id);
            //  Serial.println(" не верный статус для обновления");
            return false;
        }
        it->second.updatedAt = myclock.getEpochMillis();
        if (it->second.rezult.rezult == res && it->second.rezult.blockingIntentIDResult == blockingIntentID)
        {
            return true;
        }
        it->second.rezult.rezult = res;
        it->second.rezult.blockingIntentIDResult = blockingIntentID;
        it->second.executedAt = myclock.getEpochMillis();
        it->second.version++;
        ++version;
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
        if (it->second.rezult.command == res && it->second.rezult.initiatorID == initiatorID)
        {
            return true;
        }
        it->second.updatedAt = myclock.getEpochMillis();
        it->second.rezult.command = res;
        it->second.rezult.initiatorID = initiatorID;
        it->second.version++;
        ++version;
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
    std::lock_guard<std::mutex> lock(mutex_store);
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

void ScheduledIntentStore::fill_json(JsonArray &arr)
{
    std::lock_guard<std::mutex> lock(mutex_store);
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
        obj["life"] = to_u8(life); // тип жизни
    if (to_u8(source) != 0)
        obj["source"] = to_u8(source); // источник
    if (to_u8(state) != 0)
        obj["state"] = to_u8(state); // состояние
    if (to_u8(urgency) != 0)
        obj["urgency"] = to_u8(urgency); // важность
    if (createdAt != 0)
        obj["createdAt"] = createdAt; // время создания
    if (executedAt != 0)
        obj["executedAt"] = executedAt; // время выполнения
    if (updatedAt != 0)
        obj["updatedAt"] = updatedAt; // последнее изменение
    auto scheduleJson = obj["Schedule"].to<JsonObject>();
    schedule.fill_json(scheduleJson);
    auto intentJson = obj["Intent"].to<JsonObject>();
    intent.fill_json(intentJson);
    auto rezultJson = obj["ExecuteMeta"].to<JsonObject>();
    rezult.fill_json(rezultJson);
}

bool ScheduledIntent::fill_from_json(const JsonObject &obj)
{
    if (!obj["id"].isNull() || !obj["executedAt"].isNull() || !obj["updatedAt"].isNull() || !obj["ExecuteMeta"].isNull())
    {
        Serial.println("[ScheduledIntent] Неверный формат в JSON, указаное поле генерируется автоматически и не должно присутствовать в JSON");
        return false;
    }

    if (!obj["life"].isNull())
    {
        uint8_t v = obj["life"].as<uint8_t>();
        if (!enum_from_u8(v, life, to_u8(LifetimeType::UNENDING)))
            return false;
    }
    if (!obj["source"].isNull())
    {
        uint8_t v = obj["source"].as<uint8_t>();
        if (!enum_from_u8(v, source, to_u8(IntentSource::SYSTEM)))
            return false;
    }
    if (!obj["state"].isNull())
    {
        uint8_t v = obj["state"].as<uint8_t>();
        if (!enum_from_u8(v, state, to_u8(IntentState::TO_DELETE)))
            return false;
    }
    if (!obj["urgency"].isNull())
    {
        uint8_t v = obj["urgency"].as<uint8_t>();
        if (!enum_from_u8(v, urgency, to_u8(IntentUrgency::EMERGENCY)))
            return false;
    }
    if (!obj["createdAt"].isNull())
    {
        createdAt = obj["createdAt"].as<timeMS>();
    }

    if (obj["Schedule"].isNull() || obj["Intent"].isNull() || !obj["Schedule"].is<JsonObject>() || !obj["Intent"].is<JsonObject>())
    {
        Serial.println("[ScheduledIntent] Неверный формат в JSON, отсутствует обязательное поле Schedule или Intent");
        return false;
    }

    auto scheduleJson = obj["Schedule"].as<JsonObject>();
    auto intentJson = obj["Intent"].as<JsonObject>();

    return schedule.fill_from_json(scheduleJson) && intent.fill_from_json(intentJson);
}

bool ExecuteMeta::operator==(const ExecuteMeta &other) const
{
    return rezult == other.rezult &&
           blockingIntentIDResult == other.blockingIntentIDResult &&
           command == other.command &&
           initiatorID == other.initiatorID &&
           reason == other.reason &&
           blockingIntentIDArbitrator == other.blockingIntentIDArbitrator;
}

bool ExecuteMeta::operator!=(const ExecuteMeta &other) const
{
    return !(*this == other);
}

void ExecuteMeta::fill_json(JsonObject &obj) const
{
    if (to_u8(rezult) != 0)
        obj["ExecuteResult"] = to_u8(rezult);
    if (blockingIntentIDResult != 0)
        obj["blockingIntentIDResult"] = blockingIntentIDResult;
    if (to_u8(command) != 0)
    {
        obj["IntentCommand"] = to_u8(command);
        obj["initiatorID"] = initiatorID;
    }
    if (to_u8(reason) != 0)
        obj["IntentFailArbitrator"] = to_u8(reason);
    if (blockingIntentIDArbitrator != 0)
        obj["blockingIntentIDArbitrator"] = blockingIntentIDArbitrator;
}

void Intent::fill_json(JsonObject &obj) const
{

    obj["TargetID"] = targetID;
    if (to_u8(type) != 0)
        obj["ActionType"] = to_u8(type);
    if (auto fade = std::get_if<ConnectPayload>(&payload))
    {
        JsonObject arrJ = obj["Connect"].to<JsonObject>();
        fade->fill_json(arrJ);
        return;
    }
    if (auto fade = std::get_if<FadePayload>(&payload))
    {
        JsonObject arrJ = obj["Fade"].to<JsonObject>();
        fade->fill_json(arrJ);
        return;
    }
}

bool Intent::fill_from_json(const JsonObject &obj)
{
    if (obj["TargetID"].isNull())
    {
        return false;
    }
    targetID = obj["TargetID"].as<TargetRefID>();

    if (!obj["ActionType"].isNull())
    {
        uint8_t v = obj["ActionType"].as<uint8_t>();
        if (!enum_from_u8(v, type, to_u8(ActionType::ENABLE_FADE)))
            return false;
    }

    if (obj["Connect"].is<JsonObject>())
    {
        JsonObject c = obj["Connect"].as<JsonObject>();
        if (c["obj"].isNull())
        {
            return false;
        }
        ConnectPayload p;
        p.obj = c["obj"].as<uint32_t>();
        payload = p;
    }
    else if (obj["Fade"].is<JsonObject>())
    {
        JsonObject f = obj["Fade"].as<JsonObject>();
        FadePayload p;
        p.fill_from_json(f);
        payload = p;
    }
    return true;
}

void FadePayload::fill_json(JsonObject &obj) const
{

    if (from != to)
    {
        obj["from"] = from;
        obj["to"] = to;
    }
    if (durationMs != 0)
        obj["time"] = durationMs;
}

bool FadePayload::fill_from_json(const JsonObject &obj)
{
    if (!obj["from"].isNull())
    {
        from = obj["from"].as<uint8_t>();
    }
    if (!obj["to"].isNull())
    {
        to = obj["to"].as<uint8_t>();
    }
    if (!obj["time"].isNull())
    {
        durationMs = obj["time"].as<uint32_t>();
    }
    return true;
}

void ConnectPayload::fill_json(JsonObject &obj) const
{
    obj["obj"] = this->obj;
}
void Schedule::fill_json(JsonObject &obj) const
{
    if (startTime != endTime)
    {
        obj["start"] = startTime;
        obj["end"] = endTime;
    }
}

bool Schedule::fill_from_json(const JsonObject &obj)
{
    if (!obj["start"].isNull())
    {
        startTime = obj["start"].as<timeMS>();
    }
    if (!obj["end"].isNull())
    {
        endTime = obj["end"].as<timeMS>();
    }
    return true;
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
