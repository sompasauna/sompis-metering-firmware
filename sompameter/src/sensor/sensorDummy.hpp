#ifndef _SENSOR_DUMMY_H
#define _SENSOR_DUMMY_H

#include "../sensorClass.hpp"


class sensorDummy : public sensorClass
{
public:
    sensorDummy() : sensorClass("Dummy V1.2"){};
    ~sensorDummy(){};

    uint16_t init(uint16_t reg, bool i2c_available);
    bool connected();
    bool sample();

    enum
    {
        DUMMY = 0,
        MAX
    };
};

uint16_t sensorDummy::init(uint16_t reg, bool i2c_available)
{
    uint16_t t_reg = reg;

    for (uint16_t i = 0; i < sensorDummy::MAX; i++)
    {
        sensorClass::reg_t value;
        value.addr = t_reg;
        value.type = sensorClass::regType_t::REG_TYPE_S32_ABCD;
        t_reg += sensorClass::valueLength(value.type);
        value.value.s32 = 0;
        m_valueVector.emplace_back(value);
    }

    _connected = true;
    return t_reg - reg;
}

bool sensorDummy::sample()
{

    m_valueVector[sensorDummy::DUMMY].value.s32 = 42;
    return true;
}

bool sensorDummy::connected()
{
    return _connected;
}

#endif
