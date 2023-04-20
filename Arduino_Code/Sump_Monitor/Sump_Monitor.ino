//Sump Monitor
//Author: Andy Bard
//This code implements the sump monitor code for the home monitoring system.
//In particular, this handles reading the leak sensors and updating output
//accordingly.
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

//Constants
#define SPKR_PIN 4
#define SILENCE_BTN_PIN 3
//There are two leak sensors, Watch and Warning. The name follows weather advisory rules.
//Watch is the les severe, Warning is more severe.
//For our purposes, this watch will be inside the pit, near the top, and warning will be
//just outside the pit to alarm of an actual breach of water into the home.
#define WATCH_SENSOR 6
#define WARNING_SENSOR 2 //Use two in case we decide to use an interrupt.

//Status Globals
bool ALARM = false;
bool SILENCED = false;
bool WARNING_DRY = true;
bool WATCH_DRY = true;

bool ALARM_ENABLED = false;

LiquidCrystal_I2C lcd(0x27,16,2); //Setup LCD over Wire comms, uses default address 0x27

//Used for the interrupt, attachInterrupt requires a func
void silence_alarm(){
  SILENCED = true;
}

void setup() {
  pinMode(SPKR_PIN, OUTPUT);
  pinMode(SILENCE_BTN_PIN, INPUT_PULLUP);
  pinMode(WATCH_SENSOR, INPUT_PULLUP);
  pinMode(WARNING_SENSOR, INPUT_PULLUP);
  lcd.init();
  lcd.backlight();
  //silence alarm if button is pressed because it's in pullup, we bind to rising
  attachInterrupt(digitalPinToInterrupt(SILENCE_BTN_PIN), silence_alarm, RISING);
  SILENCED = false;
  Serial.begin(9600);
}

void check_alarm(){
  //If water is high or breached, alarm
  ALARM = !WATCH_DRY || !WARNING_DRY;
  //reset the silence to false once we're back in the dry condition so it can alarm again.
  if(!ALARM){
    SILENCED = false; 
  }
}

//Handles lcd output for the loop
void update_output(){
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("S1 Status: ");
  if(WATCH_DRY){
    lcd.print("DRY");
  } else {
    //WET, INSIDE PIT
    lcd.print("WET!!");
  }
  lcd.setCursor(0,1);
  lcd.print("S2 Status: ");
  if(WARNING_DRY){
    lcd.print("DRY");
  } else {
    //WET, OUTSIDE PIT. BREACH.
    lcd.print("WET!!");
  }
}

//this function handles communicating with the hub
void updateHub(){
  int hub_request = 0;
  int status_report = 0; 
  if(Serial.available()){
    hub_request = Serial.parseInt(); //only passing basic ints around, this should work
    //0 if all is well, 1 for watch wet, 2 for warning wet, 3 for both.
    if(!WATCH_DRY) status_report += 1;
    if(!WARNING_DRY) status_report +=2;

    //Use 128 for all is well instead of 0 so that blips don't update the hub.
    if(status_report == 0){
      status_report = 128;
    }

    Serial.println(status_report);    
  } 
  if(hub_request == 1){
    SILENCED = true;
  }
}

//timing vars
int prev_stamp = 0;//Used for timing for playing alarm and resets
int alarm_toggle = 1;

void loop() {
  int polling_freq = 250; //We'll use a quarter second for loop length;
  WATCH_DRY = (digitalRead(WATCH_SENSOR)== HIGH);//These are in pullup, low is bad.
  WARNING_DRY = (digitalRead(WARNING_SENSOR)== HIGH);
  int curr_stamp = millis();
  //play alarm sound every other quarter second if alarm is played.
  if(ALARM & ALARM_ENABLED & !SILENCED){
      if(alarm_toggle){
        tone(SPKR_PIN, 500, polling_freq*0.85);
        alarm_toggle = 0;
      } else {
        alarm_toggle = 1;
      }
    }
  while(curr_stamp - prev_stamp < polling_freq){
    //Play alarm if necessary, handle silencing alarm if necessary, really waiting out the polling_freq
    curr_stamp = millis();    
  }
  //end the note, update outputs and status globals
  update_output();
  updateHub();
  check_alarm();
  //noTone(SPKR_PIN);
  prev_stamp = curr_stamp;
}
