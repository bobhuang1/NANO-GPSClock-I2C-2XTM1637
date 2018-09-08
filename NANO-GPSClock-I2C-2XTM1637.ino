#include <Wire.h>
#include <TimeLib.h>                   // struct timeval
#include <TinyGPS++.h>
#include <SoftwareSerial.h>
#include <TM1637Display.h>

#define RXPin 3
#define TXPin 2
#define GPSBaud 4800
#define UTC_offset 8 // China Standard Time
#define SECS_PER_HOUR  3600

#define CLKPin1 9
#define DIOPin1 8
#define CLKPin2 6
#define DIOPin2 5
TM1637Display display1(CLKPin1, DIOPin1);
TM1637Display display2(CLKPin2, DIOPin2);

TinyGPSPlus gps;
SoftwareSerial Serial_GPS = SoftwareSerial(RXPin, TXPin);
time_t prevDisplay = 0; // Count for when time last displayed
const uint8_t SEG_GPS1[] = {
  SEG_A | SEG_C | SEG_D | SEG_E | SEG_F,   // G
  SEG_A | SEG_B | SEG_E | SEG_F | SEG_G,   // P
  SEG_A | SEG_C | SEG_D | SEG_F | SEG_G,   // S
  SEG_G                                    // -
};

const uint8_t SEG_GPS2[] = {
  SEG_A | SEG_B | SEG_E | SEG_F | SEG_G,   // P
  SEG_A | SEG_C | SEG_D | SEG_F | SEG_G,   // S
  SEG_G,                                   // -
  SEG_A | SEG_C | SEG_D | SEG_E | SEG_F    // G
};

const uint8_t SEG_GPS3[] = {
  SEG_A | SEG_C | SEG_D | SEG_F | SEG_G,   // S
  SEG_G,                                   // -
  SEG_A | SEG_C | SEG_D | SEG_E | SEG_F,   // G
  SEG_A | SEG_B | SEG_E | SEG_F | SEG_G    // P
};

const uint8_t SEG_GPS4[] = {
  SEG_G,                                   // -
  SEG_A | SEG_C | SEG_D | SEG_E | SEG_F,   // G
  SEG_A | SEG_B | SEG_E | SEG_F | SEG_G,   // P
  SEG_A | SEG_C | SEG_D | SEG_F | SEG_G    // S
};

const uint8_t SEG_HELO[] = {
  SEG_B | SEG_C | SEG_E | SEG_F | SEG_G,   // H
  SEG_A | SEG_D | SEG_E | SEG_F | SEG_G,   // E
  SEG_D | SEG_E | SEG_F,   // L
  SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F   // O
};

int Year = -1;
int counter = 0;
int GPSAnimationCounter = 0;
long prevAnimationDisplay = 0; // Count for when time last displayed
long prevTimeUpdate = 0;

void setup()   {
  delay(100);
  Serial.begin(115200);
  display1.setBrightness(0xA);
  display1.setSegments(SEG_HELO);
  display2.setBrightness(0xA);
  display2.setSegments(SEG_HELO);
  Serial_GPS.begin(GPSBaud); // Start GPS Serial Connection
  smartDelay(2000);
}

void loop() {
  smartDelay(100);
  Year = gps.date.year();
  byte Month = gps.date.month();
  byte Day = gps.date.day();
  byte Hour = gps.time.hour();
  byte Minute = gps.time.minute();
  byte Second = gps.time.second();
  if (Year > 2017)
  {
    tmElements_t tm;
    tm.Second = Second;
    tm.Hour = Hour;
    tm.Minute = Minute;
    tm.Day = Day;
    tm.Month = Month;
    tm.Year = Year - 1970;
    setTime(makeTime(tm) + UTC_offset * SECS_PER_HOUR);
  }
  else
  {
    if (millis() - prevAnimationDisplay > 200)
    {
      prevAnimationDisplay = millis();
      if (GPSAnimationCounter == 0)
      {
        display1.setSegments(SEG_GPS1);
        display2.setSegments(SEG_GPS1);
      }
      else if (GPSAnimationCounter == 1)
      {
        display1.setSegments(SEG_GPS2);
        display2.setSegments(SEG_GPS2);
      }
      else if (GPSAnimationCounter == 2)
      {
        display1.setSegments(SEG_GPS3);
        display2.setSegments(SEG_GPS3);
      }
      else if (GPSAnimationCounter == 3)
      {
        display1.setSegments(SEG_GPS4);
        display2.setSegments(SEG_GPS4);
      }
      GPSAnimationCounter++;
      if (GPSAnimationCounter > 3)
      {
        GPSAnimationCounter = 0;
      }
    }
  }

  if (timeStatus() != timeNotSet) {
    if (hour() >= 6 && hour() < 18)
    {
      display1.setBrightness(0xF);
      display2.setBrightness(0xF);

    }
    else
    {
      display1.setBrightness(0x9);
      display2.setBrightness(0x9);
    }

    if (now() != prevDisplay) {
      prevDisplay = now();
      if (counter < 10)
      {
        int Clock = hour() * 100 + minute();
        display1.showNumberDec(Clock, true);
        Serial.println(Clock);
        smartDelay(500);
        uint8_t segto = 0x80 | display1.encodeDigit((Clock / 100) % 10);
        display1.setSegments(&segto, 1, 1);
        smartDelay(500);
      }
      int Clock = month() * 100 + day();
      display2.showNumberDec(Clock, true);
      if (counter == 10)
      {
        for (int i = 0; i < 20; ++i)
        {
          int Speed = round(gps.speed.kmph());
          if (Speed > 0 && Speed < 200)
          {
            display2.showNumberDec(Speed, false);
            smartDelay(100);
          }
        }
      }
      counter++;
      if (counter > 10)
      {
        counter = 0;
      }
    }
  }
}

// This custom version of delay() ensures that the gps object
// is being "fed".
static void smartDelay(unsigned long ms)
{
  unsigned long start = millis();
  do
  {
    while (Serial_GPS.available())
      gps.encode(Serial_GPS.read());
  } while (millis() - start < ms);
}

