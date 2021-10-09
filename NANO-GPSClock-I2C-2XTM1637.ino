#include <Wire.h>
#include <TimeLib.h>                   // struct timeval
#include <TinyGPSPlus.h>
#include <SoftwareSerial.h>
#include <TM1637Display.h>
#include <RTClib.h>

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
RTC_DS3231 realTimeClock;
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
const int    SAMPLE_NUMBER      = 200;
const double BALANCE_RESISTOR   = 100300.0;
const double BETA               = 3950.0;
const double ROOM_TEMP          = 298.15; // 298.15
const double KELVIN_TO_CELCIUS  =  273.15; 
const double RESISTOR_ROOM_TEMP = 96700.0; // 92700
const double TEMPERATURE_CORRECTION = 0.25;
double currentTemperature = -30.0;
double mimimumTemperature = -30.0;
int thermistorPin = A0;
int vccPin = A2;
int gpsMinimumYear = 2020;
int gpsToSystemYearConversion = 1970;

void setup()   {
  delay(100);
  Serial.begin(115200);
  Serial.println("Program Begin.");
  display1.setBrightness(0xA);
  display1.setSegments(SEG_HELO);
  display2.setBrightness(0xA);
  display2.setSegments(SEG_HELO);
  Serial_GPS.begin(GPSBaud); // Start GPS Serial Connection
  if (!realTimeClock.begin()) {
    Serial.println("Couldn't find RTC");
    Serial.flush();
  }
  realTimeClock.disableAlarm(1); // turn off alarm 1
  realTimeClock.disableAlarm(2); // turn off alarm 2
  smartDelay(2000);
}

void getTemperature() {
  // variables that live in this function
  double rThermistor = 0;            // Holds thermistor resistance value
  double tKelvin     = 0;            // Holds calculated temperature
  double tCelsius    = 0;            // Hold temperature in celsius
  double adcAverage  = 0;            // Holds the average voltage measurement
  double adcVccAverage = 0;
  int    adcSamples[SAMPLE_NUMBER];  // Array to hold each voltage measurement
  int    adcVccSamples[SAMPLE_NUMBER];  // Array to hold each voltage measurement for Vcc
   
  /* Calculate thermistor's average resistance:
     As mentioned in the top of the code, we will sample the ADC pin a few times
     to get a bunch of samples. A slight delay is added to properly have the
     analogRead function sample properly */
  
  for (int i = 0; i < SAMPLE_NUMBER; i++) 
  {
    adcSamples[i] = analogRead(thermistorPin);  // read from pin and store
    adcVccSamples[i] = analogRead(vccPin);  // read from Vcc pin and store
  }

  /* Then, we will simply average all of those samples up for a "stiffer"
     measurement. */
  for (int i = 0; i < SAMPLE_NUMBER; i++) 
  {
    adcAverage += adcSamples[i];      // add all samples up . . .
    adcVccAverage += adcVccSamples[i];
  }
  adcAverage /= SAMPLE_NUMBER;        // . . . average it w/ divide
//Serial.print("adcAverage: ");
//Serial.println(adcAverage);

  adcVccAverage /= SAMPLE_NUMBER;
//Serial.print("adcVccAverage: ");
//Serial.println(adcVccAverage);
  /* Here we calculate the thermistor’s resistance using the equation 
     discussed in the article. */
  rThermistor = BALANCE_RESISTOR * adcAverage / (adcVccAverage - adcAverage);
//Serial.print("rThermistor: ");
//Serial.println(rThermistor);

  /* Here is where the Beta equation is used, but it is different
     from what the article describes. Don't worry! It has been rearranged
     algebraically to give a "better" looking formula. I encourage you
     to try to manipulate the equation from the article yourself to get
     better at algebra. And if not, just use what is shown here and take it
     for granted or input the formula directly from the article, exactly
     as it is shown. Either way will work! */

  tKelvin =  1 / ((1 / ROOM_TEMP) + ((log(rThermistor / RESISTOR_ROOM_TEMP)) / BETA));

/*     
  tKelvin = (BETA * ROOM_TEMP) / 
            (BETA + (ROOM_TEMP * log(rThermistor / RESISTOR_ROOM_TEMP)));
*/
  /* I will use the units of Celsius to indicate temperature. I did this
     just so I can see the typical room temperature, which is 25 degrees
     Celsius, when I first try the program out. I prefer Fahrenheit, but
     I leave it up to you to either change this function, or create
     another function which converts between the two units. */
  currentTemperature = tKelvin - KELVIN_TO_CELCIUS + TEMPERATURE_CORRECTION ;  // convert kelvin to celsius 
  Serial.print("currentTemperature: ");
  Serial.println(currentTemperature);
}

char* string2char(String command){
    if(command.length()!=0){
        char *p = const_cast<char*>(command.c_str());
        return p;
    }
}

void showTemperature() {
  if (currentTemperature < 0)
  {
    Serial.println("currentTemperature < 0");
    String temperatureString = String(-currentTemperature, 1);
    String temperatureToShow = "0 ";
    int dotPosition = temperatureString.indexOf(".");
    if (dotPosition > 0)
    {
      temperatureToShow += temperatureString.substring(0, dotPosition);
    }
    else
    {
      temperatureToShow += temperatureString.substring(0);
    }
    display2.showString(string2char(temperatureToShow));
  }
  else
  {
    String temperatureString = String(currentTemperature, 1);
    String temperatureToShow = "";
    int dotPosition = temperatureString.indexOf(".");
    if (dotPosition > 0)
    {
      temperatureToShow += temperatureString.substring(0, dotPosition);
      temperatureToShow += " ";
      temperatureToShow += temperatureString.substring(dotPosition + 1);
    }
    else
    {
      temperatureToShow += temperatureString.substring(0);
      temperatureToShow += " 0";
    }
    display2.showString(string2char(temperatureToShow));
  }
}

void loop() {
  getTemperature();  
  smartDelay(100);
  Year = gps.date.year();
  Serial.print("GPS Year: ");
  Serial.println(Year);
  int Month = gps.date.month();
  int Day = gps.date.day();
  int Hour = gps.time.hour();
  int Minute = gps.time.minute();
  int Second = gps.time.second();
  if (Year > gpsMinimumYear)
  {
    tmElements_t tm;
    tm.Second = Second;
    tm.Hour = Hour;
    tm.Minute = Minute;
    tm.Day = Day;
    tm.Month = Month;
    tm.Year = Year - gpsToSystemYearConversion  ;
    setTime(makeTime(tm) + UTC_offset * SECS_PER_HOUR);
    if (Second == 10)
    {
      realTimeClock.adjust(DateTime(year(), month(), day(), hour(), minute(), second()));
      Serial.println("RTC time set from GPS!");
    }
  }
  else
  {
    DateTime rtcNow = realTimeClock.now();
    int rtcYear = rtcNow.year();
    int rtcMonth = rtcNow.month();
    int rtcDay = rtcNow.day();
    int rtcHour = rtcNow.hour();
    int rtcMinute = rtcNow.minute();
    int rtcSecond = rtcNow.second();
    if (rtcYear > gpsMinimumYear)
    {
      tmElements_t tm;
      tm.Second = rtcSecond;
      tm.Hour = rtcHour;
      tm.Minute = rtcMinute;
      tm.Day = rtcDay;
      tm.Month = rtcMonth;
      tm.Year = rtcYear - gpsToSystemYearConversion;
      setTime(makeTime(tm));
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
  }

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

  if (timeStatus() != timeNotSet) {
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
       if (currentTemperature > mimimumTemperature)
       {
          showTemperature();
          smartDelay(3000);
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
