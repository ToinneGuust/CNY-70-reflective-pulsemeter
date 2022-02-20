void sendWirelessData()
{
  if ((millis() - lastWirelessTransmissionMillis) > wirelessTransmissionInterval)
  {
    if (alternatingTransfer)
    {
      sendSensorValue(17, EEPROMData.dataToSave.meter);
    } else {
      sendSensorValue(18, currentUsage);
    }

    alternatingTransfer = !alternatingTransfer;
    wirelessTransmissionInterval = random(4500, 55000);
    lastWirelessTransmissionMillis = millis();
  }
}

void sendSensorValue(int sensorID, float sensorValue)
{
  RFSensorData RFdata =
  {
    sensorID,
    sensorValue
  };
  driver.send((byte*)&RFdata, sizeof(RFdata));
  driver.waitPacketSent();
}






























