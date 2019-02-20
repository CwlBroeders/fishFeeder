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

const int maxFeedTimes = 8;                 //  default set to a max of 8
String setFeedTimes[maxFeedTimes];          //  store the times as txt to compare against the clock

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
unsigned long timer;                        //  unsigned long variables to store and check the millis() value
unsigned long now;
int daysSinceTimeUpdate = 0;                //  we want to reset the clock every once in a while with network time
int actualHour;                             //  we will only need days, hours, minutes and seconds
int actualMinute;                           //  but week month and year are also available, as is the day of the week
int actualsecond;
                                            //  time function vars

// ================================================ functions ================================================

void timeCheck(void);                       //  set clock with NTP server
void clockCycle(void);                      //  add a second to the clock
int nrOfTimes(void);                        //  check in eeprom how many different times have been set
String GetEeprom();                         //  load all set times from eeprom
void SetEeprom();                           //  store all set times in eeprom
void SetConfEeprom(void);                   //  store servo movement and duration data in eeprom
void checkTimedEvents(void);                //  compare our set times against the internal clock
void feedem(void);                          //  move the servo if conf data has been set
// ================================================ SETUP ================================================
void setup() {
  IAS.preSetDeviceName("feeder");                                   // preset Boardname this is also your MDNS responder: http://webtoggle.local
  Serial.print(IAS.eepFreeFrom);                                    // uncomment to see the last address or eeprom used for IAS config settings
  IAS.preSetAutoUpdate(false);                                      // automaticUpdate (true, false)
  IAS.preSetAutoConfig(true);                                       // automaticConfig (true, false)

  IAS.addField(dayLightSaveTime, "daylight saving time", 1, 'N');   // These fields are added to the config wifimanager and saved to eeprom. Updated values are returned to the original variable.
  IAS.addField(TZOffset, "time zone", 4, 'Z');                      // reference to org variable | field label value | max char return | Optional "special field" char
  IAS.addField(servoPin, "Servo Pin", 3, 'P');                      // pick the pin that commands the servo, default is 12


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

  IAS.begin('P');                                                   // Optional parameter: What to do with EEPROM on First boot of the app? 'F' Fully erase | 'P' Partial erase(default) | 'L' Leave intact

  if(!SPIFFS.begin()){
      Serial.println(F(" SPIFFS Mount Failed"));
      return;
  }

  EEPROM.begin(eepromSize);

  server.on("/load", HTTP_POST, [](AsyncWebServerRequest *request){ //  When the webapp is finished loading, a this POST request will be sent to load any stored data from the ESP
    EEPROM.begin(eepromSize);
    int duration = EEPROM.read(durationAddr);                       //  So we get the duration and motion for the servo
    int motion = EEPROM.read(motionAddr);
    String setChars = GetEeprom();                                  //  And we get the times from eeprom using the GetEeprom() function
    String hrStr ="";                                               //  Make some Strings to hold the clock time, so we can sync the webapp time with the ESP clock
    String minStr ="";
    String secStr ="";
    int strLngth = setChars.length();                               //  We need to loop thru setChars, so we need to know how many characters it holds
    String jsonTimes = "";                                          //  jsonTimes will hold the seperate times, we will put this into the JSON response later
      for(int i=0; i<strLngth;){                                    //  our times data was retrieved as follows 00:0011:1122:22, so we need to separate the individual times from this large string
        String thisTime="";                                         //  String to hold one seperate time, so we can add it to jsonTimes later
         for(int ii=0; ii<5;ii++){                                  //  each time has 5 characters
            thisTime +=setChars[i];                                 //  add each of those characters to thisTime
            i++;                                                    //  Note we increment the i variable here, inside the second loop
         }
         jsonTimes+= "\"";
         jsonTimes +=thisTime;                                      //  format jsonTimes and add thisTime
         jsonTimes+= "\",";
      }
      hrStr +="\"";
    if(actualHour == 0){                                            //  now make the String we need to tell our webApp what time the ESP thinks it is
        hrStr +="0";                                                //  note that we add a 0 to the string for all variables if they are 0
        }else{                                                      //  if we dont, the webApp clock display wil be 0:0 instead of 00:00 for midnight
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
    if(jsonTimes.endsWith(",")){                                    //  we added a "," so we could add more values, so now we remove that from the end of the string
      jsonTimes.remove(jsonTimes.length()-1);
      }
    String json = "{";                                              //  Now we concattonate all of our prepared strings into the JSON response to our webApp
    json += "\"time\":"+hrStr+"."+minStr+"."+secStr;                //  For more information on JSON https://developer.mozilla.org/en-US/docs/Learn/JavaScript/Objects/JSON
    json += ",\"duration\":"+String(duration);
    json += ",\"motion\":"+String(motion);
    json += ",\"feedTimes\":[";
  //  Serial.println(jsonTimes);                                    //  uncomment to view jsonTimes output in serial console
    json += jsonTimes;
    json += "]}";
  //  Serial.println(json);                                         //  uncomment to view json response output in serial console
    request->send(200, "text/json", json);                          //  respond with 200  https://httpstatuses.com/200
    json = String();                                                //  the json string is now very large, so we want to clear it to save memory
  });

  server.on("/feedTime", HTTP_POST, [](AsyncWebServerRequest *request){
    int nrOfArgs;
    String eepromTimeVal="";
      if (request->args() > 0 ) {                                   //  the feedtimes are sent as an array
        nrOfArgs = request->args();
        int nrInArr = 0;
        for ( uint8_t i = 0; i < request->args(); i++ ) {           //  so we loop thru them and store the data in a string
          if (request->argName(i) == "feedTime[]") {
            setFeedTimes[nrInArr] = request->arg(i);
            eepromTimeVal +=setFeedTimes[nrInArr];
            nrInArr++;
          }else{                                                    //  if its not a feedTime[], we are not interrested
            }
       }
       eepromTimeVal +=",";
    }else{
      eepromTimeVal="";
      }
    SetEeprom(nrOfArgs,eepromTimeVal,timesDataArrd);
     request->send(205, "", "");                                    //  respond with 205  https://httpstatuses.com/205
  });

  server.on("/conf", HTTP_POST, [](AsyncWebServerRequest *request){ //  the webApp sent us configuration data for the servo movements
    int nrOfArgs;
    int duration;
    int degrOfMotion;
      if (request->args() > 0 ) {
        nrOfArgs = request->args();
        int nrInArr = 0;
        for ( uint8_t i = 0; i < request->args(); i++ ) {           //  put the data in the respective variable
            if (request->argName(i) == "duration") {
              duration = request->arg(i).toInt();
            }else if(request->argName(i) == "angle"){
              degrOfMotion  = request->arg(i).toInt();
            }
          }
      }
      Serial.println(duration);
      Serial.println(degrOfMotion);
    SetConfEeprom(duration,degrOfMotion);                           //  and use the SetConfEeprom() function to store them in eeprom
    request->send(205, "", "");                                     //  respond with 205  https://httpstatuses.com/205
  });

  server.on("/Override", HTTP_POST, [](AsyncWebServerRequest *request){
      if (request->args() > 0 ) {                                   //  the user want to feed now, the webApp sent an override
        for ( uint8_t i = 0; i < request->args(); i++ ) {
            if (request->argName(i) == "Override") {
               feedem();                                            //  So we call the feedem() function to move the servo
            }
          }
      }
    request->send(205, "", "");                                     //  respond with 205  https://httpstatuses.com/205
  });

  server.serveStatic("/", SPIFFS, "/");                             //  server error handling
  server.onNotFound(onRequest);
  // start the HTTP server
  server.begin();
  Serial.print(F(" HTTP server started at: "));
  Serial.println(WiFi.localIP());
  Serial.println("");
  timer = millis();
  timeCheck();                                                      //  now that we have make connection and all is well, call the NTP pool to check the time
}



// ================================================ LOOP =================================================
void loop() {
 IAS.loop();                                                        // The IAS.loop() should be placed at the top of our main loop so any delays from our code dont interfere with IAS functionality
  now = millis();
  //overflow variable reset
  if((now - timer) < 0 ) {
    timer = millis();
    Serial.println(" timer reset ");
  }
  if( (now - timer) > 1000){
   timer = millis();
   clockCycle();                                                    //  more then 1000 milliseconds have passed, add another second to the clockCycle
  }
}

// ================================================ FUNCTIONS ================================================

void feedem(){                                                      //  this is the function that does the actual moving of the servo
  if(servoConfigSet){                                               //  before we move the servo, we need to check if motion and duration have been set
     Serial.println("Triggered!");
      feederServo.attach(atoi(servoPin));                           //  atach the pin selected in the IAS config menu to the servo
      EEPROM.begin(eepromSize);
      int duration = EEPROM.read(durationAddr);
      int motion = EEPROM.read(motionAddr);
      int fwdPos = 90-(motion/2);                                   //  we want our 0 point to be straight down, for our servo this means 90 degrees
      int bckPos = 90+(motion/2);                                   //  therefore we need to add/subtract 90 from the preset, if motion = 30 degrees this would mean the servo will move between 75 and and 105 degrees
      unsigned long keepRunning = (duration*1000);                  //  duration was ment in seconds, so we multiply by 1000
      int travelTime = motion * timeDuringDegr;                     //  we need to keep servo speed into account, so we dont break it, we calculate a desired travelTime
      int nrOfMoves = (keepRunning/travelTime);                     //  and we calculate the number of back and forth movements from the allotted time
      bool frstMove = true;
      for(int i=0;i<nrOfMoves;i++){                                 //  go back and forth for the amount of times we have calculated
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
      feederServo.write(0);                                         //  and return the servo to the start position
  }else{
    Serial.println("configure servo settings first!");              //  we cant move the servo if we dont know how far or for how long
    }
}


void checkTimedEvents(){                                            //  check if one of our saved times equals the current time on the clock
  String timeString;
  int nrOfPresets = nrOfTimes();                                    //  retrieve the number of times that were set
  String preSetTimes[nrOfPresets];                                  //  vreate the array to hold those times
  String preSetEeprom = GetEeprom();                                //  retrieve the times from eeprom as a large string
  int arrCount = 0;
  int strLngth = preSetEeprom.length();
     for(int i=0; i<strLngth;){                                     //  loop thru the large string
       String thisTime="";
        for(int ii=0; ii<5;ii++){                                   //  and seperate the times
           thisTime +=preSetEeprom[i];
           i++;
        }
        preSetTimes[arrCount] +=thisTime;                           //  and store those times in and array to loop thru later
        arrCount++;
      }

  if(actualHour < 10){                                              //  now we take the integer values from our internal clock and convert them to a string so we can compare
    timeString +="0";
    }
  timeString += String(actualHour)+":";
  if(actualMinute < 10){
    timeString +="0";
    }
  timeString +=String(actualMinute);
    for (int i = 0; i <= nrOfPresets; i++) {                        //  and finally we can do the actual checking of the current time against the saved times
        if (timeString == preSetTimes[i]) {
         // Serial.println("feed moment!!!");
          feedem();                                                 //  if its feeding time feedem()
        }else{
         // Serial.println("not a feed moment");
          }
     }
}

//================           EEPROM           =====================
int nrOfTimes(){                                                    //  Simple function to retrieve a single int from eeprom
  EEPROM.begin(eepromSize);
  int val = EEPROM.read(nrOfTimesAddr);
 // Serial.println(val);
  return val;
  }
void SetEeprom(int nrOfArgs, String eepromString, int startAdrr){   //  This function will store the feedtimes we get from our webapp into the eeprom of the ESP
    int i=0;
    int lngthOfStr = 5;
    int strLenght = ((nrOfArgs * lngthOfStr )+1);                   //  dont forget the end of line, as this is a string
    int endOfblock = strLenght+startAdrr;
    EEPROM.begin(eepromSize);
    EEPROM.write(nrOfTimesAddr,nrOfArgs);
   for(int address=startAdrr; address<endOfblock; address++) {      //  loop thru all the characters in the string
    char value = eepromString.charAt(i);
    // read a byte from the current address of the EEPROM
    EEPROM.write(address,value);                                    //  and prepare them for commit to eeprom
    i++;
   }
  EEPROM.commit();
 // delay(200);
}



void SetConfEeprom(byte duration, byte motion){                     //  this function will save the servo configuration data to eeprom
  EEPROM.begin(eepromSize);
    if(motion >180){                                                //  limit to 180, since the servo cant move more then 180 degrees
      motion =180;
    }
    if(duration >254){                                              //  limit to 254, since this is the largest integer we can store in a single byte of eeprom
    duration =254;
    }
  EEPROM.write(durationAddr,duration);
  EEPROM.write(motionAddr,motion);
  EEPROM.commit();                                                  //  commit the data to eeprom
  Serial.println("SetConfEeprom set");
  Serial.print(duration);
  Serial.println(" duration");
  Serial.print(motion);
  Serial.println(" motion");
  servoConfigSet = true;                                            //  and set the servoConfigSet boolean to true, this will permit the feedem() function to move the servo
}

String GetEeprom(){                                                 //  this function will return our set times from eeprom as a large string
  EEPROM.begin(eepromSize);
  int nrOfStr = nrOfTimes();
  int lngthOfStr = 5;
  int currBlockLnght = (lngthOfStr*nrOfStr);
  int address = timesDataArrd;
  int endOfblock = address+currBlockLnght;
  char value;
  String returnVal ="";

   for(int i=address; i<endOfblock; i++) {                          //  loop thru the adresses that contain our data and store it in returnVal
    // read a byte of EEPROM
    value = EEPROM.read(address);
    returnVal += value;
    address++;
   }
   return returnVal;                                                // and return the returnVal
}

void clockCycle(){                            //  gets called every 1000 milliSeconds to update the clock and check if something needs to be done at this time
  actualsecond = (actualsecond +1);
  if(actualsecond >= 60){
    actualsecond = 0;
    actualMinute = (actualMinute +1);
    checkTimedEvents();                       //   checkTimedEvents() gets called every 60 seconds;
    }
  if(actualMinute >= 60){
    actualMinute = 0;
    actualHour = (actualHour +1);
    }
    if(actualHour >= 24){
    actualHour = 0;
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
    Serial.print(".");                          //  ntp library is a little sketchy after the last update, first call always returns invalid      #TODO
    delay(200);
    timeCheck();
    }else{
      Serial.println("ntp time received");
      NTPSrvr.printDateTime(dateTime);
      actualHour = dateTime.hour;               //  update all time variables to keep current with ntp time
      actualMinute = dateTime.minute;
      actualsecond = dateTime.second;
      daysSinceTimeUpdate = 0;                  //  reset to 0, the clock is up to date again
      }

}
