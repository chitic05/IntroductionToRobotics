#include <Arduino.h>

constexpr int boardLed = 2;
constexpr unsigned int waitTime = 2000; // astepta 2000 de milisecunde dupa seria de blink-uri
constexpr unsigned int blinkInterval = 250;
unsigned long previousMillis = 0;
bool boardLedState = LOW;
bool isWaiting = false;
int blinkCount=0;
constexpr int maxBlinks = 3;

constexpr int potPin = 34;
int potValue;

constexpr int externLed = 18;
constexpr int freq = 5000;
constexpr int res = 10;

constexpr int RPin = 27;
constexpr int GPin = 26;
constexpr int BPin = 25;
constexpr int firstThirdEnd = 1023/3;
constexpr int secondThirdEnd = (1023/3)*2;

constexpr unsigned int printInterval = 500;
unsigned long lastPrint = 0;

constexpr int buzzerPin = 23;
int soundPlaying = 0;
const int melodyNotes[] = {262, 330, 392, 523};
constexpr int totalNotes = sizeof(melodyNotes) / sizeof(melodyNotes[0]);
constexpr unsigned long noteDuration = 150;     // fiecare notă durează 150 ms
constexpr unsigned int noteGap = 20;
unsigned long buzzerStart=0;
int currentNote = 0;

void setup() {
  Serial.begin(9600);

  pinMode(boardLed, OUTPUT);
  digitalWrite(boardLed, boardLedState);

  pinMode(potPin, INPUT);

  ledcSetup(0, freq, res);
  ledcAttachPin(externLed, 0);
  ledcWrite(0, 0);

  pinMode(RPin, OUTPUT);
  pinMode(GPin, OUTPUT);
  pinMode(BPin, OUTPUT);
  digitalWrite(RPin, LOW);
  digitalWrite(GPin, LOW);
  digitalWrite(BPin, LOW);

  // pinMode(buzzerPin, OUTPUT);
  // digitalWrite(buzzerPin, LOW);
  ledcSetup(1, 2000, 8);
  ledcAttachPin(buzzerPin, 1);
}

void loop(){
  // //BlinkLed
  unsigned long currentMillis = millis();
  if(isWaiting){
    if(currentMillis - previousMillis >= waitTime){
      previousMillis = currentMillis;
      isWaiting = false;
    }
  }else{
    if(currentMillis - previousMillis >= blinkInterval){
      previousMillis = currentMillis;
      boardLedState = !boardLedState;
      digitalWrite(boardLed, boardLedState);
      blinkCount++;

      if(blinkCount >= 2*maxBlinks){
        
        isWaiting = true;
        blinkCount = 0;
        boardLedState = false;
        digitalWrite(boardLed, boardLedState);
      }
    }
  }

  //Citire potentiometru si aprindere led extern
  potValue = analogRead(potPin);
  int externLedVal = map(potValue, 0, 4095, 0, 1023);
  ledcWrite(0, externLedVal);


  //Aprindere led rgb
  const char* operatingLevel;

  if (externLedVal <= firstThirdEnd) {
    soundPlaying = 0;
    ledcWriteTone(1, 0);

    digitalWrite(BPin, HIGH);
    digitalWrite(GPin, LOW);
    digitalWrite(RPin, LOW);
    operatingLevel = "LOW (Blue)";
  } else if (externLedVal >= secondThirdEnd) {
    if (soundPlaying == 0) {
      soundPlaying = 1;
      currentNote = 0;
      buzzerStart = currentMillis;
      ledcWriteTone(1, melodyNotes[currentNote]);
    }

    if (soundPlaying == 1) {
      unsigned long elapsed = currentMillis - buzzerStart;

      //Pauza intre note
      if (elapsed >= noteDuration - noteGap && elapsed < noteDuration) {
        ledcWriteTone(1, 0);
      } 
      else if (elapsed >= noteDuration) {
        buzzerStart = currentMillis;
        currentNote++;

        if (currentNote < totalNotes) {
          ledcWriteTone(1, melodyNotes[currentNote]);
        } else {
          soundPlaying = -1;   // Melodie finalizata
          ledcWriteTone(1, 0); // Oprire buzzer
        }
      }
    }

    digitalWrite(BPin, LOW);
    digitalWrite(GPin, LOW);
    digitalWrite(RPin, HIGH);
    operatingLevel = "HIGH (Red)";
  } else {
    soundPlaying = 0;
    ledcWriteTone(1, 0);
    digitalWrite(BPin, LOW);
    digitalWrite(GPin, HIGH);
    digitalWrite(RPin, LOW);
    operatingLevel = "MEDIUM (Green)";
  }

  if (currentMillis - lastPrint >= printInterval) {
    lastPrint = currentMillis;
    Serial.printf("Raw ADC: %d | PWM Output: %d | Operating Level: %s\n", 
                  potValue, externLedVal, operatingLevel);
  }
}