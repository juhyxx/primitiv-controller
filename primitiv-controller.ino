#include <MIDIUSB.h>

//#define DEBUG

#define MIDI_CHANNEL 0
#define MIDI_BASE_ID 44
#define MIDI_ANALOG_CONTROL 1
#define BAUD_RATE 115200
#define DELAY 500

#define MEASURE_REPEAT_COUNT 50

#define IN_ANALOG_1 A0
#define OUT_ANALOG_1 6
#define SWITCH_ANALOG_1 2

#define IN_DIGITAL_1 15
#define IN_DIGITAL_2 14
#define IN_DIGITAL_3 16
#define IN_DIGITAL_4 10

#define OUT_DIGITAL_1 9
#define OUT_DIGITAL_2 8
#define OUT_DIGITAL_3 7
#define OUT_DIGITAL_4 4

#define PEDAL_STATE_OFF 0
#define PEDAL_STATE_WAITING 1
#define PEDAL_STATE_PREPARED 2
#define PEDAL_STATE_ON 3

void setup()
{
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

  initTest();
}
byte digitalInputs[4] = {IN_DIGITAL_1, IN_DIGITAL_2, IN_DIGITAL_3, IN_DIGITAL_4};
byte digitalOutputs[4] = {OUT_DIGITAL_1, OUT_DIGITAL_2, OUT_DIGITAL_3, OUT_DIGITAL_4};
int prevMidiValue = 0;
byte prevDigitalInput = 0, digitalValue = 0;
int pedalValue = 0;
unsigned long pedalUpWaitingTime = 0;
unsigned long pedalWaitingTime = 0;
int pedalState = PEDAL_STATE_OFF;

void loop()
{
  byte digitalInput = 0;
  midiEventPacket_t rx;

  /**** MIDI input ****/
  do
  {
    rx = MidiUSB.read();
    if (rx.header != 0)
    {
      byte type = rx.byte1 & 0xF0;
      byte channel = rx.byte1 & 0x0F;
      byte data = rx.byte2;
      byte button = data - MIDI_BASE_ID;
      byte value = rx.byte3;

      if (type == 0xB0 && channel == MIDI_CHANNEL)
      {
        if (button >= 0 && button < 4)
        {
          bitWrite(digitalValue, button, value > 64);
          digitalWrite(digitalOutputs[button], value > 64);
        }
      }
    }
  } while (rx.header != 0);

  /**** DIGITAL ****/
  for (byte i = 0; i < 4; i++)
  {
    bitWrite(digitalInput, i, !digitalRead(digitalInputs[i]));
  }
  if (digitalInput > prevDigitalInput)
  {
    byte oldDigitalValue = digitalValue;
    digitalValue = digitalInput ^ prevDigitalInput ^ digitalValue;
    //Serial.println(digitalInput, BIN);
    for (byte i = 0; i < 4; i++)
    {
      byte bitValue = bitRead(digitalValue, i);
      if (bitRead(oldDigitalValue, i) != bitValue)
      {
        digitalWrite(digitalOutputs[i], bitValue);
        midiEventPacket_t event = {0x0B, 0xB0 | MIDI_CHANNEL, MIDI_BASE_ID - i, bitValue ? 127 : 0};
        MidiUSB.sendMIDI(event);
        MidiUSB.flush();
        //Serial.println(MIDI_BASE_ID + i);
      }
    }
  }
  prevDigitalInput = digitalInput;

  /**** ANALOG ****/
  if (digitalRead(SWITCH_ANALOG_1))
  {
    long int avgValue1 = 0;
    for (int i = 0; i < MEASURE_REPEAT_COUNT; i++)
    {
      avgValue1 += analogRead(IN_ANALOG_1);
      delayMicroseconds(DELAY / MEASURE_REPEAT_COUNT);
    }
    avgValue1 = (avgValue1 / MEASURE_REPEAT_COUNT);
    int midiValue1 = avgValue1 >> 3;

    analogWrite(OUT_ANALOG_1, avgValue1 >> 2);

    long int waitingTime = micros() - pedalWaitingTime;
    long int upWaitingTime = micros() - pedalUpWaitingTime;

    if (midiValue1 < prevMidiValue) // backward move resets
    {
      pedalState = PEDAL_STATE_OFF;
    }
    if (midiValue1 < 3)
    {
      pedalState = PEDAL_STATE_WAITING;
      pedalWaitingTime = micros();
    }

    switch (pedalState)
    {
    case PEDAL_STATE_WAITING:
      if (waitingTime > 800000)
      {
        pedalState = PEDAL_STATE_OFF;
      }
      if (midiValue1 > 125)
      {
        pedalState = PEDAL_STATE_PREPARED;
        pedalUpWaitingTime = micros();
      }
      break;

    case PEDAL_STATE_PREPARED:
      if (upWaitingTime > 300000)
      {
        pedalState = PEDAL_STATE_ON;
      }
      break;

    case PEDAL_STATE_ON:
      pedalValue = pedalValue == 127 ? 0 : 127;
      midiEventPacket_t event = {0x0B, 0xB0 | MIDI_CHANNEL, MIDI_BASE_ID - 4, pedalValue};
      MidiUSB.sendMIDI(event);
      MidiUSB.flush();
      pedalState = PEDAL_STATE_OFF;
      break;
    }
    if (abs(midiValue1 - prevMidiValue) > 1)
    {
      midiEventPacket_t event = {0x0B, 0xB0 | MIDI_CHANNEL, MIDI_ANALOG_CONTROL, midiValue1};
      MidiUSB.sendMIDI(event);
      MidiUSB.flush();
    }
#ifdef DEBUG
    Serial.println(String("PEDALstate:") + pedalState);
#endif

    prevMidiValue = midiValue1;
  }
  else
  {
    analogWrite(OUT_ANALOG_1, 0);
    delayMicroseconds(DELAY);
  }
}

void initTest()
{
  int interval = 700;

  //Serial.print("Init test...");
  digitalWrite(OUT_DIGITAL_1, HIGH);
  digitalWrite(OUT_DIGITAL_2, HIGH);
  digitalWrite(OUT_DIGITAL_3, HIGH);
  digitalWrite(OUT_DIGITAL_4, HIGH);
  analogWrite(OUT_ANALOG_1, 255);
  delay(interval);
  analogWrite(OUT_ANALOG_1, 100);
  delay(interval);
  analogWrite(OUT_ANALOG_1, 50);
  delay(interval);
  analogWrite(OUT_ANALOG_1, 5);
  delay(interval);
  analogWrite(OUT_ANALOG_1, 0);
  delay(interval);
  digitalWrite(OUT_DIGITAL_4, LOW);
  delay(interval);
  digitalWrite(OUT_DIGITAL_3, LOW);
  delay(interval);
  digitalWrite(OUT_DIGITAL_2, LOW);
  delay(interval);
  digitalWrite(OUT_DIGITAL_1, LOW);
  delay(interval);
  //Serial.println("Done.");
}
