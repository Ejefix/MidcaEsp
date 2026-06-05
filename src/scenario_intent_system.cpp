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

void ScheduledIntentStore::update()
{
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
        switch (state)
        {
        case IntentState::RUNNING:
            //     Serial.println(" установлен статус RUNNING");
            running.emplace(id);
            break;
        case IntentState::ACTIVE:
            break;
        case IntentState::DONE:
            it->second.executedAt = myclock.getEpochMillis();
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

bool ScheduledIntentStore::setStateExecutor(ScheduledIntentID id, ExecuteResult res, ScheduledIntentID blockingIntentID)
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
        it->second.updatedAt = myclock.getEpochMillis();
        it->second.rezult.rezult = res;
        it->second.rezult.blockingIntentID = blockingIntentID;
    }
    return false;
}

size_t ScheduledIntentStore::size() const
{
    return store.size();
}

void ScheduledIntentStore::clear()
{
    store.clear();
    scheduler.clear();
    running.clear();
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

    switch (state)
    {

    case IntentState::ACTIVE:
        Serial.println("ACTIVE");
        break;

    case IntentState::RUNNING:
        Serial.println("RUNNING");
        break;
    case IntentState::PAUSED:
        Serial.println("PAUSED");
        break;
    case IntentState::STOP:
        Serial.println("STOP");
        break;
    case IntentState::DONE:
        Serial.println("DONE");
        break;
    case IntentState::FAILED:
        Serial.println("FAILED");
        break;
    case IntentState::TO_DELETE:
        Serial.println("DELETE");
        break;
    default:

        Serial.println("UNKNOWN");
        break;
    }
    Serial.print("Source: ");

    switch (source)
    {
    case IntentSource::IntentDEFAULT:
        Serial.println("DEFAULT");
        break;
    case IntentSource::SENSOR:
        Serial.println("SENSOR");
        break;
    case IntentSource::SCENARIO:
        Serial.println("SCENARIO");
        break;
    case IntentSource::USER:
        Serial.println("USER");
        break;
    case IntentSource::SYSTEM:
        Serial.println("SYSTEM");
        break;
    default:
        Serial.println("UNKNOWN");
        break;
    }

    Serial.print("Life: ");

    switch (life)
    {
    case LifetimeType::ONCE_TRY:
        Serial.println("ONCE_TRY");
        break;
    case LifetimeType::ONESHOT:
        Serial.println("ONESHOT");
        break;
    case LifetimeType::REPEAT:
        Serial.println("REPEAT");
        break;
    case LifetimeType::UNENDING:
        Serial.println("UNENDING");
        break;
    default:
        Serial.println("UNKNOWN");
        break;
    }
    Serial.print("CreatedAt: ");
    Serial.println(createdAt); // время создания

    Serial.print("UpdatedAt: ");
    Serial.println(updatedAt); // последнее обновление

    Serial.print("ExecutedAt: ");
    Serial.println(executedAt); // время выполнения

    Serial.print("ExecuteResult: ");

    switch (rezult.rezult)
    {
    case ExecuteResult::NONE:
        Serial.println("NONE");
        break;
    case ExecuteResult::SUCCESS:
        Serial.println("SUCCESS");
        break;

    case ExecuteResult::SUCCESS_OVERRIDE_EQUAL_PRIORITY:
        Serial.println("SUCCESS заменил intent равного приоритета");
        break;

    case ExecuteResult::SUCCESS_OVERRIDE_LOWER_PRIORITY:
        Serial.println("SUCCESS вытеснил более слабый intent");
        break;

    case ExecuteResult::BLOCKED_BY_HIGHER_PRIORITY:
        Serial.println("BLOCKED заблокирован более сильным intent");
        break;

    case ExecuteResult::INVALID_PAYLOAD:
        Serial.println("payload не соответствует ActionType");
        break;

    case ExecuteResult::DRIVER_NOT_FOUND:
        Serial.println("отсутствует драйвер");
        break;

    case ExecuteResult::UNSUPPORTED_ACTION:
        Serial.println("действие не поддерживается");
        break;

    default:
        Serial.println("UNKNOWN");
        break;
    }
    Serial.print("IntentFailArbitrator: ");
    switch (rezult.reason)
    {
    case IntentFailArbitrator::NONE:
        Serial.println("NONE");
        break;

    case IntentFailArbitrator::TIME_EXPIRED_BEFORE:
        Serial.println("время жизни intent истекло");
        break;

    case IntentFailArbitrator::TIME_EXPIRED_AFTER_ATTEMPTS:
        Serial.println("intent уже пытался выполняться , но время жизни intent истекло");
        break;

    case IntentFailArbitrator::BLOCKED_BY_OTHER_INTENT:
        Serial.println("не выполнен из-за блокировки другим intent (конфликт приоритетов)");
        break;

    case IntentFailArbitrator::OVERRIDE_EQUAL_PRIORITY:
        Serial.println("заменил intent равного приоритета");
        break;

    case IntentFailArbitrator::UNSUPPORTED_ACTION:
        Serial.println("отсутствует драйвер");
        break;
    default:
        Serial.println("UNKNOWN");
        break;
    }
    Serial.print("blockingIntentID: ");
    Serial.println(rezult.blockingIntentID);       // время выполнения
    Serial.println("==========================="); // конец блока
    delay(1000);                                   // пауза 200 мс
}