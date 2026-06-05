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
        if (store.isFinalState(candidate.state))
        {
            continue;
        }
        auto answer = resolve_lifecycle(candidate);
        if (candidate.rezult.rezult == ExecuteResult::NONE)
        {
            if (candidate.life != LifetimeType::ONCE_TRY && candidate.life != LifetimeType::UNENDING)
            {
                if (answer == LifecycleResolution::WAIT)
                {
                    continue;
                }
                if (answer == LifecycleResolution::EXPIRED)
                {
                    ExecuteMeta rezult{candidate.rezult};
                    rezult.reason = IntentFailArbitrator::TIME_EXPIRED_BEFORE;
                    store.setState(candidate.id, IntentState::STOP, candidate.rezult);
                    continue;
                }
            }
        }
        else
        {
            switch (candidate.life)
            {
            case LifetimeType::ONCE_TRY:
                store.setState(candidate.id, IntentState::DONE, candidate.rezult);
                continue;

            case LifetimeType::ONESHOT:
                if (answer == LifecycleResolution::EXECUTE)
                {
                    break;
                }
                if (answer == LifecycleResolution::EXPIRED)
                {
                    if (isExecutionFinished(candidate))
                    {
                        store.setState(candidate.id, IntentState::DONE, candidate.rezult);
                    }
                    else
                    {
                        ExecuteMeta rezult{candidate.rezult};
                        rezult.reason = IntentFailArbitrator::TIME_EXPIRED_BEFORE;
                        store.setState(candidate.id, IntentState::STOP, candidate.rezult);
                    }
                    continue;
                }
            case LifetimeType::REPEAT:
                if (answer == LifecycleResolution::EXECUTE)
                {
                    break;
                }
                if (answer == LifecycleResolution::EXPIRED)
                {
                    if (isExecutionFinished(candidate))
                    {
                        store.setState(candidate.id, IntentState::PAUSED, candidate.rezult);
                    }
                    else
                    {
                        ExecuteMeta rezult{candidate.rezult};
                        rezult.reason = IntentFailArbitrator::TIME_EXPIRED_BEFORE;
                        store.setState(candidate.id, IntentState::PAUSED, candidate.rezult);
                    }
                    continue;
                }
            case LifetimeType::UNENDING:
                if (isExecutionFinished(candidate))
                {
                    store.setState(candidate.id, IntentState::ACTIVE, candidate.rezult);
                    continue;
                }
                else
                {
                    break;
                }
            }
        }

        switch (candidate.intent.type)
        {
        case ActionType::ENABLE_TOGGLE:
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
        case ActionType::ENABLE_FADE:
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
LifecycleResolution Arbitrator::resolve_lifecycle(const ScheduledIntent &candidate) const
{
    auto time = myclock.getEpochMillis();
    const auto &sched = candidate.schedule;

    if (time >= sched.startTime && time <= sched.endTime)
    {
        return LifecycleResolution::EXECUTE;
    }

    // Ещё не началось
    if (time < sched.startTime)
    {
        return LifecycleResolution::WAIT;
    }
    return LifecycleResolution::EXPIRED;
}

bool Arbitrator::isExecutionFinished(const ScheduledIntent &candidate) const
{
    switch (candidate.rezult.rezult)
    {
    case ExecuteResult::SUCCESS:
    case ExecuteResult::SUCCESS_OVERRIDE_EQUAL_PRIORITY:
    case ExecuteResult::SUCCESS_OVERRIDE_LOWER_PRIORITY:
        return true;
    case ExecuteResult::NONE:
    case ExecuteResult::BLOCKED_BY_HIGHER_PRIORITY:
        return false;
        break;
    case ExecuteResult::INVALID_PAYLOAD:
    case ExecuteResult::DRIVER_NOT_FOUND:
    case ExecuteResult::UNSUPPORTED_ACTION:
    default:
        return true;
    }
}
