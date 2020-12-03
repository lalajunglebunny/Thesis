/*
  - PROJECT (E) 448 (SKIPSIE == FINAL YEAR PROJECT)
  - Jonathan Paul Hendricks, University of Stellenbosch, 2020
  - TOPIC: Long term-low cost acoustic monitor
  - Monitor dolphin sounds and other marine fauna.
*/

//Includes & Declarations & Constants

#include "WavFileWriter.hpp"
#include <SerialFlash.h>
#include <Audio.h>
#include <Wire.h>
#include <TimeLib.h>
#include <OpenAudio_ArduinoLibrary.h>
#define FILE_BASE_NAME "REC"
#define PACKETS 16

const int Fs = 44100;       // Frequency to sample
const int myMic = AUDIO_INPUT_MIC;        // Mic input
unsigned long prevMil = 0;
unsigned long prevMil2 = 0;
unsigned long prevMil3 = 0;
const long interval = 10000;
const long a = 5000;
int flag;
int flag2;
int flag3;



AudioPlaySdWav        kingSD;        //Kingston SD Card
AudioInputI2S         audioIn;       //Audio Connection
AudioOutputI2S        audioOut;      //Audio Connection
AudioOutputUSB        usb1;       //For USB testing
AudioRecordQueue      queue1;        //To write data to SD Card
//AudioAnalyzeRMS      rms1;
//AudioAnalyzePeak     peak1;
AudioConnection       patchCord1(audioIn, 0, queue1, 0);   //-Record data and queue it
//AudioConnection       patchCord2(kingSD, 0, audioOut, 0);  //-perform read/write functions to save
//AudioConnection       patchCord3(kingSD, 0, audioOut, 1);  // data and play through Audio Connection output
AudioConnection       patchCord4(audioIn, 0, usb1, 0);
AudioConnection       patchCord5(audioIn, 0, usb1, 1);
//AudioConnection       patchCord6(audioIn, 0 , peak1, 0);
AudioControlSGTL5000_Extended  AudioShield;

WavFileWriter wavWriter(queue1);
const uint8_t BASE_NAME_SIZE = sizeof(FILE_BASE_NAME) - 1;
char fileName[] = FILE_BASE_NAME "00.wav";

// ---------------

// Initial setup run at start of program

void setup() {
  AudioShield.enable();
  delay(500);
  SdFile::dateTimeCallback(dateTime);
  Serial.begin(9600);
  AudioMemory(60);
  AudioShield.inputSelect(myMic);
  //AudioShield.audioPreProcessorEnable();
 // AudioShield.audioPostProcessorEnable();
  AudioShield.micGain(63);          //0-63
 // AudioShield.volume(1.5);         //0-1
  AudioShield.micBiasEnable(3);
 // AudioShield.adcHighPassFilterDisable();
  setSyncProvider(getTeensy3Time); //Initialise this
  Serial.println("Powered up & Ready to go!");
}

// ---------------

//loop function

void loop() {

  if (Serial.available() > 0) {
    // read the incoming byte:
    byte incomingByte = Serial.read();


    if (incomingByte == '1') {
      flag = 1;
      unsigned long currMil = millis();
      prevMil3 = currMil;
      prevMil2 = currMil;
      prevMil = currMil;
      flag2 = 1;
      flag3 = 0;
      //Serial.println("Start Recording");
    }

    while ( incomingByte == '1' && flag == 1) {

      unsigned long currMil = millis();

      if (currMil - prevMil >= interval) {
        while (SD.exists(fileName)) {
          if (fileName[BASE_NAME_SIZE + 1] != '9') {
            fileName[BASE_NAME_SIZE + 1]++;
          } else if (fileName[BASE_NAME_SIZE] != '9') {
            fileName[BASE_NAME_SIZE + 1] = '0';
            fileName[BASE_NAME_SIZE]++;
          } else {
            Serial.println(F("Can't create file name"));
            return;
          }
        }

        prevMil = currMil;
        Serial.println("Start recording!");
        wavWriter.open(fileName, Fs, 1);
      }
      if (currMil - prevMil2 >= a + interval) {
        prevMil2 = currMil - a;
        wavWriter.close();
        Serial.println("Stop Recording");
        digitalClockDisplay();
      }
      if ((currMil - prevMil3) >= a && flag2 == 1) {

        Serial.println("Stop recording");
        wavWriter.close();
        flag2 = 0;
        digitalClockDisplay();
      }
      if (prevMil > 7200000) {
        Serial.println("Stop recording");
        flag = 2;
        wavWriter.close();
        digitalClockDisplay();
      }
      if (flag == 1 && flag3 == 0) {
        Serial.println("Start Recording");
        wavWriter.open(fileName, Fs, 1);
        flag3 = 1;
      }
      if (wavWriter.isWriting()) {
        //adjustMicLevel();
        wavWriter.update();
        //Serial.println(wavWriter.update());
      }
      
    }
  }
   if (myMic == AUDIO_INPUT_MIC) adjustMicLevel();
}

// ---------------

/*Additional functions for implementation of acoustic monitor*/

//Timing functions

time_t getTeensy3Time() {
  return Teensy3Clock.get();
}

void digitalClockDisplay() {
  // digital clock display of the time
  Serial.print(hour());
  printDigits(minute());
  printDigits(second());
  Serial.print(" ");
  Serial.print(day());
  Serial.print(" ");
  Serial.print(month());
  Serial.print(" ");
  Serial.print(year());
  Serial.println();
}

/*  code to process time sync messages from the serial port   */
#define TIME_HEADER  "T"   // Header tag for serial time sync message

unsigned long processSyncMessage() {
  unsigned long pctime = 0L;
  const unsigned long DEFAULT_TIME = 1357041600; // Jan 1 2013

  if (Serial.find(TIME_HEADER)) {
    pctime = Serial.parseInt();
    return pctime;
    if ( pctime < DEFAULT_TIME) { // check the value is a valid time (greater than Jan 1 2013)
      pctime = 0L; // return 0 to indicate that the time is not valid
    }
  }
  return pctime;
}

void printDigits(int digits) {
  // utility function for digital clock display: prints preceding colon and leading 0
  Serial.print(":");
  if (digits < 10)
    Serial.print('0');
  Serial.print(digits);
}

void dateTime(uint16_t* date, uint16_t* time)
{
  *date = FAT_DATE(year(), month(), day());
  *time = FAT_TIME(hour(), minute(), second());
}

void adjustMicLevel() {
  if (peak1.available()) {
    if (peak1.read() == 1) {
      AudioShield.micGain(0);
      Serial.println("Gain = 0 dB, signal results in clipping");
      digitalClockDisplay();
    } else {
      AudioShield.micGain(20);
      //Serial.println(peak1.read());
    }
  }
}
