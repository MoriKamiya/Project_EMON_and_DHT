// EmonLibrary examples openenergymonitor.org, Licence GNU GPL V3
#include "EmonLib.h"                   // Include Emon Library
EnergyMonitor emon1;                   // Create an instance
unsigned long timeStamp = 0;
double Irms = 0;
unsigned int i = 0;

void setup()
{  
  Serial.begin(9600);
  emon1.current(0, 44.11764);             // Current: input pin, calibration.
  for (i = 0 ; i <= 9 ; i++)
    Irms = emon1.calcIrms(607);
}

void loop()
{
  Irms = (Irms + emon1.calcIrms(607)) / 2;  // Calculate Irms only
  if(millis() - timeStamp >= 1000) {
    timeStamp = millis();
    //JSON
      Serial.print("{");
        Serial.print("\"appendPower\":\"" + String(Irms * 110, 2) + "\",");
        Serial.print("\"current\":\"" + String(Irms, 2) + "\"");
      Serial.print("}");
      Serial.print("\n");
    //JSON EOF
  }
}
