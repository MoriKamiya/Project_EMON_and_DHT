#include <DHT.h>
#include <DHT_U.h>
#include <EmonLib.h>

/****************************************************************
    Project: Current Transformer - AC Current measure.
***************************************************************/

//Configuration
//Please change these value when you bulid this project
#define HISPEEDMODE 0 // If you flagged this value, current measure will more precisely. 
                      // Need define POWER_SYSTEM_FREQ if you flagged! 
#define POWER_SYSTEM_FREQ 60 // Input 50 or 60 that is your AC power system's frequency.
#define MODULE_COUNT 7 // What Module is you have
#define CAIL 44.11764 // CT truns / Rv    = Cailbration 
                      // 3000trun / 68Ohm = 44.11764
#define BANDRATE 9600 //Serial port's BANDRATE

//Constant value, please don't change if you doesn't know they excatlly do.
#define DEBUG 1
#if DEBUG == 1
    #define DEBUG_JSON_SHOW 1
    #define DEBUG_MESSAGE_SENDTIME 1
#else
    #define DEBUG_JSON_SHOW 0
    #define DEBUG_MESSAGE_SENDTIME 0
#endif
#define RAW_MESSAGE_SEND_PER_TIME 1000
#if HISPEEDMODE == 1  //Define Hispeed samlple rate when absolutely freq is known and set.

//Universal num for 50Hz and 60Hz power system on Arduino NANO Rev3.
//If use this Param. to be the emon.sampling, current measure function will need 100ms to run. 
//This Sample rate will dropped down the MCU's speed compare than native 60Hz/50Hz args.
//So if you need Hardware's exec. speed, try SAMPLING60HZ or SAMPLING50HZ.
    #if POWER_SYSTEM_FREQ == 60
        #define SAMPLING 101
    #else if POWER_SYSTEM_FREQ == 50
        #define SAMPLING 122
    #else
        #error Power system frequency is wrong!
    #define MESSAGE_SEND_PER_TIME RAW_MESSAGE_SEND_PER_TIME
#else 
    #define SAMPLING 607
    #define MESSAGE_SEND_PER_TIME RAW_MESSAGE_SEND_PER_TIME -50
#endif

//instance
EnergyMonitor emon[MODULE_COUNT];

//Usage variable
unsigned long timeStamp = 0;
unsigned int i = 0; // Reversed to loop count.
unsigned char j,k = 0; //Reversed to loop count.
double Irms[MODULE_COUNT] = {};

void setup() {
    Serial.begin(BANDRATE);
    for(i = 0 ; i < MODULE_COUNT ; i++)
        emon[i].current(i, CAIL);
    
    for(i = 0 ; i < MODULE_COUNT ; i++){
        for(j = 0 ; j <= 3 ; j++){
            Irms[i] = emon[i].calcIrms(SAMPLING);
        }
    }
    if(DEBUG) Serial.print("Setup Complete.\n");  
}

void loop() {
    
    for(i = 0 ; i < MODULE_COUNT ; i++){
        if(millis() - timeStamp >= MESSAGE_SEND_PER_TIME){ //Will send JSON every set time unit.
            if(DEBUG && DEBUG_MESSAGE_SENDTIME) Serial.print("Send JSON time loop is: \n" + String(millis() - timeStamp) + "\n");
            timeStamp = millis();
            if(DEBUG) Serial.print("Current rms_" + String(i) + " is: " + String(Irms[i]) + "\n");
            
            //***********************Send JSON**********************
            if(DEBUG && DEBUG_JSON_SHOW) Serial.print("JSON String: \n");
            else if(DEBUG && DEBUG_JSON_SHOW == 0) break;
            String str = "";
            Serial.print("{"); // Starter of JSON File.

            for(j = 0 ; j < MODULE_COUNT ; j++){
                str = "\"CT_" + String(j) + "\":" + 
                    "{\"apparentPower\":\"" + String(Irms[j] * 110) + "\"," + "\"currentRMS\":\"" + String(Irms[j]) + "\"}";
                Serial.print(str);
                if(j != MODULE_COUNT - 1) Serial.print(","); // Last entry cannot have , at the end.

                /* Seems like
                                {
                                    "apparentPower": "110",
                                    "currentRMS": 1
                                },
                */
            }

            Serial.print("}"); // EOF of JSON File.
            if(DEBUG && DEBUG_JSON_SHOW) Serial.print("\n");
            //**********************END SEND JSON**********************
            
        }
        Irms[i] = emon[i].calcIrms(SAMPLING); // Get Current RMS.
    }
}
