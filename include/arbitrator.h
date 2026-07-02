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

    /*
     * Первый подходящий кандидат становится исполнителем группы.
     * Остальные не выбираются как исполнитель.
     */
    void beginTarget(const std::vector<ScheduledIntentID> &vec) const;

    // отчает на вопрос, имеет ли права перейти сейчас в статус выполнения
    LifecycleResolution resolve_lifecycle(const ScheduledIntent &candidate) const;
    bool isExecutionFAILED(const ScheduledIntent &candidate) const;
    // схлопывание или откладывание
    void deferOrOverride(const ScheduledIntent &winner, const ScheduledIntent &loser) const;
    // нету рузультата выполнения
    bool approve_before_execution(const ScheduledIntent &candidate, LifecycleResolution answer) const;
    // выполнялся
    bool approve_after_execution(const ScheduledIntent &candidate, LifecycleResolution answer) const;
    bool arbitrate(const ScheduledIntent &candidate) const;
};