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

    // Проверяет жизненный цикл ScheduledIntent относительно текущего времени.
    // Определяет, находится ли intent в активном временном окне выполнения.
    //
    // Если текущее время внутри окна [startTime, endTime] — intent считается валидным для выполнения (true).
    // Если текущее время ещё не достигло startTime — никаких изменений не происходит (false).
    // Если окно выполнения уже истекло:
    //   - для REPEAT intent переводится в PAUSED и переносится на следующий день
    //   - для ONESHOT intent переводится в финальное состояние (DONE)
    //   - дополнительно фиксируется причина завершения
    //
    // Возвращает:
    //   true  — intent можно выполнять сейчас
    //   false — intent не должен выполняться сейчас (либо ещё не время, либо уже завершён/перенесён)
    bool resolve_lifecycle(const ScheduledIntent &candidate) const;
    bool resolve_execution(const ScheduledIntent &candidate) const;
};