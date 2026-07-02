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
        if (!arbitrate(candidate))
        {
            continue;
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
                    store.setMetaArbitrator(candidatefirst.id, {});
                    store.setState(candidatefirst.id, IntentState::RUNNING);
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
                    store.setMetaArbitrator(candidatefirst.id, {});
                    store.setState(candidatesecond.id, IntentState::RUNNING);
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
                store.setMetaArbitrator(candidatefirst.id, {});
                store.setState(candidate.id, IntentState::RUNNING);
            }
            break;
        default:
        {
            Serial.println("[Arbiter] UNSUPPORTED ACTION TYPE");
            store.setMetaArbitrator(candidate.id, IntentFailArbitrator::UNSUPPORTED_ACTION);
            store.setState(candidate.id, IntentState::FAILED);
            break;
        }
        }
    }
}
LifecycleResolution Arbitrator::resolve_lifecycle(const ScheduledIntent &candidate) const
{
    auto time = myclock.getEpochMillis();
    const auto &sched = candidate.schedule;
    switch (candidate.life)
    {
    case LifetimeType::ONESHOT:
    case LifetimeType::REPEAT:
    {
        // Ещё не началось
        if (sched.startTime > time)
        {
            return LifecycleResolution::WAIT;
        }
        if (sched.startTime <= time && time < sched.endTime)
        {
            return LifecycleResolution::EXECUTE;
        }

        return LifecycleResolution::EXPIRED;
    }
    case LifetimeType::ONCE_TRY:
    case LifetimeType::UNENDING:
        return LifecycleResolution::EXECUTE;
    default:
    {
        Serial.println("[Arbiter] resolve_lifecycle error");
        return LifecycleResolution::EXPIRED;
    }
    }
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

        store.setMetaArbitrator(loser.id, IntentFailArbitrator::OVERRIDE_EQUAL_PRIORITY, winner.id);
        store.setState(loser.id, IntentState::STOP);
    }
    else
    {
        store.setMetaArbitrator(loser.id, IntentFailArbitrator::DEFERRED_BY_ARBITRATOR, winner.id);
        store.setState(loser.id, IntentState::ACTIVE);
    }
}

bool Arbitrator::approve_before_execution(const ScheduledIntent &candidate, LifecycleResolution answer) const
{
    if (candidate.life != LifetimeType::ONCE_TRY && candidate.life != LifetimeType::UNENDING)
    {
        if (answer == LifecycleResolution::EXPIRED)
        {
            store.setMetaArbitrator(candidate.id, IntentFailArbitrator::TIME_EXPIRED_BEFORE);
            if (candidate.life == LifetimeType::REPEAT)
            {
                store.setState(candidate.id, IntentState::ACTIVE);
                store.moveToNextDay(candidate.id);
            }
            else
            {
                store.setState(candidate.id, IntentState::STOP);
            }

            return false;
        }
    }
    return true;
}

bool Arbitrator::approve_after_execution(const ScheduledIntent &candidate, LifecycleResolution answer) const
{
    if (isExecutionFAILED(candidate))
    {
        store.setState(candidate.id, IntentState::FAILED);
        return false;
    }
    switch (candidate.life)
    {
    case LifetimeType::ONCE_TRY:
        store.setState(candidate.id, IntentState::DONE);
        return false;

    case LifetimeType::ONESHOT:
    {
        if (answer == LifecycleResolution::EXECUTE)
        {
            return true;
        }
        if (answer == LifecycleResolution::EXPIRED)
        {
            store.setState(candidate.id, IntentState::DONE);
            return false;
        }
    }
    case LifetimeType::REPEAT:
    {

        if (answer == LifecycleResolution::EXECUTE)
        {
            return true;
        }
        if (answer == LifecycleResolution::EXPIRED)
        {
           
            store.setMetaArbitrator(candidate.id, IntentFailArbitrator::TIME_EXPIRED_AFTER_ATTEMPTS);
            store.setState(candidate.id, IntentState::ACTIVE);
            store.moveToNextDay(candidate.id);
            return false;
        }
    }
    case LifetimeType::UNENDING:
        return true;
    }
    return true;
}

bool Arbitrator::arbitrate(const ScheduledIntent &candidate) const
{
    if (store.isFinalState(candidate.state))
    {
        return false;
    }
    auto lifecycle = resolve_lifecycle(candidate);

    if (lifecycle == LifecycleResolution::WAIT)
    {
        store.setMetaArbitrator(candidate.id, IntentFailArbitrator::DEFERRED_BY_ARBITRATOR);
        return false;
    }
    if (candidate.rezult.rezult == ExecuteResult::NONE)
    {
        return approve_before_execution(candidate, lifecycle);
    }
    return approve_after_execution(candidate, lifecycle);
}
