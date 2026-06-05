#pragma once
#include "DeviceFactory.h"
#include "scenario_intent_system.h"

class PIN;
class IntentExecutor
{

public:
    void begin();

private:
    void executor(const ScheduledIntentID &id)const ;
    void executorPIN(const ScheduledIntent &intent)const;
    void executorDEVICE(const ScheduledIntent &intent)const;
    void executorRezult(PIN * pin, const ScheduledIntent &intent)const;
    
};
 