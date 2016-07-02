/*============================ Include ============================*/

#include <Arduino.h>
#include "nRF24L01.h"
#include "RF24.h"
#include "printf.h"
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_TSL2561.h>
#include <OneWire.h>

/*============================ Macro ============================*/

#define msleep(X) delay(X)

/*============================ Global variable ============================*/

// Set up nRF24L01 radio on SPI pin for CE, CSN
RF24 radio(9,10);
Adafruit_TSL2561 tsl = Adafruit_TSL2561(TSL2561_ADDR_FLOAT, 12345);
OneWire DS18B20(2);  // Temperature chip i/o on digital pin 2

/*============================ Local Function interface ============================*/

static void setupNRF24(void);
static void send(char * iData);
static void received(char* oData, long iMsTimeOut);
static void displaySensorDetails(void);
static void configureSensor(void);
static void setupTSL2561(void);
static long getTSL2561(void);
static float getDS18B20(void);

/*============================ Function implementation ============================*/

/*------------------------------- Main functions -------------------------------*/

void setup(void){
  Serial.begin(57600);
  printf_begin();
  printf("Light and temperature sensors\n\r");
   
  setupTSL2561();
  setupNRF24();

}

void loop(void){
  char lInBuffer[32];
  char lOutBuffer[32];
  static long lLightCount = 0;
  static long lTempCount = 0;

  received(lInBuffer, -1);
 // printf("received : %s\n\r",lInBuffer);

  if(strcmp(lInBuffer, "1") == 0){
    sprintf(lOutBuffer,"%ld",getTSL2561());
    send((char*)lOutBuffer);
    //printf("Light %ld\n\r",lLightCount++);
  }else if(strcmp(lInBuffer, "2") == 0){
    sprintf(lOutBuffer,"%ld",(long)(getDS18B20()*10));
    send((char*)lOutBuffer);
    //printf("Temp %ld\n\r",lTempCount++);
  }
}

/*------------------------------- nRF24 -------------------------------*/

static void setupNRF24(void){

  radio.begin();
  radio.enableDynamicPayloads();
  radio.setRetries(20,10);

  radio.openWritingPipe(0xF0F0F0F0E1LL);
  radio.openReadingPipe(1,0x7365727631LL);
   
  radio.startListening();
  //radio.printDetails();

  msleep(1000);
}

static void send(char* iData){

  if(strlen(iData) <= 32){
    radio.stopListening();
    radio.write(iData, strlen(iData));
    radio.startListening();
  }else{
    printf("Data to long %s\n\r", iData);
  }
}

static void received(char* oData, long iMsTimeOut){
  long lTimeReference;
  long lTime;

  lTimeReference = millis();
  lTime = lTimeReference;

  //msleep(iMsTimeOut); /* TODO */
  while(  ((lTime - lTimeReference) < iMsTimeOut)
        ||(iMsTimeOut == -1)){
    if(radio.available()){
      uint8_t len = radio.getDynamicPayloadSize();
      if(len <= 32){
        radio.read(oData, len);
        oData[len] = 0;
      }else{
        printf("Error in received function\n");
        oData[0] = 0;
      }
      return;
    }
    lTime = millis(); 
  }
  printf("Received time out %10ld %10ld %10ld\n", lTimeReference, lTime, lTime - lTimeReference);

  oData[0] = 0;
}

/*------------------------------- TSL2561 (Light) -------------------------------*/
/*
   Connections
   ===========
   Connect SCL to analog 5
   Connect SDA to analog 4
   Connect VDD to 3.3V DC
   Connect GROUND to common ground

   I2C Address
   ===========
   The address will be different depending on whether you leave
   the ADDR pin floating (addr 0x39), or tie it to ground or vcc. 
   The default addess is 0x39, which assumes the ADDR pin is floating
   (not connected to anything).  If you set the ADDR pin high
   or low, use TSL2561_ADDR_HIGH (0x49) or TSL2561_ADDR_LOW
   (0x29) respectively.
*/

static void displaySensorDetails(void){
  sensor_t sensor;
  tsl.getSensor(&sensor);
  // printf("------------------------------------\n\r");
  // printf("Sensor:       %s\n\r", sensor.name);
  // printf("Driver Ver:   %ld\n\r", sensor.version);
  // printf("Unique ID:    %ld\n\r", sensor.sensor_id);
  // printf("Max Value:    %ld lux\n\r", (long)(sensor.max_value));
  // printf("Min Value:    %ld lux\n\r", (long)(sensor.min_value));
  // printf("Resolution:   %ld lux\n\r", (long)(sensor.resolution));
  // printf("------------------------------------\n\r");
  msleep(500);
}

static void configureSensor(void){
  /* You can also manually set the gain or enable auto-gain support */
  // tsl.setGain(TSL2561_GAIN_1X);      /* No gain ... use in bright light to avoid sensor saturation */
  // tsl.setGain(TSL2561_GAIN_16X);     /* 16x gain ... use in low light to boost sensitivity */
  tsl.enableAutoGain(true);          /* Auto-gain ... switches automatically between 1x and 16x */
  
  /* Changing the integration time gives you better sensor resolution (402ms = 16-bit data) */
  tsl.setIntegrationTime(TSL2561_INTEGRATIONTIME_13MS);      /* fast but low resolution */
  // tsl.setIntegrationTime(TSL2561_INTEGRATIONTIME_101MS);  /* medium resolution and speed   */
  // tsl.setIntegrationTime(TSL2561_INTEGRATIONTIME_402MS);  /* 16-bit data but slowest conversions */

  /* Update these values depending on what you've set above! */  
  // printf("------------------------------------\n\r");
  // printf("Gain:         Auto\n\r");
  // printf("Timing:       13 ms\n\r");
  // printf("------------------------------------\n\r");
}

static void setupTSL2561(void){
  /* Initialise the sensor */
  if(!tsl.begin())
  {
    /* There was a problem detecting the ADXL345 ... check your connections */
    printf("Ooops, no TSL2561 detected ... Check your wiring or I2C ADDR!\n\r");
    while(1);
  }

  /* Display some basic information on this sensor */
  displaySensorDetails();
  
  /* Setup the sensor gain and integration time */
  configureSensor();
 
  /* We're ready to go! */
}

static long getTSL2561(void){
  /* Get a new sensor event */ 
  sensors_event_t event;
  tsl.getEvent(&event);
 
  /* Display the results (light is measured in lux) */
  if(event.light){
    return (long)(event.light);
  }else{
    /* If event.light = 0 lux the sensor is probably saturated
       and no reliable data could be generated! */
    printf("Sensor overload\n\r");
    return 0;
  }
}


/*------------------------------- DS18B20 (Temp) -------------------------------*/

static float getDS18B20(void){
  //returns the temperature from one DS18S20 in DEG Celsius

  byte data[12];
  byte addr[8];

  if ( !DS18B20.search(addr)) {
      //no more sensors on chain, reset search
      DS18B20.reset_search();
      return -1000;
  }

  if ( OneWire::crc8( addr, 7) != addr[7]) {
      printf("DS18B20 CRC is not valid!\n");
      return -1000;
  }

  if ( addr[0] != 0x10 && addr[0] != 0x28) {
      printf("DS18B20 is not recognized\n");
      return -1000;
  }

  DS18B20.reset();
  DS18B20.select(addr);
  DS18B20.write(0x44,1); // start conversion, with parasite power on at the end

  byte present = DS18B20.reset();
  DS18B20.select(addr);    
  DS18B20.write(0xBE); // Read Scratchpad

  
  for (int i = 0; i < 9; i++) { // we need 9 bytes
    data[i] = DS18B20.read();
  }
  
  DS18B20.reset_search();
  
  byte MSB = data[1];
  byte LSB = data[0];

  float tempRead = ((MSB << 8) | LSB); //using two's compliment
  float TemperatureSum = tempRead / 16;
  
  return TemperatureSum;
  
}