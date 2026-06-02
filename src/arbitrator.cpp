#include "arbitrator.h"
#include "globals.h"

Arbitrator::Arbitrator(ScheduledIntentStore &s) : store(s)
{
}

void Arbitrator::begin()
{

    const auto all = store.all();
    auto time = myclock.getEpochMillis();
    for (auto it = all.begin(); it != all.end(); ++it)
    {
        auto &targetID = it->first;
        uint16_t id = TargetRef::getId(targetID);
        TargetType type = TargetRef::getType(targetID);
       
        const auto &vec = it->second;
        switch (type)
        {
        case TargetType::DEVICE:
            /* code */
            break;
        case TargetType::PIN:
            beginPINtarget(vec);
            break;
        case TargetType::INTENT:
            break;
        default:
            break;
        }
    }
}

void Arbitrator::beginINTENTtarget(const ScheduledIntent *target, const std::vector<ScheduledIntentID> &vec) const
{
}
void Arbitrator::beginPINtarget(const std::vector<ScheduledIntentID> &vec) const
{

    ScheduledIntent candidatefirst{};
    bool first{false};
    bool second{false};
    ScheduledIntent candidatesecond{};

    for (auto it_vec = vec.begin(); it_vec != vec.end(); ++it_vec)
    {
        auto snap = store.get(*it_vec);

        if (!snap)
        {
            continue;
        }
        ScheduledIntent candidate = *snap;
        if (store.isFinalState(candidate.state) && !resolve_lifecycle(candidate))
        {
            continue;
        }
        switch (candidate.intent.type)
        {
        case ActionType::ON:
        case ActionType::OFF:
        case ActionType::TOGGLE:

            if (!first)
            {
                first = true;
                candidatefirst = candidate;
                continue;
            }
            else
            {

                if (candidatefirst.source == IntentSource::USER &&
                    candidate.source == IntentSource::USER &&
                    candidatefirst.urgency == candidate.urgency)
                {

                    ExecuteMeta rezult{};
                    rezult.reason = IntentFailArbitrator::OVERRIDE_EQUAL_PRIORITY;
                    rezult.blockingIntentID = candidatefirst.id;
                    store.setState(candidate.id, IntentState::STOP, rezult);
                }
            }
            break;

        case ActionType::FADE:

            if (!second)
            {
                second = true;
                candidatesecond = candidate;
                continue;
            }
            else
            {

                if (candidatesecond.source == IntentSource::USER &&
                    candidate.source == IntentSource::USER &&
                    candidatesecond.urgency == candidate.urgency)
                {

                    ExecuteMeta rezult{};
                    rezult.reason = IntentFailArbitrator::OVERRIDE_EQUAL_PRIORITY;
                    rezult.blockingIntentID = candidatesecond.id;

                    store.setState(candidate.id, IntentState::STOP, rezult);
                }
            }
            break;

        default:
        {
            Serial.println("[Arbiter] UNSUPPORTED ACTION TYPE");
            ExecuteMeta rezult{};
            rezult.reason = IntentFailArbitrator::UNSUPPORTED_ACTION;
            store.setState(candidate.id, IntentState::FAILED, rezult);
            break;
        }
        }
    }

    if (first)
    {

        if (candidatefirst.state == IntentState::ACTIVE)
        {
            store.setState(candidatefirst.id, IntentState::RUNNING, candidatefirst.rezult);
        }
    }

    if (second)
    {
        if (candidatesecond.state == IntentState::ACTIVE)
        {
            store.setState(candidatesecond.id, IntentState::RUNNING, candidatesecond.rezult);
        }
    }
}
bool Arbitrator::resolve_lifecycle(const ScheduledIntent &candidate) const
{
    auto time = myclock.getEpochMillis();
    const auto &sched = candidate.schedule;

    // Проверка расписания (ОДНА логика)
    bool isTimeValid = false;
    if (candidate.life == LifetimeType::ONESHOT)
    {
        isTimeValid = (sched.startTime == sched.endTime) ||
                      (sched.startTime <= time && time <= sched.endTime);
    }
    else // REPEAT
    {
        isTimeValid = (sched.startTime <= time && time <= sched.endTime);
    }
    if (isTimeValid)
        return true;
    if (sched.startTime > time)
    {
        return false;
    }

    IntentState state{};
    if (candidate.life == LifetimeType::REPEAT)
    {
        store.moveToNextDay(candidate.id);
        state = IntentState::PAUSED;
    }
    else
    {
        state = IntentState::DONE;
    }
    ExecuteMeta result{candidate.rezult};

    if (candidate.state == IntentState::RUNNING_ACTIVE)
    {
        store.setState(candidate.id, state, result);
        return false;
    }
    if (candidate.state == IntentState::RETRY_RUNNING)
    {
        result.reason = IntentFailArbitrator::BLOCKED_BY_OTHER_INTENT;
    }
    else
    {
        result.reason = IntentFailArbitrator::TIME_EXPIRED_BEFORE_FIRST_RUN;
    }

    store.setState(candidate.id, state, result);
    return false;
}
