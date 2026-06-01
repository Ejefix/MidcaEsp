#ifndef COMMANDEXECUTOR_H
#define COMMANDEXECUTOR_H
#include <skeleton.h>
#include <Arduino.h>

class CommandExecutor : public Skeleton
{
public:
    CommandExecutor();
    int begin(const String &packet) const;
    int playPIN(const String &packet, int pinNumber) const;
#if FW_BUILD == FW_RELAY
    int playSensor(const String &packet) const;
    
#endif
    String full_status_json() const;
};
  
#endif // COMMANDEXECUTOR_H