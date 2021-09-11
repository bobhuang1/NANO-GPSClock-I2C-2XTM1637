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

// MF52D 2K, 3435 25C
const int    SAMPLE_NUMBER      = 100;
const double BALANCE_RESISTOR   = 2000.0;
const double MAX_ADC            = 1023.0;
const double BETA               = 3435.0;
const double ROOM_TEMP          = 298.15;
const double RESISTOR_ROOM_TEMP = 2000.0;
double currentTemperature = -60.0;
double mimimumTemperature = -60.0;
int thermistorPin = A1;

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

void getTemperature() {
  // variables that live in this function
  double rThermistor = 0;            // Holds thermistor resistance value
  double tKelvin     = 0;            // Holds calculated temperature
  double tCelsius    = 0;            // Hold temperature in celsius
  double adcAverage  = 0;            // Holds the average voltage measurement
  int    adcSamples[SAMPLE_NUMBER];  // Array to hold each voltage measurement

  /* Calculate thermistor's average resistance:
     As mentioned in the top of the code, we will sample the ADC pin a few times
     to get a bunch of samples. A slight delay is added to properly have the
     analogRead function sample properly */
  
  for (int i = 0; i < SAMPLE_NUMBER; i++) 
  {
    adcSamples[i] = analogRead(thermistorPin);  // read from pin and store
    smartDelay(2);        // wait 10 milliseconds
  }

  /* Then, we will simply average all of those samples up for a "stiffer"
     measurement. */
  for (int i = 0; i < SAMPLE_NUMBER; i++) 
  {
    adcAverage += adcSamples[i];      // add all samples up . . .
  }
  adcAverage /= SAMPLE_NUMBER;        // . . . average it w/ divide

  /* Here we calculate the thermistor’s resistance using the equation 
     discussed in the article. */
  rThermistor = BALANCE_RESISTOR * ( (MAX_ADC / adcAverage) - 1);

  /* Here is where the Beta equation is used, but it is different
     from what the article describes. Don't worry! It has been rearranged
     algebraically to give a "better" looking formula. I encourage you
     to try to manipulate the equation from the article yourself to get
     better at algebra. And if not, just use what is shown here and take it
     for granted or input the formula directly from the article, exactly
     as it is shown. Either way will work! */
  tKelvin = (BETA * ROOM_TEMP) / 
            (BETA + (ROOM_TEMP * log(rThermistor / RESISTOR_ROOM_TEMP)));

  /* I will use the units of Celsius to indicate temperature. I did this
     just so I can see the typical room temperature, which is 25 degrees
     Celsius, when I first try the program out. I prefer Fahrenheit, but
     I leave it up to you to either change this function, or create
     another function which converts between the two units. */
  currentTemperature = tKelvin - 273.15 + 0.5;  // convert kelvin to celsius 
}

void showTemperature() {
    display2.showNumberDec(currentTemperature);
}

void loop() {
  getTemperature();  
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
        if (currentTemperature > mimimumTemperature)
        {
          showTemperature();
        }
        else
        {
          display2.setSegments(SEG_GPS1);
        }
      }
      else if (GPSAnimationCounter == 1)
      {
        display1.setSegments(SEG_GPS2);
        if (currentTemperature > mimimumTemperature)
        {
          showTemperature();
        }
        else
        {
          display2.setSegments(SEG_GPS2);
        }
      }
      else if (GPSAnimationCounter == 2)
      {
        display1.setSegments(SEG_GPS3);
        if (currentTemperature > mimimumTemperature)
        {
          showTemperature();
        }
        else
        {
          display2.setSegments(SEG_GPS3);
        }
      }
      else if (GPSAnimationCounter == 3)
      {
        display1.setSegments(SEG_GPS4);
        if (currentTemperature > mimimumTemperature)
        {
          showTemperature();
        }
        else
        {
          display2.setSegments(SEG_GPS4);
        }
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
        smartDelay(500);
        uint8_t segto = 0x80 | display1.encodeDigit((Clock / 100) % 10);
        display1.setSegments(&segto, 1, 1);
        smartDelay(500);
      }
      int Clock = month() * 100 + day();
      display2.showNumberDec(Clock, true);
      if (counter == 10)
      {
        showTemperature();
        smartDelay(2000);
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
