struct EEPROMDataObject loadEEPROMDataObject()
{
  int currentAddress = 0;
  EEPROMDataObject mostRecent;
  mostRecent.address = 0;
  mostRecent.dataToSave.meter = 0;
  mostRecent.dataToSave.lowThreshold = 0;
  mostRecent.dataToSave.highThreshold = 0;
  mostRecent.lastEEPROMSaveMillis = 0;
  DataToSave pulledFromEEPROM;

  do
  {
    EEPROM.get(currentAddress, pulledFromEEPROM);
   
    if (pulledFromEEPROM.meter >= mostRecent.dataToSave.meter)
    {
      mostRecent.dataToSave.meter = pulledFromEEPROM.meter;
      mostRecent.dataToSave.lowThreshold = pulledFromEEPROM.lowThreshold;
      mostRecent.dataToSave.highThreshold = pulledFromEEPROM.highThreshold;
      mostRecent.address = currentAddress;
    }

    currentAddress = getNextAdress(currentAddress, sizeof(mostRecent.dataToSave));

  } while (currentAddress != 0);

  return mostRecent;
}

struct EEPROMDataObject saveEEPROMDataObject(struct EEPROMDataObject a)
{
  EEPROM.put(a.address, a.dataToSave);
  a.address = getNextAdress(a.address, sizeof(a.dataToSave));
  a.lastEEPROMSaveMillis = millis();
  sendSensorValue(19, a.address);
  return a;
}

int getNextAdress(int lastAddress, int EEPROMObjectSize)
{
  int nextAddress = lastAddress + EEPROMObjectSize;
  if (nextAddress+EEPROMObjectSize > EEPROM.length())
  {
    nextAddress = 0;
  }
  return nextAddress;
}
