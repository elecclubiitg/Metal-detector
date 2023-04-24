#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

#define CE A2
#define CSN A4

// left motor
#define ENA 10
#define IN2 7
#define IN1 6

#define IN3 4
#define IN4 3
#define ENB 9

const byte address[6] = "00002";
const uint64_t pipe = 0xE6E6E6E6E6E6;

float speed_fac = 1;
int data[3];
int x_axis = 0;
int y_axis = 0;
int kick = 0;
int kick_last = 0;
RF24 radio(CE, CSN);

#define CYCLES_PER_SIGNAL 5000

#define BASE_TONE_FREQUENCY 280


#define SENSITIVITY_POT_APIN 1
#define SPEAKER_PIN 2
#define TRIGGER_BTN_PIN A0
#define RESET_BTN_PIN A3
unsigned long lastSignalTime = 0;
unsigned long signalTimeDelta = 0;

boolean firstSignal = true;
unsigned long storedTimeDelta = 0;

SIGNAL(TIMER1_COMPA_vect)
{
  unsigned long currentTime = micros();
  signalTimeDelta =  currentTime - lastSignalTime;
  lastSignalTime = currentTime;

  if (firstSignal)
  {
    firstSignal = false;
  }
  else if (storedTimeDelta == 0)
  {
    storedTimeDelta = signalTimeDelta;
  }

  // Reset OCR1A
  OCR1A += CYCLES_PER_SIGNAL;
}

void setup()
{

  pinMode(ENA, OUTPUT);
  pinMode(ENB, OUTPUT);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);

  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);

  Serial.begin(9600);
  radio.begin();
  radio.openReadingPipe(0, address);
  radio.setPALevel(RF24_PA_MIN);
  radio.startListening();
  
  
  TCCR1A = 0b00000000;
  
  TCCR1B = 0b00000111;

  TIMSK1 |= (1 << OCIE1A);

  OCR1A = 1;

  pinMode(SPEAKER_PIN, OUTPUT);
  pinMode(TRIGGER_BTN_PIN, INPUT_PULLUP);
  pinMode(RESET_BTN_PIN, INPUT_PULLUP);
}

void setMotorSpeeds(int speed_a, int speed_b) { 
  if(speed_a >= 0) { 
    analogWrite(ENA, speed_a);
    digitalWrite(IN1, HIGH);
    digitalWrite(IN2, LOW);
  }
  else { 
    analogWrite(ENA, -speed_a);
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, HIGH);
  }

  if(speed_b >= 0) { 
    analogWrite(ENB, speed_b);
    digitalWrite(IN3, HIGH);
    digitalWrite(IN4, LOW);
  }
  else { 
    analogWrite(ENB, -speed_b);
    digitalWrite(IN3, LOW);
    digitalWrite(IN4, HIGH);
  }
}

void loop()
{

  if (radio.available()) {
    radio.read(&data, sizeof(data));

    x_axis = data[0];
    y_axis = data[1];
    kick = data[2];
    // Serial.print(x_axis);
    // Serial.print(" | ");
    // Serial.print(y_axis);
    // Serial.print(" | ");
    // Serial.print(kick);
    // Serial.println(" | ");
  }

  int speed_a_y = y_axis;
  int speed_b_y = y_axis;
  int speed_a_x = -x_axis;
  int speed_b_x = x_axis;
  int speed_a = constrain(speed_a_x + speed_a_y, -255, 255);
  int speed_b = constrain(speed_b_x + speed_b_y, -255, 255);

  setMotorSpeeds(speed_a/speed_fac, speed_b/speed_fac);



  if (digitalRead(TRIGGER_BTN_PIN) == LOW)
  {
    float sensitivity = mapFloat(analogRead(SENSITIVITY_POT_APIN), 0, 1023, 0.5, 20.0);
    //Serial.println(storedTimeDelta);
    Serial.println(signalTimeDelta);
    int storedTimeDeltaDifference = (storedTimeDelta - signalTimeDelta) * sensitivity;
    tone(SPEAKER_PIN, BASE_TONE_FREQUENCY + storedTimeDeltaDifference);
    Serial.println(storedTimeDeltaDifference);
   
  }
  else
  {
    noTone(SPEAKER_PIN);
  }

  if (digitalRead(RESET_BTN_PIN) == LOW)
  {
    storedTimeDelta = 0;
  }
   
}

float mapFloat(int input, int inMin, int inMax, float outMin, float outMax)
{
  float scale = (float)(input - inMin) / (inMax - inMin);
  return ((outMax - outMin) * scale) + outMin;
}
