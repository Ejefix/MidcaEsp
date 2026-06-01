#include "scenario_intent_system.h"
#include <algorithm>
#include "globals.h"

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

bool ScheduledIntentStore::setState(ScheduledIntentID id, IntentState state, Rezult rezult)
{
    auto it = store.find(id);

    if (it != store.end())
    {
        if (it->second.state == IntentState::FAILED ||
            it->second.state == IntentState::STOP ||
            it->second.state == IntentState::DONE ||
            it->second.state == IntentState::TO_DELETE)
        {
            return false;
        }
        it->second.updatedAt = myclock.getEpochMillis();
        it->second.state = state;
        it->second.rezult = rezult;
        switch (state)
        {
        case IntentState::ACTIVE:
            //     Serial.println(" установлен статус ACTIVE");
            running.erase(id);
            break;
        case IntentState::RUNNING:
            //     Serial.println(" установлен статус RUNNING");
            running.emplace(id);
            break;
        case IntentState::RUNNING_ACTIVE:
            //     Serial.println(" установлен статус RUNNING");
            running.erase(id);
            break;
        case IntentState::PAUSED:
            //    Serial.println(" установлен статус PAUSED");
            running.erase(id);
            break;
        case IntentState::STOP:
            running.erase(id);
            //    Serial.println(" установлен статус STOP");
            break;
        case IntentState::DONE:
        {
            it->second.executedAt = myclock.getEpochMillis();
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
            Serial.println(" установлен статус DONE");
            break;
        }
        case IntentState::FAILED:
            running.erase(id);
            Serial.println(" установлен статус FAILED");
            break;
        case IntentState::TO_DELETE:
            running.erase(id);
            Serial.println(" установлен статус TO_DELETE");
            break;
        default:
            return false;
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
    if (it != store.end())
    {
        switch (it->second.state)
        {
        case IntentState::ACTIVE:
        case IntentState::RUNNING:
        case IntentState::PAUSED:
        case IntentState::RUNNING_ACTIVE:
            if (setState(id, IntentState::ACTIVE)) // запускаем опять в обработку
            {
                Serial.print("[ScheduledIntentStore] Намериние ScheduledIntentID ");
                Serial.print(id);
                Serial.println(" успешно обновил время дейсвия");
                it->second.updatedAt = myclock.getEpochMillis();
                it->second.schedule.endTime = time;
                return true;
            }
            return false;
        default:
            Serial.print("[ScheduledIntentStore] Намериние ScheduledIntentID ");
            Serial.print(id);
            Serial.println(" не верный статус для обновления");
            return false;
        }
    }
    return false;
}

const ScheduledIntent *ScheduledIntentStore::get(ScheduledIntentID id) const
{
    auto it = store.find(id);
    if (it != store.end())
    {
        return &it->second;
    }

    return nullptr;
}

const std::unordered_map<TargetRefID, std::vector<ScheduledIntentID>> &ScheduledIntentStore::all() const
{
    return scheduler;
}

std::unordered_set<ScheduledIntentID> ScheduledIntentStore::get_running() const
{
    return running;
}

void ScheduledIntentStore::push_running(const ScheduledIntentID &id)
{
    running.emplace(id);
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
    for(auto it = store.begin(); it != store.end();++it)
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
    case IntentState::RUNNING_ACTIVE:
        Serial.println("RUNNING_ACTIVE");
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
    case LifetimeType::ONESHOT:
        Serial.println("ONESHOT");
        break;
    case LifetimeType::REPEAT:
        Serial.println("REPEAT");
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

    Serial.print("Rezult: ");

    switch (rezult.rezult)
    {
    case ExecuteResult::SUCCESS:
        Serial.println("SUCCESS");
        break;

    case ExecuteResult::OVERRIDE_EQUAL_PRIORITY:
        Serial.println("заменил intent равного приоритета");
        break;

    case ExecuteResult::OVERRIDE_LOWER_PRIORITY:
        Serial.println("вытеснил более слабый intent");
        break;

    case ExecuteResult::BLOCKED_BY_HIGHER_PRIORITY:
        Serial.println("заблокирован более сильным intent");
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
    Serial.print("blockingIntentID: ");
    Serial.println(rezult.blockingIntentID);       // время выполнения
    Serial.println("==========================="); // конец блока
    delay(1000); // пауза 200 мс
}