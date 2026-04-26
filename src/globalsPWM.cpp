#include "firmware_profile.h"
#if FW_BUILD == FW_PWM
#include "globalsPWM.h"

const std::array<int, 15> GPIOledPwm{12, 13, 14, 19, 16, 17, 18, 21, 22, 23, 25, 26, 27, 32, 33};

const std::array<IPinDriver *, 15> PinDriver{
    new PWMDriver{GPIOledPwm[0]},
    new PWMDriver{GPIOledPwm[1]},
    new PWMDriver{GPIOledPwm[2]},
    new PWMDriver{GPIOledPwm[3]},
    new PWMDriver{GPIOledPwm[4]},
    new PWMDriver{GPIOledPwm[5]},
    new PWMDriver{GPIOledPwm[6]},
    new PWMDriver{GPIOledPwm[7]},
    new PWMDriver{GPIOledPwm[8]},
    new PWMDriver{GPIOledPwm[9]},
    new PWMDriver{GPIOledPwm[10]},
    new PWMDriver{GPIOledPwm[11]},
    new PWMDriver{GPIOledPwm[12]},
    new PWMDriver{GPIOledPwm[13]},
    new PWMDriver{GPIOledPwm[14]},
};

std::array<PIN *, 15> pinsG{
    new PIN{PinDriver[0]},
    new PIN{PinDriver[1]},
    new PIN{PinDriver[2]},
    new PIN{PinDriver[3]},
    new PIN{PinDriver[4]},
    new PIN{PinDriver[5]},
    new PIN{PinDriver[6]},
    new PIN{PinDriver[7]},
    new PIN{PinDriver[8]},
    new PIN{PinDriver[9]},
    new PIN{PinDriver[10]},
    new PIN{PinDriver[11]},
    new PIN{PinDriver[12]},
    new PIN{PinDriver[13]},
    new PIN{PinDriver[14]}};
#endif