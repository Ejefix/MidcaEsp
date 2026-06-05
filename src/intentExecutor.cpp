#include "intentExecutor.h"
#include "globals.h"

void IntentExecutor::begin()
{
    auto intents = store->get_running();
    for (auto it = intents.begin(); it != intents.end(); ++it)
    {
        executor(*it);
    }
}

void IntentExecutor::executor(const ScheduledIntentID &id) const
{
    auto snap = store->get(id);

    if (!snap)
    {
        return;
    }
    ScheduledIntent intent = *snap;

    auto time = myclock.getEpochMillis();

    auto typeID = TargetRef::getType(intent.intent.targetID); // кто должен сделать

    switch (typeID)
    {
    case TargetType::PIN:
        executorPIN(intent);

        break;
    case TargetType::DEVICE:
        executorDEVICE(intent);
        break;
    case TargetType::INTENT:
        /* code */
        break;
    default:
        break;
    }
}

void IntentExecutor::executorPIN(const ScheduledIntent &intent) const
{
    auto targetID = TargetRef::getId(intent.intent.targetID); // ID кто должен сделать
    for (size_t i{}; i < pinsG.size(); ++i)
    {
        if (pinsG[i]->get_id() == targetID)
        {
            executorRezult(pinsG[i], intent);
        }
    }
}

void IntentExecutor::executorDEVICE(const ScheduledIntent &intent) const
{
}

void IntentExecutor::executorRezult(PIN *pin, const ScheduledIntent &intent) const
{
    ExecuteResult rezult{};
    switch (intent.life)
    {
    case LifetimeType::ONCE_TRY:
        rezult = pin->executor(intent, store->resolvePriority(intent.source, intent.urgency), LockPolicyType::INFINITE);
        break;
    case LifetimeType::ONESHOT:
        rezult = pin->executor(intent, store->resolvePriority(intent.source, intent.urgency), LockPolicyType::TTL, intent.schedule.endTime);
        break;
    case LifetimeType::REPEAT:
        rezult = pin->executor(intent, store->resolvePriority(intent.source, intent.urgency), LockPolicyType::TTL, intent.schedule.endTime);
        break;
    case LifetimeType::UNENDING:
        rezult = pin->executor(intent, store->resolvePriority(intent.source, intent.urgency), LockPolicyType::INFINITE, intent.schedule.endTime);
        break;
    default:
        rezult = pin->executor(intent, store->resolvePriority(intent.source, intent.urgency), LockPolicyType::INFINITE);
    }

    switch (rezult)
    {
    case ExecuteResult::SUCCESS:
        store->setStateExecutor(intent.id, ExecuteResult::SUCCESS);
        break;
    case ExecuteResult::SUCCESS_OVERRIDE_EQUAL_PRIORITY:
    case ExecuteResult::SUCCESS_OVERRIDE_LOWER_PRIORITY:
        if (intent.id == pin->get_pending_lock(intent.intent.type).id)
            break;
        // устанавливаем данные для текущего
        store->setStateExecutor(intent.id, rezult, pin->get_pending_lock(intent.intent.type).id);
        // устанавливаем данные для того кого вытеснили
        store->setStateExecutor(pin->get_pending_lock(intent.intent.type).id, ExecuteResult::BLOCKED_BY_HIGHER_PRIORITY, intent.id);
        break;
    case ExecuteResult::BLOCKED_BY_HIGHER_PRIORITY:
        // устанавливаем данные для текущего
        store->setStateExecutor(intent.id, rezult, pin->get_active_lock(intent.intent.type).id);
        break;
    case ExecuteResult::INVALID_PAYLOAD:
    case ExecuteResult::DRIVER_NOT_FOUND:
    case ExecuteResult::UNSUPPORTED_ACTION:
    default:
        store->setStateExecutor(intent.id, rezult);
        break;
    }
}
