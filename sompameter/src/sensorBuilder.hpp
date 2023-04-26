#ifndef _SENSOR_BUILDER_CLASS_H
#define _SENSOR_BUILDER_CLASS_H

#include <ArduinoRS485.h>
#include <ArduinoModbus.h>
#include "sensorClass.hpp"
#include <map>

#include "sensor/sensorDummy.hpp"
#include "sensor/sensorSHT3X.hpp"
#include "sensor/sensorATH20.hpp"
#include "sensor/sensorBH1750.hpp"
#include "sensor/sensorMCP9600.hpp"

#define SENSOR_BUILDER_DEF_BAUD 9600
#define SENSOR_BUILDER_DEF_SLAVE 1
#define SENSOR_BUILDER_DEF_VERSION 0x10010001

#define SENSOR_BUILDER_DEF_VALUE 0x0000

class SensorBuilderClass
{
private:
    /* data */
    uint8_t _slave;
    uint32_t _baudrate;
    uint16_t _regs;
    bool _i2c_available = false;

    std::map<uint16_t, sensorClass *> m_sensorMap;

    enum
    {
        REG_ADDR = 0,
        REG_BAUD,
        REG_VERSION,
    };

public:
    SensorBuilderClass() : _regs(4){};
    ~SensorBuilderClass(){};
    uint16_t addSensor(sensorClass *sensor);
    // bool removeSensor(sensorClass *sensor);
    void check_grove(void);
    bool begin(uint8_t slave = SENSOR_BUILDER_DEF_SLAVE, uint32_t baudrate = SENSOR_BUILDER_DEF_BAUD);
    int poll();
	int truncatedPoll();
    uint16_t size();
};


void SensorBuilderClass::check_grove()
{
    // Check if an analog type sensor is connected
    GROVE_SWITCH_ADC;
    pinMode(SENSOR_ANALOG_PIN, OUTPUT);
    digitalWrite(SENSOR_ANALOG_PIN, HIGH);
    delay(10);
    pinMode(SENSOR_ANALOG_PIN, INPUT);
    // check i2c sensor
    _i2c_available = (digitalRead(SENSOR_ANALOG_PIN) == HIGH);
    if (!_i2c_available)
    {
        _i2c_available = (analogRead(SENSOR_ANALOG_PIN) > 100 && analogRead(SENSOR_ANALOG_E_PIN) > 100);
    }
}

bool SensorBuilderClass::begin(uint8_t slave, uint32_t baudrate)
{

    _slave = slave;
    _baudrate = baudrate;

    // start the Modbus RTU server, with (slave) id 1
    RS485.setDelays(5000, 5000);
    if (!ModbusRTUServer.begin(_slave, _baudrate))
    {
        Serial.println("Failed to start Modbus RTU Client!");
        while (1)
            ;
    }

    Serial.print("Version: ");
    Serial.println(SENSOR_BUILDER_DEF_VERSION, HEX);

    Serial.print("regs: ");
    Serial.println(_regs);

    ModbusRTUServer.configureInputRegisters(0x00, _regs);
    ModbusRTUServer.configureHoldingRegisters(0x00, _regs);

    ModbusRTUServer.inputRegisterWrite(SensorBuilderClass::REG_ADDR, _slave);
    ModbusRTUServer.holdingRegisterWrite(SensorBuilderClass::REG_ADDR, _slave);

    ModbusRTUServer.inputRegisterWrite(SensorBuilderClass::REG_BAUD, _baudrate / 100);
    ModbusRTUServer.holdingRegisterWrite(SensorBuilderClass::REG_BAUD, _baudrate / 100);

    ModbusRTUServer.inputRegisterWrite(SensorBuilderClass::REG_VERSION, (uint16_t)(SENSOR_BUILDER_DEF_VERSION >> 16));
    ModbusRTUServer.inputRegisterWrite(SensorBuilderClass::REG_VERSION + 1, (uint16_t)(SENSOR_BUILDER_DEF_VERSION & 0x0000FFFF));
    ModbusRTUServer.holdingRegisterWrite(SensorBuilderClass::REG_VERSION, (uint16_t)(SENSOR_BUILDER_DEF_VERSION >> 16));
    ModbusRTUServer.holdingRegisterWrite(SensorBuilderClass::REG_VERSION + 1, (uint16_t)(SENSOR_BUILDER_DEF_VERSION & 0x0000FFFF));

    /* skip head information */
    for (uint16_t i = 4; i < _regs; i += 1)
    {
        ModbusRTUServer.inputRegisterWrite(i, (uint16_t)(SENSOR_BUILDER_DEF_VALUE >> 16));
        ModbusRTUServer.holdingRegisterWrite(i, (uint16_t)(SENSOR_BUILDER_DEF_VALUE >> 16));
    }

    return true;
}

int SensorBuilderClass::truncatedPoll()
{
    // we want to pack measurements to have less needed bandwith
    // registers are readed as uint32_abcd registers
    // actual decoding is done in backed
    int8_t b8_value;
    int16_t b16_value;
    int reg = 4;
    uint16_t packed_value;
    int bi = 0;

    for (auto iter = m_sensorMap.begin(); iter != m_sensorMap.end(); ++iter)
    {
        sensorClass *sensor = (sensorClass *)iter->second;

        if (!sensor->connected())
        {
            continue;
        }

        sensor->sample();

        auto m_measureValue = sensor->getMeasureValue();
        for (auto m_iter = m_measureValue.begin(); m_iter != m_measureValue.end(); ++m_iter)
        {

            Serial.print(sensor->name().c_str());
            Serial.print(" - ");
            Serial.println(m_iter->value.s32);
            // NOTE: our current sensors return single type s32 no need to implement other yet
            // NOTE: items must be in pairs do not put 3 b8 items in row
            switch (m_iter->truncated)
            {
                case sensorClass::truncatedType_t::TRUNCATED_8:
                    b8_value = (int8_t)m_iter->value.s32;
                    packed_value += b8_value << bi * 8;
                    bi += 1;
                    break;
                case sensorClass::truncatedType_t::TRUNCATED_16:
                    b16_value = (int16_t)m_iter->value.s32;
                    packed_value += b8_value << bi * 8;
                    bi += 2;
                    break;
            }
            if (bi >= 2)
            {
                Serial.print(reg);
                Serial.print(" : ");
                Serial.println(packed_value);

                ModbusRTUServer.inputRegisterWrite(reg, packed_value);
                ModbusRTUServer.holdingRegisterWrite(reg, packed_value);
                bi = 0;
                packed_value = 0;
                reg += 1;
            }
        }
    }
    if (packed_value != 0)
    {
        Serial.print(reg);
        Serial.print(" -: ");
        Serial.println(packed_value);

        ModbusRTUServer.inputRegisterWrite(reg, packed_value);
        ModbusRTUServer.holdingRegisterWrite(reg, packed_value);
        bi = 0;
        packed_value = 0;
        reg += 1;
    }

    return ModbusRTUServer.poll();
}

int SensorBuilderClass::poll()
{
    for (auto iter = m_sensorMap.begin(); iter != m_sensorMap.end(); ++iter)
    {
        sensorClass *sensor = (sensorClass *)iter->second;

        if (!sensor->connected())
        {
            continue;
        }

        sensor->sample();

        auto m_measureValue = sensor->getMeasureValue();
        Serial.println(sensor->name().c_str());
        for (auto m_iter = m_measureValue.begin(); m_iter != m_measureValue.end(); ++m_iter)
        {
            Serial.print(m_iter->addr);
            Serial.print(':');
            switch (m_iter->type)
            {
            case sensorClass::regType_t::REG_TYPE_U16_AB:
                Serial.println(m_iter->value.u16);
                ModbusRTUServer.inputRegisterWrite(m_iter->addr, m_iter->value.u16);
                ModbusRTUServer.holdingRegisterWrite(m_iter->addr, m_iter->value.u16);
                break;
            case sensorClass::regType_t::REG_TYPE_S16_AB:
                Serial.println(m_iter->value.s16);
                ModbusRTUServer.inputRegisterWrite(m_iter->addr, m_iter->value.s16);
                ModbusRTUServer.holdingRegisterWrite(m_iter->addr, m_iter->value.s16);
                break;
            case sensorClass::regType_t::REG_TYPE_U32_ABCD:
                Serial.println(m_iter->value.u32);
                ModbusRTUServer.inputRegisterWrite(m_iter->addr, (uint16_t)(m_iter->value.u32 >> 16));
                ModbusRTUServer.inputRegisterWrite(m_iter->addr + 1, (uint16_t)(m_iter->value.u32 & 0x0000FFFF));
                ModbusRTUServer.holdingRegisterWrite(m_iter->addr, (uint16_t)(m_iter->value.u32 >> 16));
                ModbusRTUServer.holdingRegisterWrite(m_iter->addr + 1, (uint16_t)(m_iter->value.u32 & 0x0000FFFF));
                break;
            case sensorClass::regType_t::REG_TYPE_S32_ABCD:
                Serial.println(m_iter->value.s32);
                ModbusRTUServer.inputRegisterWrite(m_iter->addr, (uint16_t)(m_iter->value.s32 >> 16));
                ModbusRTUServer.inputRegisterWrite(m_iter->addr + 1, (uint16_t)(m_iter->value.s32 & 0x0000FFFF));
                ModbusRTUServer.holdingRegisterWrite(m_iter->addr, (uint16_t)(m_iter->value.s32 >> 16));
                ModbusRTUServer.holdingRegisterWrite(m_iter->addr + 1, (uint16_t)(m_iter->value.s32 & 0x0000FFFF));
                break;
            case sensorClass::regType_t::REG_TYPE_U32_CDAB:
                Serial.println(m_iter->value.u32);
                ModbusRTUServer.inputRegisterWrite(m_iter->addr, (uint16_t)(m_iter->value.u32 & 0x0000FFFF));
                ModbusRTUServer.inputRegisterWrite(m_iter->addr + 1, (uint16_t)(m_iter->value.u32 >> 16));
                ModbusRTUServer.holdingRegisterWrite(m_iter->addr, (uint16_t)(m_iter->value.u32 & 0x0000FFFF));
                ModbusRTUServer.holdingRegisterWrite(m_iter->addr + 1, (uint16_t)(m_iter->value.u32 >> 16));
                break;
            case sensorClass::regType_t::REG_TYPE_S32_CDAB:
                Serial.println(m_iter->value.s32);
                ModbusRTUServer.inputRegisterWrite(m_iter->addr, (uint16_t)(m_iter->value.s32 & 0x0000FFFF));
                ModbusRTUServer.inputRegisterWrite(m_iter->addr + 1, (uint16_t)(m_iter->value.s32 >> 16));
                ModbusRTUServer.holdingRegisterWrite(m_iter->addr, (uint16_t)(m_iter->value.s32 & 0x0000FFFF));
                ModbusRTUServer.holdingRegisterWrite(m_iter->addr + 1, (uint16_t)(m_iter->value.s32 >> 16));
                break;
            default:
                break;
            }
        }
        Serial.println();
    }
    Serial.println("================");

    return ModbusRTUServer.poll();
}

uint16_t SensorBuilderClass::size()
{
    return m_sensorMap.size();
}

uint16_t SensorBuilderClass::addSensor(sensorClass *_sensor)
{
    uint16_t regs = 0;

    for (std::map<uint16_t, sensorClass *>::const_iterator iter = m_sensorMap.begin(); iter != m_sensorMap.end(); ++iter)
    {
        if (iter->second == _sensor)
        {
            return false;
        }
    }

    m_sensorMap.insert(std::pair<uint16_t, sensorClass *>(m_sensorMap.size(), _sensor));

    regs = _sensor->init(_regs, _i2c_available);

    _regs += regs;

    return regs;
}

// bool SensorBuilderClass::removeSensor(sensorClass *_sensor)
// {
//     for (std::map<uint16_t, sensorClass *>::iterator iter = m_sensorMap.begin(); iter != m_sensorMap.end(); ++iter)
//     {
//         if (iter->second == _sensor)
//         {
//             iter = m_sensorMap.erase(iter);
//             return true;
//         }
//     }

//     return false;
// }

#endif
