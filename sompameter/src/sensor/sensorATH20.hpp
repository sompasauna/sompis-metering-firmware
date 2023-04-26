#ifndef _SENSOR_ATH20_H
#define _SENSOR_ATH20_H

#include "../sensorClass.hpp"
#include <ATH20.h>
#include <Wire.h>


class sensorATH20 : public sensorClass
{
public:
    sensorATH20() : sensorClass("ATH20")
    {
        ath20 = new ATH20();
    };
    ~sensorATH20(){};

    uint16_t init(uint16_t reg, bool i2c_available);
    bool connected();
    bool sample();

    enum
    {
        TEMPERATURE = 0x00, // C
        HUMIDITY = 0x01,    // %
        MAX
    };

private:
    ATH20 *ath20;
};

uint16_t sensorATH20::init(uint16_t reg, bool i2c_available)
{
    uint16_t t_reg = reg;

    for (uint16_t i = 0; i < sensorATH20::MAX; i++)
    {
        sensorClass::reg_t value;
        value.addr = t_reg;
        value.type = sensorClass::regType_t::REG_TYPE_S32_ABCD;
        value.truncated = sensorClass::truncatedType_t::TRUNCATED_8;
        value.value.s32 = 0;
        m_valueVector.emplace_back(value);
        t_reg += sensorClass::valueLength(value.type);
    }

    if (!i2c_available)
    {
        _connected = false;
        return t_reg - reg;
    }

    Serial.println("ath20 init");
    // unfortunately this lib is not sophisticated
    // so no test for success of init etc.
    GROVE_SWITCH_IIC;
    ath20->begin();

    _connected = true;
    return t_reg - reg;
}

bool sensorATH20::sample()
{
    GROVE_SWITCH_IIC;

    Serial.println("ath20 sample");
    float humidity;
    float temperature;

    int ret = ath20->getSensor(&humidity, &temperature);

    if(not ret) {
        return false;
    }

    m_valueVector[TEMPERATURE].value.s32 = temperature;
    m_valueVector[HUMIDITY].value.s32 = humidity * 100;

    return true;
}

bool sensorATH20::connected()
{
    return _connected;
}

#endif
