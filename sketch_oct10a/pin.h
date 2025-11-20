#ifndef PIN_H
#define PIN_H
#include <vector>
#include <skeleton.h>


class PIN :  public Skeleton
{
public:
    PIN(unsigned int pin);
    void push_script_time(std::pair<unsigned long long, unsigned long long> &time);
    bool clear_script_time(unsigned long long &time);
    
    bool isPinActivation(const unsigned long long &time);

    String get_status_pin();
    void set_status_pin(bool status);
    // принудительное 1. включить 2. выключить 3. запустить сценарий
    void set_status_user(int user);

    void save();

    unsigned int get_pinNumber();

private:    

    void load();
    unsigned int pinNumber{};
    bool user_on{false};
    bool user_off{false}; 
    bool script{true}; 
    std::vector<std::pair<uint64_t, uint64_t>> script_time{};
    
    
    bool status_pin{false};
    const uint64_t one_day = 24ULL * 60ULL * 60ULL * 1000ULL;
};

#endif // PIN_H
