#include <Wire.h>
#include <TimeLib.h>                   // struct timeval
#include <TinyGPSPlus.h>
#include <SoftwareSerial.h>
#include <TM1637TinyDisplay.h>
#include <RTClib.h>
#include "StringHelpers.h"

#define RXPin 3
#define TXPin 2
#define GPSBaud 4800
#define UTC_offset 8 // China Standard Time
#define SECS_PER_HOUR  3600

#define HELLOTEXT "HELO"

#define CLKPin1 9
#define DIOPin1 8
#define CLKPin2 6
#define DIOPin2 5
TM1637TinyDisplay display1(CLKPin1, DIOPin1);
TM1637TinyDisplay display2(CLKPin2, DIOPin2);

TinyGPSPlus gps;
RTC_DS3231 realTimeClock;
SoftwareSerial Serial_GPS = SoftwareSerial(RXPin, TXPin);
time_t prevDisplay = 0; // Count for when time last displayed

const int GPSTEXT_SIZE = 16; // WAIT FOR GPS----
const char *GPSTEXT[GPSTEXT_SIZE] = {"WAIT", "AIT ", "IT F", "T FO", " FOR", "FOR ", "OR G", "R GP", " GPS", "GPS-", "PS--", "S---", "----", "---W", "--WA", "-WAI"};

int Year = -1;
int counter = 0;
int GPSAnimationCounter = 0;
long prevAnimationDisplay = 0; // Count for when time last displayed
long prevTemperatureUpdate = 0;
#define TEMPERATURE_UPDATE_INTERVAL_MS 2000 // engine bay temp changes slowly - no need to resample every loop

// 10K NTC thermistor, B = 3950 (the common 3D-printer type, see Wiring.txt), on the low
// side of a divider with a 10K balance resistor to the A2 reference. If your balance
// resistor differs, measure it and put the measured value in BALANCE_RESISTOR.
const int    SAMPLE_NUMBER      = 200;
const double BALANCE_RESISTOR   = 10000.0;
const double BETA               = 3950.0;
const double ROOM_TEMP          = 298.15; // 25 C in kelvin
const double KELVIN_TO_CELCIUS  =  273.15;
const double RESISTOR_ROOM_TEMP = 10000.0; // thermistor resistance at 25 C
const double TEMPERATURE_CORRECTION = 0.25;
double currentTemperature = -40.0;
double mimimumTemperature = -40.0;
int thermistorPin = A0;
int vccPin = A2;
int gpsMinimumYear = 2020;
#define GPS_MAX_AGE_MS 1500UL      // older GPS time is stale: fall back to the RTC
bool rtcOk = false;                // DS3231 answered on I2C at boot
bool rtcLostPower = false;         // DS3231 reported an oscillator stop (dead backup battery)
int gpsToSystemYearConversion = 1970;

void setup()   {
  delay(100);
  Serial.begin(115200);
  Serial.println("Program Begin.");
  display1.clear();
  display2.clear();
  display1.setBrightness(4);
  display2.setBrightness(4);
  display1.showString(HELLOTEXT);
  getTemperature();
  showTemperature();
  Serial_GPS.begin(GPSBaud); // Start GPS Serial Connection
  rtcOk = realTimeClock.begin();
  if (!rtcOk) {
    Serial.println("Couldn't find RTC");
    Serial.flush();
  } else {
    rtcLostPower = realTimeClock.lostPower();
  }
  realTimeClock.disableAlarm(1); // turn off alarm 1
  realTimeClock.disableAlarm(2); // turn off alarm 2
  smartDelay(1000);
}

void getTemperature() {
  // variables that live in this function
  double rThermistor = 0;            // Holds thermistor resistance value
  double tKelvin     = 0;            // Holds calculated temperature
  double tCelsius    = 0;            // Hold temperature in celsius
  double adcAverage  = 0;            // Holds the average voltage measurement
  double adcVccAverage = 0;
  // static instead of stack locals: 2 x 200 ints = 800 B would otherwise sit on
  // the stack of an ATmega328 (2 KB total RAM) on every call. Reuse between calls
  // is safe - this function never runs re-entrantly or from an interrupt.
  static int adcSamples[SAMPLE_NUMBER];       // Array to hold each voltage measurement
  static int adcVccSamples[SAMPLE_NUMBER];    // Array to hold each voltage measurement for Vcc
   
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
  if (adcVccAverage - adcAverage == 0)
  {
    // Thermistor shorted/open or an ADC misread: keep the previous reading
    // instead of dividing by zero (which yields inf and corrupts the beta math).
    Serial.println("getTemperature: degenerate ADC reading, keeping previous value");
    return;
  }
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

void showTemperature() {
  display2.clear();
  display2.showString("\xB0", 1, 3);        // Degree Mark, length=1, position=3 (right)
  display2.showNumber(currentTemperature, 1, 3, 0);    // Number, length=3, position=0 (left)
}

// Hour -> TM1637 brightness level (0-7). Covers all 24 hours explicitly so the
// mapping is auditable at a glance; the previous if/else chain had an
// unreachable final else and was fragile to edits.
const uint8_t HOUR_BRIGHTNESS[24] = {
  1, 1, 1, 1, 1, 2,        // 00-05 night
  3, 4, 5, 7, 7, 7,        // 06-11
  7, 7, 7, 7, 5, 4,        // 12-17
  3, 3, 2, 2, 1, 1         // 18-23
};

void setAllBrightness() {
  int brightnessLevel = HOUR_BRIGHTNESS[hour() % 24];
  display1.setBrightness(brightnessLevel);
  display2.setBrightness(brightnessLevel);
}

void loop() {
  if (millis() - prevTemperatureUpdate > TEMPERATURE_UPDATE_INTERVAL_MS)
  {
    prevTemperatureUpdate = millis();
    getTemperature();
  }
  smartDelay(100);
  Year = gps.date.year();
  Serial.print("GPS Year: ");
  Serial.println(Year);
  int Month = gps.date.month();
  int Day = gps.date.day();
  int Hour = gps.time.hour();
  int Minute = gps.time.minute();
  int Second = gps.time.second();
  // TinyGPSPlus keeps the last decoded date/time forever, so a stale value would keep
  // resetting the clock to the moment the fix was lost (tunnels, garages). Only trust a
  // timestamp decoded within the last 1.5 s; otherwise run from the RTC.
  bool gpsFresh = gps.date.isValid() && gps.time.isValid()
                  && gps.time.age() < GPS_MAX_AGE_MS
                  && Year > gpsMinimumYear;
  if (gpsFresh)
  {
    tmElements_t tm;
    tm.Second = Second;
    tm.Hour = Hour;
    tm.Minute = Minute;
    tm.Day = Day;
    tm.Month = Month;
    tm.Year = Year - gpsToSystemYearConversion  ;
    setTime(makeTime(tm) + UTC_offset * SECS_PER_HOUR);
    DateTime rtcNow = realTimeClock.now();
    int rtcYear = rtcNow.year();
    int rtcMonth = rtcNow.month();
    int rtcDay = rtcNow.day();
    int rtcHour = rtcNow.hour();
    int rtcMinute = rtcNow.minute();
    if (rtcOk && (rtcLostPower || rtcYear != year() || rtcMonth != month() || rtcDay != day() || rtcHour != hour() || rtcMinute != minute()))
    {
      Serial.println("RTC and system clock does not match! Setting now!");
      realTimeClock.adjust(DateTime(year(), month(), day(), hour(), minute(), second()));
      rtcLostPower = false; // adjust() restarts the oscillator with a known-good time
      Serial.println("RTC time set from GPS!");
      Serial.print("After set year: ");
      Serial.println(realTimeClock.now().year());
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
    // A missing DS3231 or one that lost its backup battery returns nonsense (often
    // year 2165); show the "WAIT FOR GPS" animation instead of a garbage time.
    if (rtcOk && !rtcLostPower && rtcYear > gpsMinimumYear && rtcYear < 2100)
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
        display1.showString(GPSTEXT[GPSAnimationCounter]);
        if (currentTemperature > mimimumTemperature)
        {
          showTemperature();
        }
        else
        {
          display2.showString(GPSTEXT[GPSAnimationCounter]);
        }
        GPSAnimationCounter++;
        if (GPSAnimationCounter >= GPSTEXT_SIZE)
        {
          GPSAnimationCounter = 0;
        }
      }
    }
  }

  setAllBrightness();
  
  if (timeStatus() != timeNotSet) {
    if (now() != prevDisplay) {
      prevDisplay = now();
      if (counter < 10)
      {
        int Clock = hour() * 100 + minute();
        if (Clock < 100)
        {
          display1.showNumber(Clock, true);
        }
        else
        {
          display1.showNumber(Clock, false);
        }
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
