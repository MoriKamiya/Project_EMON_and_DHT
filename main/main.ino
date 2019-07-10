#include <DHT.h>
#include <DHT_U.h>
#include <EmonLib.h>

/****************************************************************
    Project: Current Transformer - AC Current RMS measure.
    Author: MoriKamiya
***************************************************************/

/* Configuration */
//Please change these value when you bulid this project
    #define HISPEEDMODE 1 // If you flagged this value, current measure will more precisely. 
                          // Need define IN_POWER_SYSTEM_FREQ if you flagged! 
    #define IN_SAMPLING_UNIVERSAL 585
    #define IN_SAMPLING_60HZ 86
    #define IN_SAMPLING_50HZ 100
    #define IN_POWER_SYSTEM_FREQ 60 // Input 50 or 60 that is your AC power system's frequency.
    #define IN_MODULE_COUNT 7 // What Module is you have
    #define IN_CAIL 44.11764 // CT truns / Rv    = Cailbration 
                          // 3000trun / 68Ohm = 44.11764
    #define SERIAL_BANDRATE 9600 //Serial port's bandrate
/* End of Configuration */

/* Constant value define */
//Please don't change if you doesn't know they excatlly do!
    #define DEBUG 0
    #if DEBUG == 1
        #define DEBUG_JSON_SHOW 1
        #define DEBUG_MESSAGE_SENDTIME 1
    #else
        #define DEBUG_JSON_SHOW 0
        #define DEBUG_MESSAGE_SENDTIME 0
    #endif
    #define IN_MESSAGE_SEND_PER_TIME 1000
    #if HISPEEDMODE == 1  //Define Hispeed samlple rate when absolutely freq is known and set.

    //Universal num for 50Hz and 60Hz power system on Arduino NANO Rev3.
    //If use this Param. to be the emon.sampling, current measure function will need 100ms to run. 
    //This Sample rate will dropped down the MCU's speed compare than native 60Hz/50Hz args.
    //So if you need Hardware's exec. speed, try let HISPEEDMODE 0
        #if IN_POWER_SYSTEM_FREQ == 60
            #ifndef IN_SAMPLING_60HZ
                #error Power system frequency is wrong!
            #else
                #define SAMPLING IN_SAMPLING_60HZ
            #endif
        #elif IN_POWER_SYSTEM_FREQ == 50
            #ifndef IN_SAMPLING_50HZ
                #error Power system frequency is wrong!
            #else
                #define SAMPLING IN_SAMPLING_50HZ
            #endif
        #else
            #error Power system frequency is wrong!
        #endif    
        #define MESSAGE_SEND_PER_TIME IN_MESSAGE_SEND_PER_TIME
    #else 
        #define SAMPLING 
        #define MESSAGE_SEND_PER_TIME IN_MESSAGE_SEND_PER_TIME -50
    #endif
/* End of Constant define */

//Instance
EnergyMonitor emon[IN_MODULE_COUNT];

//Usage variable
unsigned long timeStamp = 0; 
unsigned int i = 0; // Reversed to loop count.
unsigned char j, k = 0; //Reversed to loop count.
double Irms[IN_MODULE_COUNT] = {}; // Restore current RMS value every different module in here.

//Main program
void setup() {
    Serial.begin(SERIAL_BANDRATE); // Initialize Serial communication
    unsigned long timeMeasure[2] = {}; //Used for debug, will release memory space on setup session end.

    //Setup Instance of emon.current() object.
    for(i = 0 ; i < IN_MODULE_COUNT ; i++) 
        emon[i].current(i, IN_CAIL);
    
    //Trying to NULL measure because first 3~5 times measure will inaccurate.
    //if DEBUG mode, will measure the SAMPLING's correctness
    for(i = 0 ; i < IN_MODULE_COUNT ; i++){ 
        for(j = 0 ; j <= 3 ; j++){
            if(DEBUG) timeMeasure[0] = micros(); //Record 1st time
            Irms[i] = emon[i].calcIrms(SAMPLING); //NULL measure
            if(DEBUG){
                timeMeasure[1] = (timeMeasure[1] + (micros() - timeMeasure[0])) / 2; // Func.s' time = 2nd time - 1st time.
            }
        }
    }
    if(DEBUG){
        Serial.print("Function exec. time is: " + String(timeMeasure[1]) + "us \n");
        if(DEBUG && 
            (HISPEEDMODE && IN_POWER_SYSTEM_FREQ == 60 && (timeMeasure[1] <= 15000 || timeMeasure[1] >= 17332)) || 
            (HISPEEDMODE && IN_POWER_SYSTEM_FREQ == 50 && (timeMeasure[1] <= 19000 || timeMeasure[1] >= 21000)) || 
            (!HISPEEDMODE && (timeMeasure[1] <= 99000 || timeMeasure[1] >= 101000))){
            Serial.print("***WARNING! Sampling value doesn't correct!***\n");
        }
        Serial.print("Setup Complete.\n");  
    }
}

void loop() {
    for(i = 0 ; i < IN_MODULE_COUNT ; i++){
        //Will send JSON every set time unit.
        //If in DEBUG mode then JSON interval will print out too.
        if(millis() - timeStamp >= MESSAGE_SEND_PER_TIME){ 
            if(DEBUG && DEBUG_MESSAGE_SENDTIME) Serial.print("Send JSON time loop is: \n" + String(millis() - timeStamp) + "\n");
            timeStamp = millis();
            if(DEBUG) Serial.print("Current rms_" + String(i) + " is: " + String(Irms[i]) + "\n");
            
        //**********************SEND JSON***********************//
            if(DEBUG && DEBUG_JSON_SHOW) Serial.print("JSON String: \n");
            else if(DEBUG && DEBUG_JSON_SHOW == 0) break;
            String str = "";
            Serial.print("{"); // Starter of JSON File.
            //Put every single module's current RMS into JSON
            for(j = 0 ; j < IN_MODULE_COUNT ; j++){
                str = "\"CT_" + String(j) + "\":" + 
                    "{\"apparentPower\":\"" + String(Irms[j] * 110) + "\"," + "\"currentRMS\":\"" + String(Irms[j]) + "\"}";
                Serial.print(str);
                if(j != IN_MODULE_COUNT - 1) Serial.print(","); // Last entry can't have , at the end.

                /* Seems like
                                CT_*: {
                                    "apparentPower": "110",
                                    "currentRMS": 1
                                },
                                CT_*: { ...
                */
            }

            Serial.print("}"); // EOF of JSON File.
            if(DEBUG && DEBUG_JSON_SHOW) Serial.print("\n");
        //**********************END SEND JSON*******************//
            
        }
        Irms[i] = emon[i].calcIrms(SAMPLING); // Get Current RMS.
    }
}
