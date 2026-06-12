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
        beginTarget(vec);
    }
}

void Arbitrator::beginTarget(const std::vector<ScheduledIntentID> &vec) const
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
                    rezult.reason = IntentFailArbitrator::BLOCKED_BY_OTHER_INTENT;
                    if (candidate.life == LifetimeType::REPEAT)
                    {
                        store.moveToNextDay(candidate.id);
                        store.setState(candidate.id, IntentState::ACTIVE, rezult);
                    }
                    else
                    {
                        store.setState(candidate.id, IntentState::STOP, rezult);
                    }

                    continue;
                }
            }
        }
        else
        {
            if (isExecutionFAILED(candidate))
            {
                store.setState(candidate.id, IntentState::FAILED, candidate.rezult);
                continue;
            }
            switch (candidate.life)
            {
            case LifetimeType::ONCE_TRY:
                store.setState(candidate.id, IntentState::DONE, candidate.rezult);
                continue;

            case LifetimeType::ONESHOT:
            {
                if (answer == LifecycleResolution::EXECUTE)
                {
                    break;
                }
                if (answer == LifecycleResolution::EXPIRED)
                {
                    store.setState(candidate.id, IntentState::DONE, candidate.rezult);
                    continue;
                }
            }
            case LifetimeType::REPEAT:
            {
                if (answer == LifecycleResolution::EXECUTE)
                {
                    break;
                }
                if (answer == LifecycleResolution::EXPIRED)
                {
                    store.moveToNextDay(candidate.id);
                    ExecuteMeta rezult{candidate.rezult};
                    rezult.reason = IntentFailArbitrator::TIME_EXPIRED_AFTER_ATTEMPTS;
                    store.setState(candidate.id, IntentState::ACTIVE, rezult);
                    continue;
                }
            }
            case LifetimeType::UNENDING:
                break;
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
                if (candidatefirst.state == IntentState::ACTIVE)
                {
                    store.setState(candidatefirst.id, IntentState::RUNNING, candidatefirst.rezult);
                }
                continue;
            }
            else
            {
                deferOrOverride(candidatefirst, candidate);
            }
            break;
        case ActionType::ENABLE_FADE:
        case ActionType::FADE:

            if (!second)
            {
                second = true;
                candidatesecond = candidate;
                if (candidatesecond.state == IntentState::ACTIVE)
                {
                    store.setState(candidatesecond.id, IntentState::RUNNING, candidatesecond.rezult);
                }
                continue;
            }
            else
            {
                deferOrOverride(candidatesecond, candidate);
            }
            break;
        case ActionType::CONNECT:
        case ActionType::DISCONNECT:
        case ActionType::ERASE:
            if (candidate.state == IntentState::ACTIVE)
            {
                store.setState(candidate.id, IntentState::RUNNING, candidate.rezult);
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
bool Arbitrator::isExecutionFAILED(const ScheduledIntent &candidate) const
{
    switch (candidate.rezult.rezult)
    {
    case ExecuteResult::SUCCESS:
    case ExecuteResult::SUCCESS_OVERRIDE_EQUAL_PRIORITY:
    case ExecuteResult::SUCCESS_OVERRIDE_LOWER_PRIORITY:
    case ExecuteResult::NONE:
    case ExecuteResult::BLOCKED_BY_HIGHER_PRIORITY:
        return false;
    default:
        return true;
    }
}

void Arbitrator::deferOrOverride(const ScheduledIntent &winner, const ScheduledIntent &loser) const
{
    ExecuteMeta rezult{loser.rezult};
    if (winner.source == IntentSource::USER &&
        loser.source == IntentSource::USER &&
        winner.urgency == loser.urgency)
    {

        rezult.reason = IntentFailArbitrator::OVERRIDE_EQUAL_PRIORITY;
        rezult.blockingIntentIDArbitrator = winner.id;

        store.setState(loser.id, IntentState::STOP, rezult);
    }
    else
    {
        rezult.reason = IntentFailArbitrator::DEFERRED_BY_ARBITRATOR;
        rezult.blockingIntentIDArbitrator = winner.id;
        store.setState(loser.id, IntentState::ACTIVE, rezult);
    }
}
