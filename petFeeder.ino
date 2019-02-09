/*
  This is an example sketch to demontrate a possible use of the IOTAppStory.com infrastructure

  This sketch is based on:
  WebAppToggleBtn V3.0.0  By Onno Dirkzwager      https://github.com/iotappstory/ESP-Library/tree/master/examples/WebAppToggleBtn
  NTPtimeESP example sketch for NTPtimeEsp lib    https://github.com/SensorsIot/NTPtimeESP

  Copyright (c) [2017] [Christiaan Broeders]

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.

  Don't forget to upload the contents of the data folder with MkSPIFFS Tool ("ESP8266 Sketch Data Upload" in Tools menu in Arduino IDE)
  or you can upload the file to IOTAppStory.com when using the OTA functionality
*/


#define COMPDATE __DATE__ __TIME__                    // compiling timestamp
#define MODEBUTTON 0                                  // Button pin on the esp for selecting modes. D3 for the Wemos!



#include <IOTAppStory.h>

#if defined ESP8266
  #include <ESPAsyncTCP.h>                            // https://github.com/me-no-dev/ESPAsyncTCP
#elif defined ESP32
  #include <AsyncTCP.h>                               // https://github.com/me-no-dev/AsyncTCP
  #include <SPIFFS.h>
#endif
IOTAppStory IAS(COMPDATE,MODEBUTTON);                 // Initialize IotAppStory

#include <ESPAsyncWebServer.h>                        // https://github.com/me-no-dev/ESPAsyncWebServer

#include <NTPtimeESP.h>                               // https://github.com/SensorsIot/NTPtimeESP
//  #define DEBUG_ON                                  // uncomment for NTPtime debugging

//  uncomment the ntp server pool for your part of the world if u have timeout problems
//  for information on ntp servers look here --> http://support.ntp.org/bin/view/Servers/NTPPoolServers
NTPtime NTPSrvr("pool.ntp.org");                  // worldwide NTP server pool
//  NTPtime NTPSrvr("nl.pool.ntp.org");               // Dutch NTP server pool
//  NTPtime NTPSrvr("ch.pool.ntp.org");               // Swiss NTP server pool, for improved accuracy :)
//  NTPtime NTPSrvr("europe.pool.ntp.org");           // European NTP server pool
//  NTPtime NTPSrvr("asia.ntp.org");                  // Asian NTP server pool
//  NTPtime NTPSrvr("north-america.pool.ntp.org");    // North American NTP server pool
//  NTPtime NTPSrvr("oceania.ntp.org");               // Oceanian NTP server pool
//  NTPtime NTPSrvr("south-america.pool.ntp.org");    // South American NTP server pool

strDateTime dateTime;                                 // create dateTime object

#include <Servo.h>
Servo feederServo;                                    // create servo object to control a servo

AsyncWebServer server(80);                            // Initialize AsyncWebServer

void onRequest(AsyncWebServerRequest *request){
  //Handle Unknown Request, return 404, page not found
  request->send(404);
}


// ================================================ VARS =================================================

bool runServo = true;                       //  should servo run?
bool servoConfigSet = false;                //  make sure we have assigned a pin to the servo object before we call feederServo.write()
int timeDuringDegr = 10;                    //  delay in ms between servo movements to slow it down a little

String setFeedTimes[maxFeedTimes];          //  store the times as txt to compare against the clock
const int maxFeedTimes = 8;

int eepromSize = 4096;                      //  EEPROM
int nrOfTimesAddr = 1000;;                  //  EEPROM
int durationAddr = 1001;;                   //  EEPROM
int motionAddr = 1002;;                     //  EEPROM
int timesDataArrd = 1003;;                  //  EEPROM

//  IAS config menu variables
char* servoPin          = "12";             //  servopin var to use after config
char* TZOffset          = "0.0";            //  select the timezone
char* dayLightSaveTime  = "1";              //  implement daylight saving time (EU = 1 US = 2)

                                            //  time function vars
unsigned long timer;
unsigned long now;
int daysSinceTimeUpdate = 0;
int actualday;
int actualHour;
int actualMinute;
int actualsecond;
                                            //  time function vars

// ================================================ functions ================================================

int nrOfTimes(void);      //  check in eeprom how many different times have been set
void timeCheck(void);     //  set clock with NTP server
void clockCycle(void);    //  add a second to the clock
String GetEeprom();       //  load all settings from eeprom



// ================================================ SETUP ================================================
void setup() {
  IAS.preSetDeviceName("feeder");                           //  preset Boardname this is also your MDNS responder: http://webtoggle.local
  Serial.print(IAS.eepFreeFrom);                            // uncomment to see the last address or eeprom used for IAS config settings
  IAS.preSetAutoUpdate(false);                              // automaticUpdate (true, false)
  IAS.preSetAutoConfig(false);                              // automaticConfig (true, false)
  //IAS.addField(updTimer, "Update timer:Turn on", 1, 'C'); // These fields are added to the config wifimanager and saved to eeprom. Updated values are returned to the original variable.

  IAS.addField(dayLightSaveTime, "daylight saving time", 1, 'n');
  IAS.addField(TZOffset, "time zone", 3, 'z');              // reference to org variable | field label value | max char return | Optional "special field" char
  IAS.addField(servoPin, "Servo Pin", 2, 'P');


  // You can configure callback functions that can give feedback to the app user about the current state of the application.
  // In this example we use serial print to demonstrate the call backs. But you could use leds etc.
  IAS.onModeButtonShortPress([]() {
    Serial.println(F(" If mode button is released, I will enter in firmware update mode."));
    Serial.println(F("*-------------------------------------------------------------------------*"));
  });

  IAS.onModeButtonLongPress([]() {
    Serial.println(F(" If mode button is released, I will enter in configuration mode."));
    Serial.println(F("*-------------------------------------------------------------------------*"));
  });

  IAS.begin('P');                                           // Optional parameter: What to do with EEPROM on First boot of the app? 'F' Fully erase | 'P' Partial erase(default) | 'L' Leave intact

  if(!SPIFFS.begin()){
      Serial.println(F(" SPIFFS Mount Failed"));
      return;
  }

  EEPROM.begin(eepromSize);

  server.on("/load", HTTP_POST, [](AsyncWebServerRequest *request){
    EEPROM.begin(eepromSize);
    int duration = EEPROM.read(durationAddr);
    int motion = EEPROM.read(motionAddr);
    String setChars = GetEeprom();
    String hrStr ="";
    String minStr ="";
    String secStr ="";
    int strLngth = setChars.length();
    String jsonTimes = "";
      for(int i=0; i<strLngth;){
        String thisTime="";
         for(int ii=0; ii<5;ii++){
            thisTime +=setChars[i];
            i++;
         }
         jsonTimes+= "\"";
         jsonTimes +=thisTime;
         jsonTimes+= "\",";
      }
      hrStr +="\"";
    if(actualHour == 0){
        hrStr +="0";
        }else{
        hrStr += String(actualHour);
        }
    if(actualMinute == 0){
        minStr += "0";
        }else{
        minStr += String(actualMinute);
        }
    if(actualsecond == 0){
        secStr += "0";
        }else{
        secStr += String(actualsecond);
        }
     secStr +="\"";
    if(jsonTimes.endsWith(",")){
      jsonTimes.remove(jsonTimes.length()-1);
      }
    String json = "{";
    json += "\"time\":"+hrStr+"."+minStr+"."+secStr;
    json += ",\"duration\":"+String(duration);
    json += ",\"motion\":"+String(motion);
    json += ",\"feedTimes\":[";
  //  Serial.println(jsonTimes);                //  uncomment to view json output in serial console
    json += jsonTimes;
    json += "]}";
    Serial.println(json);
    request->send(200, "text/json", json);      //  respond with 200  https://httpstatuses.com/200
    json = String();
  });

  server.on("/feedTime", HTTP_POST, [](AsyncWebServerRequest *request){
    int nrOfArgs;
    String eepromTimeVal="";
      if (request->args() > 0 ) {
        nrOfArgs = request->args();
        int nrInArr = 0;
        for ( uint8_t i = 0; i < request->args(); i++ ) {
          if (request->argName(i) == "feedTime[]") {
            setFeedTimes[nrInArr] = request->arg(i);
            eepromTimeVal +=setFeedTimes[nrInArr];
            nrInArr++;
          }else{
            }
       }
       eepromTimeVal +=",";
    }else{
      eepromTimeVal="";
      }
    SetEeprom(nrOfArgs,eepromTimeVal,timesDataArrd);
     request->send(205, "", "");                   //  respond with 205  https://httpstatuses.com/205
  });

  server.on("/conf", HTTP_POST, [](AsyncWebServerRequest *request){
    int nrOfArgs;
    int duration;
    int degrOfMotion;
      if (request->args() > 0 ) {
        nrOfArgs = request->args();
        int nrInArr = 0;
        for ( uint8_t i = 0; i < request->args(); i++ ) {
            if (request->argName(i) == "duration") {
              duration = request->arg(i).toInt();
            }else if(request->argName(i) == "angle"){
              degrOfMotion  = request->arg(i).toInt();
            }
          }
      }
    SetConfEeprom(duration,degrOfMotion);
    request->send(205, "", "");                       //  respond with 205  https://httpstatuses.com/205
  });

  server.on("/Override", HTTP_POST, [](AsyncWebServerRequest *request){
      if (request->args() > 0 ) {
        for ( uint8_t i = 0; i < request->args(); i++ ) {
            if (request->argName(i) == "Override") {
               feedem();
            }
          }
      }
    request->send(205, "", "");                       //  respond with 205  https://httpstatuses.com/205
  });
  server.serveStatic("/", SPIFFS, "/");
  server.onNotFound(onRequest);
  // start the HTTP server
  server.begin();
  Serial.print(F(" HTTP server started at: "));
  Serial.println(WiFi.localIP());
  Serial.println("");
  timer = millis();
  timeCheck();
}



// ================================================ LOOP =================================================
void loop() {
 IAS.loop(); // this routine handles the calling home functionality and reaction of the MODEBUTTON pin. If short press (<4 sec): update of sketch, long press (>7 sec): Configuration
  now = millis();
  //overflow variable reset functie
  if((now - timer) < 0 ) {
    timer = millis();
    Serial.println("line timer reset");
  }
  if( (now - timer) > 1000){
   timer = millis();
   clockCycle();
  }
}

// ================================================ EXTRA FUNCTIONS ================================================

void feedem(){
  if(servoConfigSet){
     //get eeprom
     Serial.println("Triggered!");
      feederServo.attach(atoi(servoPin));           //  atach the pin selected in the IAS config menu to the servo
      EEPROM.begin(eepromSize);
      int duration = EEPROM.read(durationAddr);
      int motion = EEPROM.read(motionAddr);
      int fwdPos = 90-(motion/2);
      int bckPos = 90+(motion/2);
      unsigned long keepRunning = (duration*1000);
      int travelTime = motion * timeDuringDegr;
      int nrOfMoves = (keepRunning/travelTime);
      bool frstMove = true;
      for(int i=0;i<nrOfMoves;i++){
              if(frstMove == true){
                 frstMove = false;
                 feederServo.write(fwdPos);
                 delay(travelTime);
              }else{
                 frstMove = true;
                 feederServo.write(bckPos);
                 delay(travelTime);
              }
      }
      feederServo.write(0);
  }else{
    Serial.println("configure servo settings first!");
    }
}

// ===================== PUT YOUR TIME BASED TRIGGER EVENTS HERE ===========================                  <----
void checkTimedEvents(){
  String timeString;
  int nrOfPresets = nrOfTimes();
  String preSetTimes[nrOfPresets];
  String preSetEeprom = GetEeprom();
  int arrCount = 0;
  int strLngth = preSetEeprom.length();
     for(int i=0; i<strLngth;){
       String thisTime="";
        for(int ii=0; ii<5;ii++){
           thisTime +=preSetEeprom[i];
           i++;
        }
        preSetTimes[arrCount] +=thisTime;
        arrCount++;
      }

  if(actualHour < 10){
    timeString +="0";
    }
  timeString += String(actualHour)+":";
  if(actualMinute < 10){
    timeString +="0";
    }
  timeString +=String(actualMinute);
    for (int i = 0; i <= nrOfPresets; i++) {
        if (timeString == preSetTimes[i]) {
         // Serial.println("feed moment!!!");
          feedem();
        }else{
         // Serial.println("not a feed moment");
          }
     }
}

//================           EEPROM           =====================
int nrOfTimes(){
  EEPROM.begin(eepromSize);
  int val = EEPROM.read(nrOfTimesAddr);
 // Serial.println(val);
  return val;
  }
void SetEeprom(int nrOfArgs, String eepromString, int startAdrr){
    int i=0;
    int lngthOfStr = 5;
    int strLenght = ((nrOfArgs * lngthOfStr )+1);
    int endOfblock = strLenght+startAdrr;
    EEPROM.begin(eepromSize);
    EEPROM.write(nrOfTimesAddr,nrOfArgs);
   for(int address=startAdrr; address<endOfblock; address++) {
    char value = eepromString.charAt(i);
    // read a byte from the current address of the EEPROM
    EEPROM.write(address,value);
    i++;
   }
  EEPROM.commit();
  delay(200);
}



void SetConfEeprom(byte duration, byte motion){
    EEPROM.begin(eepromSize);
      if(motion >180){
        motion =180;
      }
      if(duration >254){
      duration =254;
      }
    EEPROM.write(durationAddr,duration);
    EEPROM.write(motionAddr,motion);
    EEPROM.commit();
  Serial.println("confEeprom set");
  servoConfigSet = true;
}

String GetEeprom(){
  EEPROM.begin(eepromSize);
  int nrOfStr = nrOfTimes();
  int lngthOfStr = 5;
  int currBlockLnght = (lngthOfStr*nrOfStr);
  int address = timesDataArrd;
  int endOfblock = address+currBlockLnght;
  char value;
  String returnVal ="";

   for(int i=address; i<endOfblock; i++) {
    // read a byte of EEPROM
    value = EEPROM.read(address);
    returnVal += value;
    address++;
   }
   return returnVal;
}

void clockCycle(){                            //  gets called every 1000 milliSeconds to update the clock and check if something needs to be done at this time
  actualsecond = (actualsecond +1);
  if(actualsecond >= 60){
    actualsecond = 0;
    actualMinute = (actualMinute +1);
    checkTimedEvents();                       //   checkTimedEvents();
    }
  if(actualMinute >= 60){
    actualMinute = 0;
    actualHour = (actualHour +1);
    }
    if(actualHour >= 24){
    actualHour = 0;
    actualday++;
    daysSinceTimeUpdate++;
    }
    if(daysSinceTimeUpdate >= 2){
    timeCheck();                                //  reset the clock every 2 days to stay current
    }
  }

void timeCheck(){                               //  check in at the NTP server to keep current
  // first parameter: Time zone in floating point (for India); second parameter: 1 for European summer time; 2 for US daylight saving time
  dateTime = NTPSrvr.getNTPtime(atof(TZOffset), 1);
  if(!dateTime.valid){
    Serial.println("dateTime was invalid!");    //  ntp library is a little sketchy after the last update     #TODO
    delay(200);
    timeCheck();
    }else{
      NTPSrvr.printDateTime(dateTime);
      actualday =dateTime.day;
      actualHour = dateTime.hour;
      actualMinute = dateTime.minute;
      actualsecond = dateTime.second;
      daysSinceTimeUpdate = 0;
      }

}
