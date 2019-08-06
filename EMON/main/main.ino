#include <EmonLib.h>

/****************************************************************
    Project: Current Transformer - AC Current RMS measure.
    Author: MoriKamiya
***************************************************************/

/* Configuration */
//Please change these value when you bulid this project
    #define HISPEEDMODE 1 // If you flagged this value, current measure will more precisely. 
                          // Need define IN_POWER_SYSTEM_FREQ if you flagged!
    #define IN_MODULE_LINE_CAIL 1 // Use last analogIn pin to measure pull up voltage error and cailbrate it.
    #define IN_MODULE_LINE_CAIL_PIN 6 // Which pin is you use to error cailbrate.
    #define IN_SAMPLING_UNIVERSAL 581
    #define IN_SAMPLING_60HZ 82
    #define IN_SAMPLING_50HZ 100
    #define IN_CITYPOWER_VOLTAGE 110
    #define IN_POWER_SYSTEM_FREQ 60 // Input 50 or 60 that is your AC power system's frequency.
    #define IN_MODULE_COUNT 6 // What Module is you have (include voltage error cailbration pin)
    #define IN_CAIL 44.11764 // CT truns / Rb    = Cailbration 
                          // 3000trun / 68Ohm = 44.11764
    #define SERIAL_BANDRATE 9600 //Serial port's bandrate
/* End of Configuration */

/* Constant value define */
//Please don't change if you doesn't know they excatlly do!
    #define DEBUG 0
    #if DEBUG == 1
        #define DEBUG_JSON_SHOW 0
        #define DEBUG_MESSAGE_SENDTIME 1
        #define DEBUG_IRMS_SHOW 1
    #else
        #define DEBUG_JSON_SHOW 0
        #define DEBUG_MESSAGE_SENDTIME 0
        #define DEBUG_IRMS_SHOW 0
    #endif

    #if IN_MODULE_LINE_CAIL == 1
        #define MODULE_COUNT IN_MODULE_COUNT + 1
    #else
        #define MODULE_COUNT IN_MODULE_COUNT
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
        #define SAMPLING IN_SAMPLING_UNIVERSAL
        #define MESSAGE_SEND_PER_TIME IN_MESSAGE_SEND_PER_TIME -50
    #endif
/* End of Constant define */

/* Define Instance */


//Instance
EnergyMonitor emon[MODULE_COUNT];

//Usage variable
unsigned long timeStamp = 0; 
unsigned int i = 0; // Reversed to loop count.
unsigned char j, k = 0; //Reversed to loop count.
unsigned int averageCount[MODULE_COUNT] = {}; // 
double Irms[MODULE_COUNT] = {}; // Restore current RMS value every different module in here.

//Main program
void setup() {
    Serial.begin(SERIAL_BANDRATE); // Initialize Serial communication
    unsigned long timeMeasure[3] = {}; //Used for debug, will release memory space on setup session end.
    //Setup Instance of emon.current() object.
    for(i = 0 ; i < IN_MODULE_COUNT ; i++) {
        emon[i].current(i, IN_CAIL);
    }
    if(IN_MODULE_LINE_CAIL){
        emon[IN_MODULE_COUNT].current(IN_MODULE_LINE_CAIL_PIN, IN_CAIL);
    }
    boolean ignore1stTime = true;
    for(i = 0 ; i < MODULE_COUNT ; i ++) {    
        for(j = 0 ; j < 5 ; j ++){
            //Trying to find fucntion's exec. time.
            if(DEBUG) timeMeasure[0] = micros(); //Record 1st time
            Irms[i] = emon[i].calcIrms(SAMPLING); //NULL measure to find function exec. time.
            timeMeasure[1] = micros() - timeMeasure[0];
            if (ignore1stTime){
                ignore1stTime = !ignore1stTime;
                break;
            }
            if(DEBUG){
                timeMeasure[2] = ((timeMeasure[2] * i) + timeMeasure[1]) / (i + 1); // Func.s' time = 2nd time - 1st time, and average them.
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
    //Doesn't know why NULL measure block can't effect on setup function
    //Here is initialize block for EMON's calc,
    //Trying to NULL measure because first 3~5 times measure will inaccurate.
    for(i = 0 ; i < MODULE_COUNT ; i ++){ 
        for(j = 0 ; j <= SAMPLING / (char(!HISPEEDMODE * 60) + 1) ; j ++){
            Irms[i] = emon[i].calcIrms(SAMPLING); //NULL measure
        }
    }
    Serial.print("{\"V\":\"" + String(IN_CITYPOWER_VOLTAGE) + "\"}");
    //Real main function block.
    while(1){
        for(i = 0 ; i < MODULE_COUNT ; i++){
            //Will send JSON every set time unit.
            //If in DEBUG mode then JSON interval will print out too.
            if(millis() - timeStamp >= MESSAGE_SEND_PER_TIME){ 
                if(IN_MODULE_LINE_CAIL) {
                    for(j = 0 ; j < IN_MODULE_COUNT ; j ++){
                        Irms[j] = Irms[j] - Irms[MODULE_COUNT - 1];
                        if(Irms[j] < 0.00) Irms[j] = 0.00;
                    }
                }
                if(DEBUG && DEBUG_MESSAGE_SENDTIME) Serial.print("Send JSON time loop is: " + String(millis() - timeStamp) + " ms \n");
                timeStamp = millis();
                if(DEBUG && DEBUG_IRMS_SHOW){
                    for(j = 0 ; j < IN_MODULE_COUNT ; j ++){
                        Serial.print("Irms." + String(j) + ": " + String(Irms[j]) + "\n");
                    }
                }
                for(j = 0 ; j < MODULE_COUNT ; j ++){ // Clear the Current RMS state.
                    averageCount[j] = 0; 
                }
            //**********************SEND JSON***********************//
                if(DEBUG && DEBUG_JSON_SHOW) Serial.print("JSON String: \n");
                else if(DEBUG && DEBUG_JSON_SHOW == 0) break;
                String str = "";
                Serial.print("{"); // Starter of JSON File.
                //Put every single module's current RMS into JSON
                for(j = 0 ; j < IN_MODULE_COUNT ; j++){
                    str = "\"CT_" + String(j) + "\":" + "{"
                        "\"I\":\"" + String(Irms[j]) + "\"}";
                    Serial.print(str);
                    if(j != IN_MODULE_COUNT - 1) Serial.print(","); // Last entry can't have "," at the end.

                    /* Seems like
                                    CT_0: {
                                        "I": 1.00
                                    },
                                    CT_*: { ...
                    */
                }

                Serial.print("}"); // EOF of JSON File.

                if(DEBUG && DEBUG_JSON_SHOW) Serial.print("\n");
            //**********************END SEND JSON*******************//

            }
            #if IN_MODULE_LINE_CAIL
                if(i == MODULE_COUNT - 1){
                    Irms[i] = (Irms[i] * averageCount[i] + emon[i].calcIrms(10)) / (averageCount[i] + 1); 
                }
                //Calculate Irms average basis for second, and try it best.
                Irms[i] = (Irms[i] * averageCount[i] + emon[i].calcIrms(SAMPLING)) / (averageCount[i] + 1); 
                averageCount[i] ++;
            #else
                //Calculate Irms average basis for second, and try it best.
                Irms[i] = (Irms[i] * averageCount[i] + emon[i].calcIrms(SAMPLING)) / (averageCount[i] + 1); 
                averageCount[i] ++;
            #endif
        }
    }
}
