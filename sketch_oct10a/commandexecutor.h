#ifndef COMMANDEXECUTOR_H
#define COMMANDEXECUTOR_H
#include <skeleton.h>
#include <Arduino.h>

class CommandExecutor : public Skeleton
{
public:
    CommandExecutor();
    int begin(const String &packet);
    int playPIN(const String &packet, int pinNumber);
    String full_status_pins()const;
    String status_pins()const;

};

#endif // COMMANDEXECUTOR_H