#ifndef _SENSOR_MCP9600_H
#define _SENSOR_MCP9600_H

#include "../sensorClass.hpp"
#include <Adafruit_MCP9600.h>
#include <Wire.h>

#define SENSOR_MCP9600_I2C_ADDR 0x67

class sensorMCP9600 : public sensorClass
{
public:
    sensorMCP9600() : sensorClass("MCP9600")
    {
        mcp = new Adafruit_MCP9600();
    };
    ~sensorMCP9600(){};

    uint16_t init(uint16_t reg, bool i2c_available);
    bool connected();
    bool sample();

    enum
    {
        HOTJUNCTION = 0x00, // C
        COLDJUNCTION = 0x01, // C
        // probably not interesting to transmit
        // ADC = 0x02, // uV
        MAX
    };

private:
    Adafruit_MCP9600 *mcp;
};

uint16_t sensorMCP9600::init(uint16_t reg, bool i2c_available)
{
    uint16_t t_reg = reg;

    for (uint16_t i = 0; i < sensorMCP9600::MAX; i++)
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
    Wire.beginTransmission(SENSOR_MCP9600_I2C_ADDR);
    if (Wire.endTransmission() != 0)
    {
        _connected = false;
        return t_reg - reg;
    }

    Serial.println("mcp init");
    GROVE_SWITCH_IIC;
    if (!mcp->begin(SENSOR_MCP9600_I2C_ADDR))
    {
        Serial.println("mcp init failed");
        _connected = false;
        return t_reg - reg;
    }
    mcp->setADCresolution(MCP9600_ADCRESOLUTION_18);
    mcp->setThermocoupleType(MCP9600_TYPE_K);

    _connected = true;
    return t_reg - reg;
}

bool sensorMCP9600::sample()
{
    GROVE_SWITCH_IIC;

    Serial.println("mcp sample");
    float hotJunction = mcp->readThermocouple();
    float coldJunction = mcp->readAmbient();
    int32_t adc = mcp->readADC();

    if (isnan(hotJunction) or isnan(coldJunction)) {
        return false;
    }

    m_valueVector[HOTJUNCTION].value.s32 = hotJunction * SCALE;
    m_valueVector[COLDJUNCTION].value.s32 = coldJunction * SCALE;

    return true;
}

bool sensorMCP9600::connected()
{
    return _connected;
}

#endif
