#include <pt.h>

#define PT_(pt, ms, ts) \
  ts = millis(); \
  PT_WAIT_WHILE(pt, millis()-ts < (ms));


#define MOTION_SEN 7 // PIN, DIGITAL,
#define LED_PIN_R 6 // PIN, ANALOG 0-255
#define LED_PIN_G 5 // PIN, ANALOG 0-255
#define LED_PIN_B 3 // PIN, ANALOG 0-255
#define LED_PIN_ROOM  9 // PIN, DIGITAL
#define RELAY_PIN 8 // PIN, DIGITAL
#define IN  0 // person is in the room
#define OUT 1 // oerson is not in the room
#define DETECTED  1 // motion detector detect a motion
#define NOT_DETECTED  0 // motion detector not detect a motion
#define ON 1 // room led on
#define OFF 0 // room led off
#define LED_MODE_OFF 0 // no light
#define LED_MODE_ALERT 1 // alternate between red and blue
#define LED_MODE_NORMAL 2 // constant white light
#define LED_MODE_FIRE 3 //

int state = ON; // state of which the house is in
int motionDetected = NOT_DETECTED; // state for motion detector
int ledR = 255; //value of the led red 0-255
int ledG; //value of the led green 0-255
int ledB; //value of the led blue 0-255
int ledRoom = OFF;
int currentLEDMode = LED_MODE_OFF;
int oldLEDMode = LED_MODE_OFF;
String req = ""; // request from node

struct pt pt_taskMotion; // thread for motion dector ,alway run
struct pt pt_taskLED; // thread for led ,alway run
struct pt pt_taskLEDControler; // thread for controling the led
struct pt pt_taskMainController; // control all the sensor
struct pt pt_taskLEDRoom; // LED in the room
struct pt pt_taskRequest; // handle request



// control led in room
PT_THREAD(taskRequest(struct pt* pt))
{
  static uint32_t ts;
  PT_BEGIN(pt);
  while (1)
  {
    if(req == "true"){

      state = OUT;
      Serial.println("on "+String(state)+" "+String(OUT));
    }
    if(req == "false"){

      state = IN;
       Serial.println("off "+String(state)+" "+String(IN));
    }
//    else{

//    Serial.print(req+ " " + String(state));
    serialEvent();
    PT_(pt,200, ts);
  }
  PT_END(pt);
}
// control led in room
PT_THREAD(taskLEDRoom(struct pt* pt))
{
  static uint32_t ts;
  PT_BEGIN(pt);
  while (1)
  {
//    Serial.print(ledRoom);
  if(ledRoom == ON){
    digitalWrite(LED_PIN_ROOM, HIGH);
  }
  else{
    digitalWrite(LED_PIN_ROOM, LOW);
  }
    PT_(pt, 100, ts);
  }
  PT_END(pt);
}

// Main controller
PT_THREAD(taskMainController(struct pt* pt))
{
  static uint32_t ts;
  PT_BEGIN(pt);
  while (1)
  {
//  Serial.println(state);
 if(motionDetected == DETECTED && (state == IN)){
//    Serial.println("1");
    changeLedMode(LED_MODE_NORMAL);
    sendResponse("1");
  }
  else if (motionDetected == NOT_DETECTED && state ==IN){
//    Serial.println("2");
    changeLedMode(LED_MODE_OFF);
    sendResponse("0");
  }
      else if(motionDetected  == DETECTED&& state ==OUT){
//    Serial.println("3");
    changeLedMode(LED_MODE_ALERT);
    sendResponse("1");
  }
  else if (motionDetected == NOT_DETECTED && state == OUT){
//    Serial.println("4");
    changeLedMode(LED_MODE_OFF);
    sendResponse("0");
  }
    PT_(pt, 500, ts);
  }
  PT_END(pt);
}

// LED controller
PT_THREAD(taskLEDControler(struct pt* pt))
{
  static uint32_t ts;
  PT_BEGIN(pt);
  while (1)
  {
//    Serial.println(currentLEDMode);
    if(currentLEDMode == LED_MODE_OFF && oldLEDMode == LED_MODE_NORMAL && state == IN){
      ledRoom = ON;
    }
    if(currentLEDMode == LED_MODE_OFF){
      ledControl(0,0,0);
    }
    else if(currentLEDMode == LED_MODE_ALERT ){
      ledControl(255,0,0);
      PT_(pt, 1000, ts);
      ledControl(0,0,255);
      PT_(pt, 1000, ts);
    }
    else if(currentLEDMode == LED_MODE_NORMAL){
      ledControl(255,255,255);
      PT_(pt, 1000, ts);
    }
    PT_(pt, 20, ts);
  }
  PT_END(pt);
}

// LED task
PT_THREAD(taskLED(struct pt* pt))
{
  static uint32_t ts;
  PT_BEGIN(pt);
  while (1)
  {
  analogWrite(LED_PIN_R,ledR);
  analogWrite(LED_PIN_G,ledG);
  analogWrite(LED_PIN_B,ledB);
  PT_(pt, 10, ts);
  }

  PT_END(pt);
}

//motion task
PT_THREAD(taskMotion(struct pt* pt))
{
  static uint32_t ts;
  PT_BEGIN(pt);
  int a;
  while (1)
  {
    a = digitalRead(MOTION_SEN);
    if(a==HIGH){// value that the sensor detect
      motionDetected = DETECTED;
    }
    else{
      motionDetected = NOT_DETECTED;
    }
    PT_(pt, 50, ts);
//   Serial.println(a);
  }


  PT_END(pt);
}

// set the color of the led
void ledControl(int r, int g, int b){
  ledR = r%256;
  ledG = g%256;
  ledB = b%256;
}

// chane the mode of the led
void changeLedMode(int mode){
  if(currentLEDMode != mode){
    oldLEDMode = currentLEDMode;
    currentLEDMode = mode;
  }

}

// out put to the relay(control 220 v )
void relay(int input){
  if(input == ON){
    digitalWrite(RELAY_PIN,HIGH);
  }
  if(input == OFF){
    digitalWrite(RELAY_PIN,LOW);
  }
}

void setup() {
  Serial.begin(9600);
  Serial1.begin(115200);
  PT_INIT(&pt_taskMotion);
  PT_INIT(&pt_taskLED);
  PT_INIT(&pt_taskLEDControler);
  PT_INIT(&pt_taskMainController);
  PT_INIT(&pt_taskRequest);
  pinMode(MOTION_SEN,INPUT);
  pinMode(LED_PIN_R,OUTPUT);
  pinMode(LED_PIN_G,OUTPUT);
  pinMode(LED_PIN_B,OUTPUT);
  pinMode(RELAY_PIN,OUTPUT);

}
// get input from sensor and output
void control(){

  if(motionDetected == DETECTED && (state == IN)){
//    Serial.println("1");
    changeLedMode(LED_MODE_NORMAL);
  }
  else if (motionDetected == NOT_DETECTED && state ==IN){
//    Serial.println("2");
    changeLedMode(LED_MODE_OFF);
  }
      else if(motionDetected  == DETECTED&& state ==OUT){
//    Serial.println("3");
    changeLedMode(LED_MODE_ALERT);
  }
  else if (motionDetected == NOT_DETECTED && state == OUT){
//    Serial.println("4");
    changeLedMode(LED_MODE_OFF);
  }
//  delay(50);
}

void sendResponse(String msg){
  Serial1.print(msg);
  Serial1.print("\r");
}
void serialEvent(){
  if(Serial1.available()){
    req = Serial1.readStringUntil('\r');
  }
}

// handle request from node
void handleRequest(){

}
void loop() {
  taskMotion(&pt_taskMotion);
  taskLED(&pt_taskLED);
  taskLEDControler(&pt_taskLEDControler);
  taskMainController(&pt_taskMainController);
  taskRequest(&pt_taskRequest);
  taskLEDRoom(&pt_taskLEDRoom);

//  control();

}
