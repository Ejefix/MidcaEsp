#pragma once
#include "scenario_intent_system.h"

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

    timeMS time{};
};