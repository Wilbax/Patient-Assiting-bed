#define USE_ARDUINO_INTERRUPTS true
#include <Arduino.h>
#include <SoftwareSerial.h>
#include <DallasTemperature.h>
#include <OneWire.h>
#include <PulseSensorPlayground.h>
#include "DHT.h"
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
#include <uRTCLib.h>
#include <SoftwareSerial.h>
#include <HX711_ADC.h>



// Pins fan + Load cell
//DHT pin declared above 
const int HX711_dout = D4;  // MCU > HX711 dout pin
const int HX711_sck = D5;  // MCU > HX711 sck pin
const int RELAYPIN = D1;    // MCU > LED pin


//touch sensors
const int toggleModePin = 11;
const int bedUpPin = 12;
const int bedDownPin = 13;
bool invalid = false;

//motor
const int motorPinUp = 1; //up
const int motorPinDown = 2; //down
unsigned long curentMotorPos = 0;

unsigned long functionStartTime = 0;
unsigned long functionEndTime = 0;


//body temperature sensor
#define bodyTempPin 9
OneWire bTempPin(bodyTempPin);
DallasTemperature bodyTempSensor(&bTempPin);

//heart pulse sensor
PulseSensorPlayground pulseSensor;
const int PulseWire = A2;       // PulseSensor PURPLE WIRE connected to ANALOG PIN 0
const int LED = LED_BUILTIN;          // The on-board Arduino LED, close to PIN 13.
int Threshold = 650;  
int count = 0;
int bpm = 0;
unsigned long lastPulse = 0;
int signal = 0;
unsigned long lastBPM = 0;
int currentBPM = 0;

//DHT sensor
#define DHTPIN 8
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

//RTC
uRTCLib rtc(0x68);

//LCD
LiquidCrystal_I2C lcd(0x27, 16, 2);
uint8_t heart[8] = {0x0,0xa,0x1f,0x1f,0xe,0x4,0x0};


//GSM module
SoftwareSerial mySerial(3, 2);


//function to read temperature
void readBodyTemp();

//fuction to read pulse
void readPulse();

//function to read humidity and room temperature
float readDHT();

void createDisplay();

void printTime();

void sendAlerts(String str);

void sendMessage(String str);

int isWeightDetected();

int overHumidity();

int overTemperature();



void setup() {
  //touch sensors
  pinMode(toggleModePin, INPUT);
  pinMode(bedUpPin, INPUT);
  pinMode(bedDownPin, INPUT);

  //serial connection with NodeMCU
  Serial.begin(9600);
  Serial1.begin(9600);

  //serial connection for gsm module
  mySerial.begin(9600);

  //body temp sensor
  bodyTempSensor.begin();

  //dht sensor
  dht.begin();

  if (isnan(dht.readHumidity())) {
    Serial.println(F("Failed to read from DHT sensor!"));
    //create infinite while loop
    while (1){
      Serial1.println("Connecting to DHT sensor...");
    }
  }

  //display and rtc
  URTCLIB_WIRE.begin();
  lcd.init();
  lcd.backlight();
  lcd.createChar(0, heart);

  //gsm module
  Serial.println("Initializing GSM Module...");
  delay(1000);

  mySerial.println("AT"); //Once the handshake test is successful, it will back to OK
  updateSerial();
  mySerial.println("AT+CMGF=1"); // Configuring TEXT mode
  updateSerial();
  mySerial.println("AT+CMGS=\"+Z94703487817\"");//change ZZ with country code and xxxxxxxxxxx with phone number to sms
  updateSerial();
  mySerial.print("Message send from Patient Assisting Bed.This is a testing message"); //text content
  updateSerial();
  mySerial.write(26);

  LoadCell.begin();
  float calibrationValue;
  calibrationValue = -115.53;

  unsigned long stabilizingtime = 2000;
  boolean _tare = true;
  LoadCell.start(stabilizingtime, _tare);
  if (LoadCell.getTareTimeoutFlag()) {
    Serial.println("Timeout, check MCU > HX711 wiring and pin designations");
    while (1);
    Serial.println("LoadCell Startup is complete");
  } else {
    LoadCell.setCalFactor(calibrationValue);
    Serial.println("Startup is complete");
  }

}


void loop() { 
  

  // createDisplay();
  // // printTime();


  // bpm = 0;
  // if(bpm>0){
  //   float humidity = dht.readHumidity();
  //   float roomTemp = dht.readTemperature();
  //   float bodyTemp = bodyTempSensor.getTempCByIndex(0);
  //   currentBPM = bpm;  //show curent bpm in display

  //   Serial1.println("humidity =" + String(humidity));
  //   Serial1.println("roomTemp=" + String(roomTemp));
  //   Serial1.println("bodyTemp=" + String(bodyTemp));
  //   Serial1.println("bpm=" + String(bpm));
  // }

  // //only check bpm and body temp critical in 5 min intervals
  // if(millis() - lastBPM > 300000){
  //   checkBPM(bpm);
  //   checkBodyTemp(bodyTempSensor.getTempCByIndex(0));
  // } 

  bedLiftingFunction(); 

  //relay + dht to on the fan 
  int isWeight = isWeightDetected();
  int ishumid  = overhumidity();
  int istemp   = overtemperature();
    
  if ((isWeight == 1 && ishumid==1) || (isWeight == 1 && istemp==1)) {
    // Serial.println("both");
    digitalWrite(RELAYPIN, HIGH);
  }

  else {
    // Serial.println("neither");
    digitalWrite(RELAYPIN, LOW);
  }  

}


//read body temperature
void readBodyTemp() {
  bodyTempSensor.requestTemperatures();
  Serial1.println("bTemp=" + String(bodyTempSensor.getTempCByIndex(0)));
  delay(10);
}

//create function to output random hr values without sensor readings
void readPulse() {
  // int pulse = random(60, 100);
  // Serial1.println("pulse = " + String(pulse));
  signal = analogRead(PulseWire);
  if(signal > Threshold && millis() - lastPulse> 300){
    count++;
    lastPulse = millis(); //get the last time a pulse was read
  }
  if(millis() - lastBPM > 60000){
    bpm = count;
    count = 0; //reset count
    lastBPM = millis(); //get the last time a BPM was read
    Serial1.println("bpm=" + String(bpm));
  }
}

float readDHT() {
  // float h = dht.readHumidity();
  // float t = dht.readTemperature();
  //random values for testing
  float h = random(60, 100);
  float t = random(60, 100);

  // Serial1.println("humidity =" + String(h));

  return h,t;
  // Serial1.println("roomTemp=" + String(t));
}


void createDisplay(){
  rtc.refresh();
  lcd.clear();
  lcd.setCursor(1, 0);
  lcd.print(0);
  lcd.setCursor(0, 1);
  lcd.print("Body Temp: " + String(dht.readHumidity()) + "C");

}

//function to output current time to serial monitor
void printTime() {
  rtc.refresh();
  Serial1.println(String(rtc.hour()) + ":" + String(rtc.minute()) + ":" + String(rtc.second()));
}
//function to split string "=" and make it into a key value pair



//function to read serial input string start with "#" only
void sendAlerts(String str){
  if(str.startsWith("#")){
    sendMessage(str);
  }
}


void updateSerial(){
  delay(500);
  while (Serial.available()) {
    mySerial.write(Serial.read());//Forward what Serial received to Software Serial Port
  }
  while(mySerial.available()) {
    Serial.write(mySerial.read());//Forward what Software Serial received to Serial Port
  }
}

//create function to send messages using gsm module
void sendMessage(String str){
  mySerial.println("AT+CMGS=\"+94703487817\"");//change ZZ with country code and xxxxxxxxxxx with phone number to sms
  updateSerial();
  mySerial.print(str); //text content
  updateSerial();
  mySerial.write(26);
  // updateSerial();
}


//create function to identify critical heart rate 
void checkBPM(int bpm){
  if(bpm > 100){
    //send alert
    sendMessage("Patient: Janith heart rate is too high");
  }
  else if(bpm < 60){
    //send alert
    sendMessage("Patient: Janith heart rate is too low");
  }
}

//create function to identify critical body temperature
void checkBodyTemp(float bodyTemp){
  if(bodyTemp > 37.5){
    //send alert
    sendMessage("Patient: Janith Body temperature is too high");
  }
  else if(bodyTemp < 35.5){
    //send alert
    sendMessage("Patient: Janith body temperature is too low");
  }
}

void bedLiftingFunction(){
  if(digitalRead(bedUpPin)==HIGH && digitalRead(motorPinDown)==HIGH){
    invalid = true;
  }else{
    invalid = false;
  }
  if(digitalRead(bedUpPin)==HIGH && !invalid){
    digitalWrite(motorPinUp, HIGH);
  }
  if(digitalRead(motorPinDown)==HIGH && !invalid){
    digitalWrite(motorPinDown, HIGH);
  }
  if(digitalRead(bedUpPin)==LOW){
    digitalWrite(motorPinUp, LOW);
  }
  if(digitalRead(motorPinDown)==LOW){
    digitalWrite(motorPinDown, LOW);
  }
}




int isWeightDetected() {
  const int serialPrintInterval = 1000;

  if (LoadCell.update())
    newDataReady = true;

  // get smoothed value from the dataset:
  if (newDataReady) {
    if (millis() > t + serialPrintInterval) {
      float i = LoadCell.getData();
      if (i > 400.0) {
        return 1;
      }
      newDataReady = false;
      t = millis();
    }
  }
  return 0;
}

//to find over humidity or not
int overHumidity(){
  // delay(10);

  // sensors_event_t eventH;
  // dht.humidity().getEvent(&eventH);
  if (isnan(dht.readHumidity()) || isnan(dht.readTemperature())) {
    Serial.println(F("Error reading humidity!"));
    
  }
  else {
    if(dht.readHumidity()>80.0){
      return 1;
    }
  }
    return 0;
}

int overTemperature(){
  // delay(10);
  // dht.temperature().getEvent(&eventT);
  if (isnan(dht.readHumidity()) || isnan(dht.readTemperature())) {
    Serial.println(F("Error reading temperature!"));
  }
  else {
    if(eventT.temperature>30.0){
      return 1;
    }
  }
    return 0;
}




