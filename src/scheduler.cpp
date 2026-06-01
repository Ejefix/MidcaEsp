
#include "scheduler.h"
#include "globals.h"

Arbitrator::Arbitrator(ScheduledIntentStore &s) : store(s)
{
}

void Arbitrator::begin()
{

    const auto &all = store.all();
    time = myclock.getEpochMillis();
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

    if (!target)
    {
        return;
    }
    auto resolvePriority_target = store.resolvePriority(target->source, target->urgency);

    const ScheduledIntent *executor{nullptr};
    for (auto it_vec = vec.begin(); it_vec != vec.end(); ++it_vec)
    {
        const ScheduledIntent *candidate = store.get(*it_vec);
        if (!candidate)
            continue;
        if (candidate->state != IntentState::ACTIVE)
        {
            continue;
        }
        if (executor)
        {
            store.setState(candidate->id, IntentState::PAUSED);
            continue;
        }
        if (candidate->schedule.startTime != candidate->schedule.endTime && candidate->schedule.endTime < time)
        {
            store.setState(candidate->id, IntentState::STOP);
        }
        auto resolvePriority_executor = store.resolvePriority(candidate->source, candidate->urgency);
        if (resolvePriority_executor >= resolvePriority_target)
        {
            if (candidate->schedule.startTime == candidate->schedule.endTime || (time >= candidate->schedule.startTime && candidate->schedule.endTime >= time))
                executor = candidate;
        }
        else
        {
            store.setState(candidate->id, IntentState::PAUSED);
        }
    }
    if (executor)
    {
        store.setState(executor->id, IntentState::RUNNING);
    }
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
            candidate->state == IntentState::RUNNING_ACTIVE;
        switch (candidate->intent.type)
        {
        case ActionType::ON:
        case ActionType::OFF:
        case ActionType::TOGGLE:

            if (!candidatefirst)
            {
                if (isExecutable)
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
                    Rezult rezult{};
                    rezult.rezult = ExecuteResult::OVERRIDE_EQUAL_PRIORITY;
                    rezult.blockingIntentID = candidatefirst->id;
                    store.setState(candidate->id, IntentState::STOP, rezult);
                }
            }
            break;

        case ActionType::FADE:
            if (!candidatesecond)
            {
                if (isExecutable)
                    candidatesecond = candidate;
                continue;
            }
            else
            {

                if (candidatesecond->source == IntentSource::USER 
                    && candidate->source == IntentSource::USER 
                    && candidatesecond->urgency == candidate->urgency   )
                {
                    Rezult rezult{};
                    rezult.rezult = ExecuteResult::OVERRIDE_EQUAL_PRIORITY;
                    rezult.blockingIntentID = candidatesecond->id;
                    store.setState(candidate->id, IntentState::STOP, rezult);
                }
            }
            break;

        default:
            // никакое другое намериние не может приментся тут
            // если оно тут, то это ошибка
            Rezult rezult{};
            rezult.rezult = ExecuteResult::UNSUPPORTED_ACTION;
            store.setState(candidate->id, IntentState::FAILED, rezult);
            break;
        }
    }
    if (candidatefirst && candidatefirst->state == IntentState::ACTIVE)
    {
        store.setState(candidatefirst->id, IntentState::RUNNING);
    }
    if (candidatesecond && candidatesecond->state == IntentState::ACTIVE)
    {
        store.setState(candidatesecond->id, IntentState::RUNNING);
    }
}
