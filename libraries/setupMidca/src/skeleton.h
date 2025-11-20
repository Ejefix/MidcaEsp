#ifndef SKELETON_H
#define SKELETON_H
#include <Arduino.h>
#include <array>

class Skeleton
{
public:
    Skeleton();


    enum{
        pin1 , pin2, pin3, pin4, pin5,
        pin6, pin7, pin8, pin9, pin10,
        pin11,pin12,pin13,pin14,pin15,
        pin16,pin17,pin18,pin19,pin20,
        pin21,pin22,pin23,pin24,pin25,
        pin26,pin27,pin28,pin29,pin30,

        idESP,idUSER, idSer,
        accepted,
        start,
        separator,
        finish,

        on, off,
        time_on,time_off, user_on, user_off,
        script_on, script_off,


        status, status_full,
        end
    };
    const std::array<String,Skeleton::end> commands{
        "%p01", "%p02", "%p03", "%p04", "%p05",
        "%p06", "%p07", "%p08", "%p09", "%p10",
        "%p11", "%p12", "%p13", "%p14", "%p15",
        "%p16", "%p17", "%p18", "%p19", "%p20",
        "%p21", "%p22", "%p23", "%p24", "%p25",
        "%p26", "%p27", "%p28", "%p29", "%p30",

        "%I00", "%I11", "%I88",
        "%G00",
        "%S00",
        "%S01",
        "%F00",

        "%O01", "%O02",
        "%t01", "%t02","%U01", "%U02",
        "%U03", "%U04",

        "%S10","%S11"
    };
    String id{};
};

#endif // SKELETON_H
