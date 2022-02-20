#include <RH_ASK.h>
#include <SPI.h> // Not actually used but needed to compile
#include <EEPROM.h>

struct RFSensorData
{
  int sensorID;
  float sensorValue;
};

struct DataToSave
{
  int lowThreshold;
  int highThreshold;
  float meter;
};

struct EEPROMDataObject
{
  int address;
  unsigned long lastEEPROMSaveMillis;
  DataToSave dataToSave;
};

//EEPROM storage variables
EEPROMDataObject EEPROMData;
const unsigned long minimumEEPROMSaveInterval = 900000; // save to EEPROM every 90000 milliseconds = 15 minutes
float lastSavedMeter;

//LED
const byte ledPin = 13;

//RF module
const byte rfPin = 6;

// RF TRANSMITTER (data)
RH_ASK driver = RH_ASK(1000, 11, rfPin);
unsigned long lastWirelessTransmissionMillis = 0;
unsigned long wirelessTransmissionInterval = random(9000, 11000);
boolean alternatingTransfer;

//CNY70
//1000 Ohm pull down resistor on receptor
//1000 Ohm resistor on ir emitter
const byte cny70LedPin = 5;
const byte cny70Analog = A0;

//PROGRAM VARIABLES
unsigned long lastReadingMillis;
const unsigned long readingInterval = 10;
const unsigned int slidingAverageReadings = 20;
int reading;
const unsigned long calibrationDuration = 60000; //calibrate during 60 seconds
boolean state = LOW;
int temp1;
int temp2;
const unsigned long minimumDelayBetweenPulses = 5000;
unsigned long lastPulseMillis = 0;
unsigned long previousPulseMillis = 0;
float currentUsage;
const unsigned long resetCurrentUsageAfterMillis = 300000; //resets currentUsage after 5 minutes of inactivity

void setup() {
  EEPROMData.dataToSave.lowThreshold = 275;
  EEPROMData.dataToSave.highThreshold = 325;
  pinMode(cny70LedPin, OUTPUT);
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, state);
  driver.init();
  //Read last saved entry in EEPROM
  EEPROMData = loadEEPROMDataObject();
  calibrate();
}

void loop() {
  performReading();
  sendWirelessData();
  resetCurrentUsage();


  if ((millis() - EEPROMData.lastEEPROMSaveMillis) > minimumEEPROMSaveInterval &&  lastSavedMeter != EEPROMData.dataToSave.meter)
  {
    EEPROMData = saveEEPROMDataObject(EEPROMData);
    lastSavedMeter = EEPROMData.dataToSave.meter;
  }
}

void performReading()
{
  //Get reading
  if ((millis() - lastReadingMillis) >= readingInterval)
  {
    lastReadingMillis = millis();
    digitalWrite(cny70LedPin, LOW);
    delay(1);
    temp1 = analogRead(cny70Analog);

    digitalWrite(cny70LedPin, HIGH);
    delay(1);
    temp2 = analogRead(cny70Analog);
    //Save some power by powering down the led
    digitalWrite(cny70LedPin, LOW);

    if (temp2 > temp1)
    {
      if ((temp2 - temp1) > reading)
      {
        reading += ((temp2 - temp1) - reading) / slidingAverageReadings;
      }

      if ((temp2 - temp1) < reading)
      {
        reading -= (reading - (temp2 - temp1)) / slidingAverageReadings;
      }
    }
  }


  //Change state if the reading is lower than the low threshold and the current state is  high
  if (reading < EEPROMData.dataToSave.lowThreshold && state && (millis() - lastPulseMillis) > minimumDelayBetweenPulses)
  {
    state = false;
    digitalWrite(ledPin, state);
  }

  //If the reading is higher than the high threshold and the current state is low
  if (reading > EEPROMData.dataToSave.highThreshold && !state && (millis() - lastPulseMillis) > minimumDelayBetweenPulses)
  {

    EEPROMData.dataToSave.meter += 0.01f;
    state = true;
    digitalWrite(ledPin, state);
    //Calculate usage per minute 60 seconds * 1000 millis * 10 literperpulse / elapsed millis = litre of gas per minute
    float elapsedMillis = millis() - lastPulseMillis;
    currentUsage = (600000.0f) / elapsedMillis;
    //Set lastPulseMillis
    previousPulseMillis = lastPulseMillis;
    lastPulseMillis = millis();
  }
}

void resetCurrentUsage()
{
  if ((millis() - lastPulseMillis) > resetCurrentUsageAfterMillis || millis() - lastPulseMillis > ((lastPulseMillis - previousPulseMillis)*2))
  {
    currentUsage = 0.0f;
  }
}


void calibrate()
{
  int lowestReading = 0;
  int highestReading = 0;
  unsigned long startCalibrationMillis = millis();
  unsigned long newLowThreshold = 0;
  unsigned long newHighThreshold = 0;


  //Set startpoint for reading
  digitalWrite(cny70LedPin, LOW);
  delay(1);
  temp1 = analogRead(cny70Analog);

  digitalWrite(cny70LedPin, HIGH);
  delay(1);
  temp2 = analogRead(cny70Analog);

  reading = temp2 - temp1;
  lowestReading = reading;
  highestReading = reading;


  //Test of one minute
  while ((millis() - startCalibrationMillis) < calibrationDuration)
  {
    if ((millis() - lastReadingMillis) >= readingInterval)
    {
      lastReadingMillis = millis();
      digitalWrite(cny70LedPin, LOW);
      delay(1);
      temp1 = analogRead(cny70Analog);

      digitalWrite(cny70LedPin, HIGH);
      delay(1);
      temp2 = analogRead(cny70Analog);

      if (temp2 > temp1)
      {
        if ((temp2 - temp1) > reading)
        {
          reading += (temp2 - temp1) / slidingAverageReadings;
        }

        if ((temp2 - temp1) < reading)
        {
          reading -= (temp2 - temp1) / slidingAverageReadings;
        }
      }
      if (reading > highestReading)
      {
        highestReading = reading;
      }

      if (reading < lowestReading)
      {
        lowestReading = reading;
      }
    }
  }

  newLowThreshold = lowestReading + ((highestReading - lowestReading) * 50 / 100);
  newHighThreshold = highestReading - ((highestReading - lowestReading) * 25 / 100);

  if (newHighThreshold - newLowThreshold > 100)
  {
    digitalWrite(cny70LedPin, LOW);
    EEPROMData.dataToSave.lowThreshold = newLowThreshold;
    EEPROMData.dataToSave.highThreshold = newHighThreshold;
    for (int i = 0 ; i < EEPROM.length() ; i++)
    {
      EEPROM.write(i, 0);
    }
    EEPROMData = saveEEPROMDataObject(EEPROMData);
  }
  else
  {
    digitalWrite(cny70LedPin, HIGH);
  }

  for(int i = 0; i < 5; i++)
  {
    sendSensorValue(19, EEPROMData.dataToSave.lowThreshold);
    delay(1000);
    sendSensorValue(20, EEPROMData.dataToSave.highThreshold);
    delay(1000);
  }

}

