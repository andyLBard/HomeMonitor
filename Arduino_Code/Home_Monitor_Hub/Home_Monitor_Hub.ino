//Home Monitor Hub
//Author: Andy Bard
//This code implements the hub/wall panel for my home monitor system.
//The various sensors talk to the hub (Arduino Mega), and the hub
//talks to the web controller, which is the raspberry pi.
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <LiquidCrystal.h>
#include <arduino-timer.h>

//Status Globals
bool ALARM = false;
bool SILENCED = false;
bool SENSOR_ONE_WATCH = false;
bool SENSOR_ONE_WARNING = false;
bool SENSOR_TWO_OUTAGE = false;

const int EMERGENCY_LIGHT = 27;

//lcd is the i2c connection, and it's the one that duplicates the
//sump monitor. lcd0 is the one that connects through the
//normal liquid crystal lib, and that is the one that will
// handle a rough uptime for the system and some status indicators.
LiquidCrystal_I2C lcd(0x26, 16, 2);

/*NOTES:
  This code is dependent upon running on and Arduino Mega.
  2) Wiring: 
  TX1 = 18  / Serial1 = Power Monitor
  RX1 = 19

  TX2 = 16 / Serial2 = Sump Monitor
  RX2 = 17

  Note order flips for serial zero, arduino uno uses the same 0=rx/1=tx
  RX0 = 0
  TX0 = 1 / USB  = Web Controller.
  We will use USB serial to connect to the raspberry pi 
  So that we can power the hub off of the web controller for
  convenience.
*/
// initialize the library by associating any needed LCD interface pin
// with the arduino pin number it is connected to
const int rs = 9, en = 8, d4 = 5, d5 = 4, d6 = 3, d7 = 2;
LiquidCrystal lcd0(rs, en, d4, d5, d6, d7);

//Used for the interrupt, attachInterrupt requires a func
void silence_alarm(){
  SILENCED = true;
}

void setup() {
  //Set up the contrast for lcd0 before setting up lcds
  pinMode(A1, OUTPUT);
  analogWrite(A1, 1); //set contrast here;
  
  //Use an interrupt to handle when we need to silence.
  attachInterrupt(digitalPinToInterrupt(21), silence_alarm, RISING);

  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Starting...");

  lcd0.begin(16,2);
  lcd0.clear();
  lcd0.setCursor(0,0);
  lcd0.print("Starting...");

  //Make sure lights are off at start
 // digitalWrite(EMERGENCY_LIGHT, HIGH);
  
  //Start Serial Connections
  Serial.begin(9600);
  Serial2.begin(9600);
}

const unsigned long secPerMilli = 1000;
const unsigned long minutePerMilli = secPerMilli * 60;
const unsigned long hourPerMilli = minutePerMilli * 60;
const unsigned long dayPerMilli = hourPerMilli * 24;

//Timing Variables
unsigned long prev_stamp = millis();
unsigned long curr_stamp = prev_stamp;
int  poll_freq = 1000; //1s refresh

void updateDisplayZero(){
  unsigned long secs=curr_stamp/1000, mins=secs/60;
  unsigned int hours=mins/60, days=hours/24;
  secs-=mins*60;
  mins-=hours*60;
  hours-=days*24;
  char timeout[16];
  //handle printing days, hours, minutes of uptime.
  snprintf(timeout, 16, "UP: %02dD %02d:%02d", days, hours, mins);
  lcd0.clear();
  lcd0.setCursor(0,0);
  lcd0.print(timeout);
  lcd0.setCursor(0,1);
  lcd0.print("Sens: 2 | WC: +");
}

void updateDisplayOne(){
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("WATCH:   ");
  if(!SENSOR_ONE_WATCH){
    lcd.print("GOOD");
  } else {
    //WET, INSIDE PIT
    lcd.print("BAD!");
  }
  lcd.setCursor(0,1);
  lcd.print("WARNING: ");
  if(!SENSOR_ONE_WARNING){
    lcd.print("GOOD");
  } else {
    //WET, OUTSIDE PIT. BREACH.
    lcd.print("BAD!");
  }
  // //Handle emergency lights
  // if(SENSOR_ONE_WATCH || SENSOR_ONE_WARNING){
  //   digitalWrite(EMERGENCY_LIGHT, HIGH);
  // } else {
  //   digitalWrite(EMERGENCY_LIGHT, LOW);
  // }
}

//This function will handle serial comms to the sump monitor
void updateFromSensorOne(){
  int status_code = 0; // 128 is no alerts, 1 is watch, 2 is warning, 3 is both.
  int request_type = 0; //0 is the request type to just get an update, request 1 silences
  if(SILENCED){
    request_type = 1;
    SILENCED = false;
  }
  if(Serial2.available()){
    status_code = Serial2.parseInt(SKIP_WHITESPACE); //I'm only sending individual lines with numbers back. 
    //Serial.println(status_code);
    
    //This is just a comms blip, do not update to being good.
    if(status_code == 0){
      return;
    }
    
    if(status_code & 1){
      SENSOR_ONE_WATCH = true;
    } else {
      SENSOR_ONE_WATCH = false;
    }
    if(status_code & 2){
      SENSOR_ONE_WARNING = true; 
    } else {
      SENSOR_ONE_WARNING = false;
    }
  } else { //if there is no message from the sump monitor, send a request for update
    Serial2.println(request_type);//request update
  } 
}

//This function will handle comms for the power monitor
void updateFromSensorTwo(){

}

void updateWebController(){
  int controller_request = 0;
  int status_report = 0; // 1 = watch wet, 2 = warning wet, 
  if(Serial.available()){
    Serial.readStringUntil('\n'); //only passing basic ints around, this should work
    //Since the message is kind of irrelevant and is always a status update, we just
    //discard it for now.
    //128 if all is well, 1 for watch wet, 2 for warning wet, 3 for both.
    if(SENSOR_ONE_WATCH) status_report += 1;
    if(SENSOR_ONE_WARNING) status_report +=2;
    char message[10] = "";
    sprintf(message, "%d,%d", status_report, millis()); // Updated hub to act as power monitor, send uptime to the controller.
    Serial.println(message);    
  } 
}

void loop() {
  curr_stamp = millis();
  while(curr_stamp - poll_freq < prev_stamp){

    curr_stamp = millis();
  }
  prev_stamp = curr_stamp;
  updateFromSensorOne();
  updateFromSensorTwo();
  updateWebController();
  updateDisplayZero();
  updateDisplayOne();
}