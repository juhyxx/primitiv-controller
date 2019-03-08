#include <MIDI.h>

//#define DEBUG

#define MIDI_BAUD_RATE 31250
#define MIDI_CHANNEL 1
#define MIDI_BASE_ID 41

#ifdef DEBUG
  #define BAUD_RATE 115200
#else
  #define BAUD_RATE MIDI_BAUD_RATE
#endif

#define DELAY 0.5

#define MEASURE_REPEAT_COUNT 50

#define IN_ANALOG_1 A0

#define OUT_ANALOG_1 6 
#define SWITCH_ANALOG_1 2

#define IN_DIGITAL_1 13
#define IN_DIGITAL_2 12
#define IN_DIGITAL_3 11 
#define IN_DIGITAL_4 10

#define OUT_DIGITAL_1 9 
#define OUT_DIGITAL_2 8
#define OUT_DIGITAL_3 7
#define OUT_DIGITAL_4 4

struct MySettings : public midi::DefaultSettings {
    static const bool UseRunningStatus = true;
};

MIDI_CREATE_CUSTOM_INSTANCE(HardwareSerial, Serial, MIDI, MySettings);


void setup() {   
  Serial.begin(BAUD_RATE);

  pinMode(IN_DIGITAL_1, INPUT_PULLUP);
  pinMode(IN_DIGITAL_2, INPUT_PULLUP);
  pinMode(IN_DIGITAL_3, INPUT_PULLUP);
  pinMode(IN_DIGITAL_4, INPUT_PULLUP);
  pinMode(SWITCH_ANALOG_1, INPUT_PULLUP);
  
  pinMode(OUT_DIGITAL_1, OUTPUT);
  pinMode(OUT_DIGITAL_2, OUTPUT);
  pinMode(OUT_DIGITAL_3, OUTPUT);
  pinMode(OUT_DIGITAL_4, OUTPUT);

  pinMode(IN_ANALOG_1, INPUT);
  pinMode(OUT_ANALOG_1, OUTPUT);

  #ifdef DEBUG
    Serial.write("MIDI channel: ");
    Serial.println(MIDI_CHANNEL);
  #else
    MIDI.setHandleControlChange(onControlChange);
    MIDI.begin(MIDI_CHANNEL); 
  #endif 
  initTest();
}
byte digitalInputs[4] = {IN_DIGITAL_1, IN_DIGITAL_2, IN_DIGITAL_3, IN_DIGITAL_4};
byte digitalOutputs[4] = {OUT_DIGITAL_1, OUT_DIGITAL_2, OUT_DIGITAL_3, OUT_DIGITAL_4};
int prevValue1 = 0;
byte prevDigitalInput = 0, digitalValue = 0;

void onControlChange(byte channel, byte number, byte value) {
  if (channel == MIDI_CHANNEL) {
    byte i = number - MIDI_BASE_ID;
    byte bitValue = value > 64 ? 1 : 0;

    digitalWrite(digitalOutputs[i], bitValue);
  }
}


void loop(){
  #ifndef DEBUG
   MIDI.read();
  #endif

  /**** DIGITAL ****/
  byte digitalInput = 0;

  for (byte i = 0; i < 4; i++) {
    bitWrite(digitalInput, i, !digitalRead(digitalInputs[i]));
  }
  if (digitalInput > prevDigitalInput) {
    byte oldDigitalValue = digitalValue;
    digitalValue = digitalInput ^ prevDigitalInput ^ digitalValue;

    if ((digitalInput ^ 0b0011) == 0) {
      autoMode();
    }

    Serial.println(digitalInput, BIN);

    for (byte i = 0; i < 4; i++) {
      byte bitValue = bitRead(digitalValue, i);
      if (bitRead(oldDigitalValue, i) != bitValue) {
        digitalWrite(digitalOutputs[i], bitValue);  
        #ifndef DEBUG
          MIDI.sendControlChange(MIDI_BASE_ID + i, bitValue ? 127 : 0, MIDI_CHANNEL);
        #else
          Serial.println(MIDI_BASE_ID + i);
        #endif
      }
    }
  }
  prevDigitalInput = digitalInput;

  if (digitalRead(SWITCH_ANALOG_1)) {
    /**** ANALOG ****/
    long int avgValue1 = 0;
    for (int i = 0; i < MEASURE_REPEAT_COUNT; i++) {
      avgValue1 += analogRead(IN_ANALOG_1);
      delayMicroseconds(DELAY * 1000/MEASURE_REPEAT_COUNT);
    }
    
    avgValue1 = (avgValue1 /MEASURE_REPEAT_COUNT);

    int midiValue1 = avgValue1 >> 3;
    analogWrite(OUT_ANALOG_1, avgValue1>>2);
    if (abs(midiValue1 - prevValue1) > 1) {
      
      #ifdef DEBUG
        Serial.print(avgValue1, DEC);
        Serial.print(",");
        Serial.println(midiValue1, DEC);
      #else
        MIDI.sendControlChange(1,midiValue1, MIDI_CHANNEL);
      #endif 
      prevValue1 = midiValue1;
    }
  }
  else {
    analogWrite(OUT_ANALOG_1, 0);
     delay(DELAY);
  }
}

void autoMode() {
  Serial.println("autoMode");
  for (int i = 0; i <= 255; i++) {
    analogWrite(OUT_ANALOG_1, i);
    delay(2);
  }
  for (int i = 255; i > 0; i--) {
    analogWrite(OUT_ANALOG_1, i);
    delay(2);
  }
}

void initTest() {
  #ifdef DEBUG
    Serial.println("Init test...");
  #endif 
   for (int i = 0; i <= 255; i+=5) {
    analogWrite(OUT_ANALOG_1, i);
    delay(25);
  }
  digitalWrite(OUT_DIGITAL_1, HIGH);
  digitalWrite(OUT_DIGITAL_2, HIGH);
  digitalWrite(OUT_DIGITAL_3, HIGH);
  digitalWrite(OUT_DIGITAL_4, HIGH);
  delay(300);
  digitalWrite(OUT_DIGITAL_4, LOW);
  delay(100);
  digitalWrite(OUT_DIGITAL_3, LOW);
  delay(100);
  digitalWrite(OUT_DIGITAL_2, LOW);
  delay(100);
  digitalWrite(OUT_DIGITAL_1, LOW);
  delay(100);
  analogWrite(OUT_ANALOG_1, 0);
  delay(200);
  
  #ifdef DEBUG
    Serial.println("init test done.");
  #endif 
}
