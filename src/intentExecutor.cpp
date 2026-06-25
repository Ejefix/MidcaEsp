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
    {
        store->setStateExecutor(intent.id, ExecuteResult::SUCCESS);
        auto targetID = TargetRef::getId(intent.intent.targetID);
        switch (intent.intent.type)
        {
        case ActionType::ON:
            store->setIntentCommand(targetID, IntentCommand::ACTIVE, intent.id);
            break;
        case ActionType::OFF:
            store->setIntentCommand(targetID, IntentCommand::PAUSED, intent.id);
            break;
        case ActionType::ERASE:
            store->setIntentCommand(targetID, IntentCommand::TO_DELETE, intent.id);
            break;
        }
        break;
    }
    default:
        store->setStateExecutor(intent.id, ExecuteResult::ERROR_TARGET);
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
            auto *exec = dynamic_cast<IExecutor *>(pinsG[i]);
            if (exec)
            {
                executorRezult(exec, intent);
            }
            else
            {
                store->setStateExecutor(intent.id, ExecuteResult::FAIL_CAST);
            }
        }
    }
}

void IntentExecutor::executorDEVICE(const ScheduledIntent &intent) const
{
    auto targetID = TargetRef::getId(intent.intent.targetID); // ID кто должен сделать
    if (device_registry->exists(targetID))
    {
        auto device = device_registry->get(targetID);
        auto *exec = dynamic_cast<IExecutor *>(device);
        if (exec)
        {
            executorRezult(exec, intent);
        }
        else
        {
            store->setStateExecutor(intent.id, ExecuteResult::FAIL_CAST);
        }
    }
}

void IntentExecutor::executorRezult(IExecutor *dev, const ScheduledIntent &intent) const
{
    ExecuteResult rezult{};
    switch (intent.life)
    {
    case LifetimeType::ONCE_TRY:
        rezult = dev->execute(intent, store->resolvePriority(intent.source, intent.urgency), LockPolicyType::INFINITE, intent.schedule.endTime);
        break;
    case LifetimeType::ONESHOT:
        rezult = dev->execute(intent, store->resolvePriority(intent.source, intent.urgency), LockPolicyType::TTL, intent.schedule.endTime);
        break;
    case LifetimeType::REPEAT:
        rezult = dev->execute(intent, store->resolvePriority(intent.source, intent.urgency), LockPolicyType::TTL, intent.schedule.endTime);
        break;
    case LifetimeType::UNENDING:
        rezult = dev->execute(intent, store->resolvePriority(intent.source, intent.urgency), LockPolicyType::INFINITE, intent.schedule.endTime);
        break;
    default:
        store->setStateExecutor(intent.id, ExecuteResult::ERROR_LIFE);
        return;
    }

    switch (rezult)
    {
    case ExecuteResult::SUCCESS:
        store->setStateExecutor(intent.id, ExecuteResult::SUCCESS);
        break;
    case ExecuteResult::SUCCESS_OVERRIDE_EQUAL_PRIORITY:
    case ExecuteResult::SUCCESS_OVERRIDE_LOWER_PRIORITY:
        if (intent.id == dev->get_pending_lock(intent.intent.type).id)
            break;
        // устанавливаем данные для текущего
        store->setStateExecutor(intent.id, rezult, dev->get_pending_lock(intent.intent.type).id);
        // устанавливаем данные для того кого вытеснили
        store->setStateExecutor(dev->get_pending_lock(intent.intent.type).id, ExecuteResult::BLOCKED_BY_HIGHER_PRIORITY, intent.id);
        break;
    case ExecuteResult::BLOCKED_BY_HIGHER_PRIORITY:
        // устанавливаем данные для текущего
        store->setStateExecutor(intent.id, rezult, dev->get_active_lock(intent.intent.type).id);
        break;
    case ExecuteResult::INVALID_PAYLOAD:
    case ExecuteResult::DRIVER_NOT_FOUND:
    case ExecuteResult::UNSUPPORTED_ACTION:
    default:
        store->setStateExecutor(intent.id, rezult);
        break;
    }
}
