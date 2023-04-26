#ifndef _SENSOR_BH1750_H
#define _SENSOR_BH1750_H

#include "../sensorClass.hpp"
#include <BH1750.h>
#include <Wire.h>

#define SENSOR_BH1750_I2C_ADDR 0x23

class sensorBH1750 : public sensorClass
{
public:
    sensorBH1750(uint8_t addr = SENSOR_BH1750_I2C_ADDR) : sensorClass("BH1750")
    {
        bh1750 = new BH1750(SENSOR_BH1750_I2C_ADDR);
    };
    ~sensorBH1750(){};

    uint16_t init(uint16_t reg, bool i2c_available);
    bool connected();
    bool sample();

    enum
    {
        LUX = 0x00,
        MAX
    };

private:
     BH1750 *bh1750;
};

uint16_t sensorBH1750::init(uint16_t reg, bool i2c_available)
{
    uint16_t t_reg = reg;

    for (uint16_t i = 0; i < sensorBH1750::MAX; i++)
    {
        sensorClass::reg_t value;
        value.addr = t_reg;
        value.type = sensorClass::regType_t::REG_TYPE_S32_ABCD;
        value.truncated = sensorClass::truncatedType_t::TRUNCATED_16;
        value.value.s32 = 0;
        m_valueVector.emplace_back(value);
        t_reg += sensorClass::valueLength(value.type);
    }

    if (!i2c_available)
    {
        _connected = false;
        return t_reg - reg;
    }
    GROVE_SWITCH_IIC;
    Wire.begin();
    Wire.beginTransmission(SENSOR_BH1750_I2C_ADDR);
    if (Wire.endTransmission() != 0)
    {
        _connected = false;
        return t_reg - reg;
    }

    Serial.println("bh1750 init");
    GROVE_SWITCH_IIC;
    if (!bh1750->begin())
    {
        Serial.println("bh1750 init failed");
        _connected = false;
        return t_reg - reg;
    }

    _connected = true;
    return t_reg - reg;
}

bool sensorBH1750::sample()
{
    GROVE_SWITCH_IIC;

    Serial.println("bh1750 sample");
    float lux = bh1750->readLightLevel();

    if (isnan(lux)) {
        return false;
    }

    m_valueVector[LUX].value.s32 = lux * SCALE;

    return true;
}


bool sensorBH1750::connected()
{
    return _connected;
}

#endif
