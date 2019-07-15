#include <DHT.h>

#define IN_DHT_PIN 2
#define IN_DHT_TYPE DHT22
#define IN_SERIAL_BANDRATE 9600

#define DEBUG 1

float Humidity = 0;
float Temperature = 0;
unsigned long timeStamp = 0;

DHT dht(IN_DHT_PIN, IN_DHT_TYPE);

void setup(){
    Serial.begin(IN_SERIAL_BANDRATE);
    dht.begin();
}

void loop(){
    delay(2000);
    Humidity = dht.readHumidity();
    Temperature = dht.readTemperature();
    if(millis() - timeStamp >= 1000){
        timeStamp = millis();
        if(isnan(Humidity) || isnan(Temperature)){
            Serial.print("{");
                Serial.print("\"FailedToReadDHT\":1");
            Serial.print("}");
        }
        else {
            Serial.print("{");
                Serial.print("\"Humidity\":" + String(Humidity) + ",");
                Serial.print("\"Temperature\":" + String(Temperature));
            Serial.print("}");
        }
    }
}   