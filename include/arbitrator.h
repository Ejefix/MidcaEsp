#pragma once
#include "scenario_intent_system.h"

enum class LifecycleResolution
{
    WAIT,    // время выполнения ещё не наступило
    EXECUTE, // сейчас можно выполнять
    EXPIRED  // жизненный цикл уже закончился
};

/*
обрабатывает намериния пользователя
является центром логики обработки событий.
*/
class Arbitrator
{
public:
    Arbitrator(ScheduledIntentStore &s);
    void begin();

private:
    ScheduledIntentStore &store;
    void beginINTENTtarget(const ScheduledIntent *target, const std::vector<ScheduledIntentID> &vec) const;

    /*
     * Первый подходящий кандидат становится исполнителем группы.
     * Остальные не выбираются как исполнитель.
     */
    void beginPINtarget(const std::vector<ScheduledIntentID> &vec) const;

    // отчает на вопрос, имеет ли права перейти сейчас в статус выполнения
    LifecycleResolution resolve_lifecycle(const ScheduledIntent &candidate) const;
    bool isExecutionFinished(const ScheduledIntent &candidate) const;
};