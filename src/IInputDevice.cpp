#include "IInputDevice.h"


IInputDevice::IInputDevice(uint16_t id_): id(id_)
{
}

uint16_t IInputDevice::get_id() const
{
    return id;
}
