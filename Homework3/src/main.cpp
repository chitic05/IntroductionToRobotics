#include <Arduino.h>

#define BUTTON 23
#define BUILT_IN_LED 2
#define LED 22

enum Modes{
  OFF = 0,
  DIM = 1,
  FULL = 2,
  BLINK = 3
};
Modes lampMode = OFF;

uint8_t history = 0xFF; //ajuta la debouncing, 0xFF = notpressed, 0x00 = pressed
bool pressed = false;
long holdStart = 1;
long holdEnd = 0;
constexpr int holdTargetDuration = 1000;
volatile bool checkButton= false;
bool validating = false;

constexpr int buttonReadDelay = 5;
long lastButtonRead = 0;
long currentTime;

bool isWaiting = false;
constexpr int blinkDuration = 250;
constexpr int blinkTimeout = 1000;
constexpr int blinkNumber = 2;
int blinkCount = 0;
long lastBlink = 0;
bool blinkState = false;

constexpr int ledBlinkDuration = 500;
long lastLedBlink;
bool ledState = true;



void IRAM_ATTR buttonInt(void){
  checkButton = true;
}

void setup(){
  Serial.begin(9600);
  pinMode(BUTTON, INPUT_PULLUP);
  pinMode(BUILT_IN_LED, OUTPUT);
  
  ledcSetup(0, 5000, 10);
  ledcAttachPin(LED, 0);
  ledcWrite(0, 0);
  attachInterrupt(digitalPinToInterrupt(BUTTON), buttonInt, FALLING);
}

void loop(){
  currentTime = millis();

  if(isWaiting){
    if(currentTime - lastBlink >= blinkTimeout){
      isWaiting = false;
      lastBlink = currentTime;
    }
  }else{
     if(currentTime - lastBlink >= blinkDuration){
      lastBlink = currentTime;
      blinkState = !blinkState;
      digitalWrite(BUILT_IN_LED, blinkState);
      blinkCount++;

      if(blinkCount >= 2*blinkNumber){
        isWaiting = true;
        blinkCount = 0;
        blinkState = false;
        digitalWrite(BUILT_IN_LED, blinkState);
      }
    }
  }

  if(checkButton && !validating){
    validating = true;
    checkButton = false;
    history = 0xFF;
  }

  if(validating){
    if(currentTime - lastButtonRead >= buttonReadDelay){

      lastButtonRead = currentTime;
      history = (history << 1) | digitalRead(BUTTON);
      if(history == 0x0 && !pressed){
        pressed = true;
        holdStart = currentTime;
      }

      if(history == 0xFF && pressed){
        pressed = false;
        validating = false;
        unsigned long holdDuration = currentTime - holdStart;
        if(holdDuration >= holdTargetDuration){
          lampMode = OFF;
        } else {
          if(lampMode == BLINK) lampMode = OFF;
          else lampMode = (Modes)((int)lampMode + 1);
        }
      }
      
    }
  }
  
  
  switch(lampMode){
    case OFF:
      ledcWrite(0, 0);
      break;
    case DIM:
      ledcWrite(0, 250);
      break;
    case FULL:
      ledcWrite(0, 4095);
      break;
    case BLINK:
      if(currentTime - lastLedBlink >= ledBlinkDuration){
        lastLedBlink = currentTime;
        ledState = !ledState;
        if(ledState){
          ledcWrite(0, 4095);
        }else ledcWrite(0, 0);
      }
  }
  
}