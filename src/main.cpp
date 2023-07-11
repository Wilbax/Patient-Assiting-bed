#include <Arduino.h>
#include <SoftwareSerial.h>
#include <DallasTemperature.h>
#include <OneWire.h>




#define bodyTempPin 9
OneWire bTempPin(bodyTempPin);
DallasTemperature bodyTempSensor(&bTempPin);


//create function to read temperature
void readBodyTemp();




//define serial pins as  2, 3
SoftwareSerial mySerial(6, 7); // RX, TX

void setup() {
  mySerial.begin(9600);
  Serial.begin(9600);
  bodyTempSensor.begin();
}


void loop() { 
  readBodyTemp();
}


void readBodyTemp() {
  bodyTempSensor.requestTemperatures();
  mySerial.println(bodyTempSensor.getTempCByIndex(0));
  delay(10);
}