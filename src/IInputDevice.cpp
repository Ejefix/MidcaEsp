#include "IInputDevice.h"


IInputDevice::IInputDevice(uint16_t id_): id(id_)
{
}

uint32_t IInputDevice::get_version()const
{
    return version;
}

uint16_t IInputDevice::get_id() const
{
    return id;
}
