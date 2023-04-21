#ifndef _SENSOR_SHT3X_H
#define _SENSOR_SHT3X_H

#include "../sensorClass.hpp"
#include <Adafruit_SHT31.h>
#include <Wire.h>

// set to 0X45 for alternative address
#define SENSOR_SHT3X_I2C_ADDR 0x44

class sensorSHT3X : public sensorClass
{
public:
    sensorSHT3X(uint8_t addr = SENSOR_SHT3X_I2C_ADDR) : sensorClass("SHT3X")
    {
        sht3X = new Adafruit_SHT31();
    };
    ~sensorSHT3X(){};

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
    Adafruit_SHT31 *sht3X;
};

uint16_t sensorSHT3X::init(uint16_t reg, bool i2c_available)
{
    uint16_t t_reg = reg;

    for (uint16_t i = 0; i < sensorSHT3X::MAX; i++)
    {
        sensorClass::reg_t value;
        value.addr = t_reg;
        value.type = sensorClass::regType_t::REG_TYPE_S32_ABCD;
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
    Wire.beginTransmission(SENSOR_SHT3X_I2C_ADDR);
    if (Wire.endTransmission() != 0)
    {
        _connected = false;
        return t_reg - reg;
    }

    Serial.println("sht3X init");
    GROVE_SWITCH_IIC;
    if (!sht3X->begin(SENSOR_SHT3X_I2C_ADDR))
    {
        Serial.println("sht3X init failed");
        _connected = false;
        return t_reg - reg;
    }

    _connected = true;
    return t_reg - reg;
}

bool sensorSHT3X::sample()
{
    GROVE_SWITCH_IIC;

    Serial.println("sht31 sample");
    float temperature = sht3X->readTemperature();
    float humidity = sht3X->readHumidity();

    if (isnan(temperature) or isnan(humidity)) {
	return false;
    }

    m_valueVector[TEMPERATURE].value.s32 = temperature * SCALE;
    m_valueVector[HUMIDITY].value.s32 = humidity * SCALE;

    return true;
}

bool sensorSHT3X::connected()
{
    return _connected;
}

#endif
