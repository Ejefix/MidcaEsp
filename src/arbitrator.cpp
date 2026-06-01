
#include "arbitrator.h"
#include "globals.h"

Arbitrator::Arbitrator(ScheduledIntentStore &s) : store(s)
{
}

void Arbitrator::begin()
{

    const auto &all = store.all();
    auto time = myclock.getEpochMillis();
    for (auto it = all.begin(); it != all.end(); ++it)
    {
        auto &targetID = it->first;
        uint16_t id = TargetRef::getId(targetID);
        TargetType type = TargetRef::getType(targetID);
        auto target = store.get(id);
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

            if (!target)
            {
                break;
            }
            beginINTENTtarget(target, vec);
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
    const ScheduledIntent *candidatefirst{nullptr};
    const ScheduledIntent *candidatesecond{nullptr};
    for (auto it_vec = vec.begin(); it_vec != vec.end(); ++it_vec)
    {
        const ScheduledIntent *candidate = store.get(*it_vec);
        if (!candidate)
            continue;
        const bool isExecutable =
            candidate->state == IntentState::ACTIVE ||
            candidate->state == IntentState::RUNNING ||
            candidate->state == IntentState::RUNNING_ACTIVE ||
            candidate->state == IntentState::RETRY_RUNNING;

        switch (candidate->intent.type)
        {
        case ActionType::ON:
        case ActionType::OFF:
        case ActionType::TOGGLE:

            if (!candidatefirst)
            {
                if (isExecutable && resolve_lifecycle(candidate))

                    candidatefirst = candidate;
                else
                {
                    continue;
                }
            }
            else
            {
                if (candidatefirst->source == IntentSource::USER &&
                    candidate->source == IntentSource::USER &&
                    candidatefirst->urgency == candidate->urgency)
                {
                    ExecuteMeta rezult{};
                    rezult.reason = IntentFailArbitrator::OVERRIDE_EQUAL_PRIORITY;
                    rezult.blockingIntentID = candidatefirst->id;
                    store.setState(candidate->id, IntentState::STOP, rezult);
                }
            }
            break;

        case ActionType::FADE:
            if (!candidatesecond)
            {
                if (isExecutable && resolve_lifecycle(candidate))
                    candidatesecond = candidate;
                continue;
            }
            else
            {

                if (candidatesecond->source == IntentSource::USER &&
                    candidate->source == IntentSource::USER &&
                    candidatesecond->urgency == candidate->urgency)
                {
                    ExecuteMeta rezult{};
                    rezult.reason = IntentFailArbitrator::OVERRIDE_EQUAL_PRIORITY;
                    rezult.blockingIntentID = candidatesecond->id;
                    store.setState(candidate->id, IntentState::STOP, rezult);
                }
            }
            break;

        default:
            // никакое другое намериние не может приментся тут
            // если оно тут, то это ошибка
            ExecuteMeta rezult{};
            rezult.reason = IntentFailArbitrator::UNSUPPORTED_ACTION;
            store.setState(candidate->id, IntentState::FAILED, rezult);
            break;
        }
    }
    if (candidatefirst && candidatefirst->state == IntentState::ACTIVE)
    {
        store.setState(candidatefirst->id, IntentState::RUNNING, candidatefirst->rezult);
    }
    if (candidatesecond && candidatesecond->state == IntentState::ACTIVE)
    {
        store.setState(candidatesecond->id, IntentState::RUNNING, candidatesecond->rezult);
    }
}

bool Arbitrator::resolve_lifecycle(const ScheduledIntent *candidate) const
{
    auto time = myclock.getEpochMillis();
    if (candidate->life == LifetimeType::ONESHOT &&
        (candidate->schedule.startTime == candidate->schedule.endTime ||
         candidate->schedule.startTime >= time && time <= candidate->schedule.endTime))
    {
        return true;
    }
    if (candidate->life == LifetimeType::REPEAT && candidate->schedule.startTime >= time && time <= candidate->schedule.endTime)
    {
        return true;
    }
    switch (candidate->life)
    {
    case LifetimeType::ONESHOT:
        if (candidate->schedule.startTime == candidate->schedule.endTime ||
            candidate->schedule.startTime >= time && time <= candidate->schedule.endTime)
        {
            return true;
        }
        break;
    case LifetimeType::REPEAT:
        if (candidate->schedule.startTime >= time && time <= candidate->schedule.endTime)
        {
            return true;
        }
        break;
    default:
        break;
    }

    if (candidate->state == IntentState::RUNNING_ACTIVE)
    {
        store.setState(candidate->id, IntentState::DONE, candidate->rezult);
    }
    else
    {
        if (candidate->state == IntentState::RETRY_RUNNING)
        {
            ExecuteMeta rezultPIN{};
            rezultPIN = candidate->rezult;
            rezultPIN.reason = IntentFailArbitrator::BLOCKED_BY_OTHER_INTENT;
            store.setState(candidate->id, IntentState::FAILED_TIMEOUT, rezultPIN);
        }
        else
        {
            ExecuteMeta rezultPIN{};
            rezultPIN = candidate->rezult;
            rezultPIN.reason = IntentFailArbitrator::TIME_EXPIRED_BEFORE_FIRST_RUN;
            store.setState(candidate->id, IntentState::FAILED_TIMEOUT, rezultPIN);
        }
    }
    return false;
}
