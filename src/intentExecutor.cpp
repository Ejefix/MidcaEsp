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
    auto intent = store->get(id);
    if (!intent)
        return;
    auto time = myclock.getEpochMillis();

    auto typeID = TargetRef::getType(intent->intent.targetID); // кто должен сделать

    switch (typeID)
    {
    case TargetType::PIN:
        executorPIN(*intent);

        break;
    case TargetType::DEVICE:
        executorDEVICE(*intent);
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
            ExecuteResult rezult = ExecuteResult::UNSUPPORTED_ACTION;
            // это повидение учитывает только если не указан временной интервал и намериние однократное !
            if (intent.life == LifetimeType::ONESHOT &&  intent.schedule.startTime == intent.schedule.endTime)
            {
                // ExecuteResult executor(const ScheduledIntent &intent, uint8_t priority, LockPolicyType policy, timeMS endTime = 0);
                auto rezult = pinsG[i]->executor(intent, store->resolvePriority(intent.source, intent.urgency), LockPolicyType::INFINITE);
                Rezult rezultPIN{};
                switch (rezult)
                {
                case ExecuteResult::SUCCESS:
                    store->setState(intent.id, IntentState::DONE);
                    break;
                case ExecuteResult::OVERRIDE_EQUAL_PRIORITY:
                case ExecuteResult::OVERRIDE_LOWER_PRIORITY:
                    store->setState(intent.id, IntentState::DONE);
                    store->setState(pinsG[i]->get_pending_lock(intent.intent.type).id, IntentState::RUNNING);
                    break;
                case ExecuteResult::BLOCKED_BY_HIGHER_PRIORITY:
                    rezultPIN.rezult = ExecuteResult::BLOCKED_BY_HIGHER_PRIORITY;
                    rezultPIN.blockingIntentID = pinsG[i]->get_active_lock(intent.intent.type).id;
                    store->setState(intent.id, IntentState::STOP, rezultPIN);
                    break;
                case ExecuteResult::INVALID_PAYLOAD:
                    rezultPIN.rezult = ExecuteResult::BLOCKED_BY_HIGHER_PRIORITY;
                    store->setState(intent.id, IntentState::FAILED, rezultPIN);
                    break;
                case ExecuteResult::DRIVER_NOT_FOUND:
                    rezultPIN.rezult = ExecuteResult::DRIVER_NOT_FOUND;
                    store->setState(intent.id, IntentState::FAILED, rezultPIN);
                    break;
                case ExecuteResult::UNSUPPORTED_ACTION:
                default:
                    rezultPIN.rezult = ExecuteResult::UNSUPPORTED_ACTION;
                    store->setState(intent.id, IntentState::FAILED, rezultPIN);
                    break;
                }
            }
            return;
        }
    }
}

void IntentExecutor::executorDEVICE(const ScheduledIntent &intent) const
{
}
