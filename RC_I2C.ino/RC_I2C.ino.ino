//RCtoI2C arduino pro mni code by Gerald Condon
//Based off of RCreciever by rambo, however now with added smoothing, Serial and I2C protocol
//https://github.com/rambo/Arduino_rcreceiver/blob/master/Arduino_rcreceiver.ino
#include "PinChangeInt_userData.h"

typedef struct {
    const uint8_t rc_channel;
    const uint8_t input_pin;
    const uint16_t pulse_len_low;
    const uint16_t pulse_len_high; 
    const uint8_t smooth_size;
    uint16_t avg_duty_cycle;
    volatile unsigned long start_micros;
    volatile unsigned long stop_micros;
    volatile boolean new_data;
} RCInput;

//Inital Parameters
//Low fidelity:
int Apin=7;
int Bpin=5;
int Cpin=4;

RCInput inputs[] = {
    {
      1,  //channel
      9, //Pin
      1000,//pulselengthlow
      2010,//pulselengthhigh
      10, //smoothing size
      512,//starting average;
      0,0,false //placeholders
    },
    {
      2,  //channel
      8, //Pin
      996,//pulselengthlow
      2000,//pulselengthhigh
      10, //smoothing size
      512,//starting average;
      0,0,false //placeholders
    },
    { 
      3,  //channel
      6, //Pin
      850,//pulselengthlow
      2000,//pulselengthhigh
      10, //smoothing size
      512,//starting average;
      0,0,false //placeholders
    }
};

//Multiplexed 
unsigned long A_start_micros=0;
unsigned long A_stop_micros=0;
uint16_t A_duty_cycle=512;
uint16_t A_pulse_len_low=13900;
uint16_t A_pulse_len_high=15900;

unsigned long B_start_micros=0;
unsigned long B_stop_micros=0;
uint16_t B_duty_cycle=512;
uint16_t B_pulse_len_low=14000;
uint16_t B_pulse_len_high=15700;

unsigned long C_start_micros=0;
unsigned long C_stop_micros=0;
uint16_t C_duty_cycle=0;
uint16_t C_pulse_len_low=14000;
uint16_t C_pulse_len_high=15700;

const uint8_t inputs_len = sizeof(inputs) / sizeof(RCInput);
volatile unsigned long last_report_time = millis();
uint8_t ABCticker=0;

//called via interupts on hi
void rc_pulse_high(void* inptr) {
    RCInput* input = (RCInput*)inptr;
    input->new_data = false;
    input->start_micros = micros();
}

//called via interupts on low
void rc_pulse_low(void* inptr) {
    RCInput* input = (RCInput*)inptr;
    input->stop_micros = micros();
    input->new_data = true;
}

void calculate(void* inptr) {
    RCInput* input = (RCInput*)inptr;
    input->avg_duty_cycle = map((uint16_t)(input->stop_micros - input->start_micros),input->pulse_len_low,input->pulse_len_high,0,1023);
    input->new_data = false;
}

void Acalculate() {
    if(A_pulse_len_low&&A_pulse_len_high){
      A_duty_cycle = map((uint16_t)(A_stop_micros - A_start_micros),A_pulse_len_low,A_pulse_len_high,0,1023);
    }
}

void Bcalculate() {
    if(B_pulse_len_low&&B_pulse_len_high){
      B_duty_cycle = map((uint16_t)(B_stop_micros - B_start_micros),B_pulse_len_low,B_pulse_len_high,0,1023);
    }
}

void Ccalculate() {
    if(C_pulse_len_low&&C_pulse_len_high){
      C_duty_cycle = map((uint16_t)(C_stop_micros - C_start_micros),C_pulse_len_low,C_pulse_len_high,0,1023);
    }
}

void setup() {
  Serial.begin(115200);
  for (uint8_t i=0; i < inputs_len; i++){
      PCintPort::attachInterrupt(inputs[i].input_pin, &rc_pulse_high, RISING, &inputs[i]);
      PCintPort::attachInterrupt(inputs[i].input_pin, &rc_pulse_low, FALLING, &inputs[i]);
  }
  pinMode(Apin,INPUT);
  pinMode(Bpin,INPUT);
  pinMode(Cpin,INPUT);
  Serial.println(F("RC_I2C Started"));
}

void loop() {
  for (uint8_t i=0; i < inputs_len; i++){
    if (inputs[i].new_data){
      calculate(&inputs[i]);
    }
  }
  if(ABCticker==0){
    A_start_micros=pulseIn(Apin,HIGH);
  }
  if(ABCticker==1){
    A_stop_micros=pulseIn(Apin,LOW);
  }
  if(ABCticker==2){
    B_start_micros=pulseIn(Bpin,HIGH);
  }
  if(ABCticker==3){
    B_stop_micros=pulseIn(Bpin,LOW);
  }
  if(ABCticker==4){
    C_start_micros=pulseIn(Cpin,HIGH);
  }
  if(ABCticker==5){
    C_stop_micros=pulseIn(Cpin,LOW);
  }
  ABCticker++;
  if(ABCticker>5){
    ABCticker=0;
  }

  Acalculate();
  Bcalculate();
  Ccalculate();
  
  // Report positions every second
  if (millis() - last_report_time > 200){
      Serial.println();
      for (uint8_t i=0; i < inputs_len; i++){
          Serial.print(F("Channel "));
          Serial.print(inputs[i].rc_channel, DEC);
          Serial.print(F(" position "));
          Serial.println(inputs[i].avg_duty_cycle, DEC);
      }
      Serial.print(F("Channel A"));
      Serial.print(F(" position "));
      Serial.println(A_duty_cycle,DEC);
      Serial.print(F("Channel B"));
      Serial.print(F(" position "));
      Serial.println(B_duty_cycle,DEC);
      Serial.print(F("Channel C"));
      Serial.print(F(" position "));
      Serial.println(C_duty_cycle,DEC);
      last_report_time = millis();
   } 
}
