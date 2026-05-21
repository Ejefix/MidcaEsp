#pragma once
enum class InputEvent
{
    // ошибка
    NoEventErr,
    // ничего
    NoEvent,
    // 0 -> 1
    RisingEdge,
    // 1 -> 0
    FallingEdge,
    // удерживается 1
    HighLevel,
    // удерживается 0
    LowLevel,
    // переключить
    Toggle,
    
    LongPress, // долгое удержание

    DoubleClick // двойной клик

};

enum class DeviceType
{
    Sensor,
    Switch,
    Button
};
class IInputDevice
{
public:
    virtual ~IInputDevice() = default;
    virtual InputEvent event() = 0;
    virtual DeviceType type() = 0;
    
};