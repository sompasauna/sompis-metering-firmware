#include <Arduino.h>

// sensor libraries - these go under sensors in src

#include <ArduinoModbus.h>
#include <ArduinoRS485.h>

#include <Adafruit_SHT31.h>
#include <Adafruit_MCP9600.h>
#include <Adafruit_MCP9601.h>
#include <Adafruit_BusIO_Register.h>
#include <Adafruit_I2CDevice.h>
#include <Adafruit_I2CRegister.h>
#include <Adafruit_SPIDevice.h>
#include <Adafruit_ADS1X15.h>

#include <BH1750.h>
#include <ATH20.h>
// NOTE: no ready made library for HT0740 40V, 10A Switch Breakout =/

#include "src/sensorBuilder.hpp"

SensorBuilderClass SensorBuilder;

sensorBH1750 outsideLightLevel;

void setup() {
  Serial.begin(9600);
  SensorBuilder.check_grove();
 
  /* sensor list */
  //sensorDummy *dummySensor = new sensorDummy();
  //SensorBuilder.addSensor(dummySensor);
  
  //sensorSHT3X *saunaTempAndHumidity = new sensorSHT3X();
  //SensorBuilder.addSensor(saunaTempAndHumidity);

  sensorATH20 *outsideTempAndHumidity = new sensorATH20();
  SensorBuilder.addSensor(outsideTempAndHumidity);
  sensorATH20 *outsideTempAndHumidity2 = new sensorATH20();
  SensorBuilder.addSensor(outsideTempAndHumidity2);
  sensorATH20 *outsideTempAndHumidity3 = new sensorATH20();
  SensorBuilder.addSensor(outsideTempAndHumidity3);
  sensorATH20 *outsideTempAndHumidity4 = new sensorATH20();
  SensorBuilder.addSensor(outsideTempAndHumidity4);
  sensorATH20 *outsideTempAndHumidity5 = new sensorATH20();
  SensorBuilder.addSensor(outsideTempAndHumidity5);


  sensorMCP9600 *saunaStonesTemp = new sensorMCP9600();
  SensorBuilder.addSensor(saunaStonesTemp);

  // if need to broadcast
  SensorBuilder.addSensor(&outsideLightLevel);

  SensorBuilder.begin();

}

void loop() {
  SensorBuilder.truncatedPoll();
  // TODO: buffer measurements for 5min and return averages(?)

  // TODO: toggle lights on/off by light level
  //float lux = outsideLightLevel.directRead();

}
