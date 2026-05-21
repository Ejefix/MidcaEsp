#include "scenario_intent_system.h"

void IntentStore::add(Intent &intent)
{

    intent.id = nextId;
    ++nextId;
    data.emplace(intent.id, std::move(intent));
}

void IntentStore::remove(uint64_t id)
{
    data.erase(id);
}

const std::map<uint64_t, Intent> &IntentStore::all() const
{
    return data;
}
