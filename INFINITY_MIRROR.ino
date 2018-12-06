#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <NeoPixelBus.h>
#include "CONSTANTS.h"

//#define DEBUG

/********************************** Global Variables *******************************/
 
int16_t HueDelta[Panes]; // Hue of LEDs (temperature)
int16_t HueTarget[Panes];
int16_t HueCurrent[Panes];
int8_t  HueSign[Panes];
int16_t HueError[Panes];

int8_t  BriDelta[Panes]; // Low brightness of twinkling LEDs (rain)
int16_t BriTarget[Panes];
int8_t  BriCurrent[Panes];
int8_t  BriSign[Panes];
int16_t BriError[Panes];

int8_t  TwiDelta[Panes];  // Probability an LED will Twinkle (rain)
int16_t TwiTarget[Panes];
int8_t  TwiCurrent[Panes];
int8_t  TwiSign[Panes];
int16_t TwiError[Panes];

int8_t  SatDelta[Panes]; // Saturation of wind LED (wind speed)
int16_t SatTarget[Panes];
int8_t  SatCurrent[Panes];
int8_t  SatSign[Panes];
int16_t SatError[Panes];

int16_t AngDelta[Panes]; // Angle of wind LED (wind direction)
int16_t AngTarget[Panes];
int16_t AngCurrent[Panes];
int8_t  AngSign[Panes];
int16_t AngError[Panes];
int16_t WindLED[Panes]; // Index of wind LED

float   Temperature[Panes];  // API variables
float   WindSpeed[Panes];
float   WindBearing[Panes];
float   PrecipProb[Panes];
uint32_t UNIXTime; // int64_t will not compile. use uint32_t, will work till year 2106. int32_t will work till year 2038

int16_t TwinkleBuffer[TwinkleFrames*Panes];  // ring buffer. position in ring determines stage of twinkling
int16_t TwinkleIndex = 0;   // position in the twinkle buffer

bool FirstPass = true;

// initialize pixels
NeoPixelBus<NeoGrbFeature, Neo800KbpsMethod> strip(PixelCount, PixelPin);

/********************************** Functions **************************************/

void GetWeather() { // gets the weather from the api
  WiFiClientSecure client;

  // Connect to host
  if (!client.connect(APIHost, 443)) {
    #ifdef DEBUG
    Serial.println(F("! Connection failed !")); Serial.println();
    #endif
    return;
  }
  
  // Send GET Request
  client.print(String("GET ") + APIURL + " HTTP/1.1\r\n" +
               "Host: " + APIHost + "\r\n" +
               "User-Agent: ESP8266\r\n" +
               "Connection: close\r\n\r\n");
  
  // Timeout if no response
  uint32_t timeout = millis();
  while (!client.connected() && !client.available()) {
    if (millis() - timeout > 5000) {
      #ifdef DEBUG
      Serial.println(F("! Client timeout !"));
      #endif
      client.stop();
      return;
    }
  }
  
  if (client.find("\"currently\":")) {    // skips the headers and anything else before
    StaticJsonBuffer<jsonSize> jsonBuffer;
    JsonObject& interval = jsonBuffer.parseObject(client);
    if (interval.success()) {
      UNIXTime = interval["time"];
      int8_t j = WeatherIntervalIndex(0);  // find the index of value 0 in WeatherInterval[];
      if (j >= 0) {
        Temperature[j]  = interval["temperature"];
        WindSpeed[j]    = interval["windSpeed"];
        WindBearing[j]  = interval["windBearing"];
        PrecipProb[j]   = interval["precipProbability"];
      }
    }
  }
  
  if (client.find("\"hourly\":")) { // skips the headers and anything else before
    client.find('[');               // skips to the first item in the interval data list
    client.find("},");              // advance to the next interval because we've already addressed 0
    for (int8_t i=1; i < 49; i++) { // 1 to 48 intervals of forecast
      int8_t j = WeatherIntervalIndex(i);  // find the index of value i in WeatherInterval[];
      if (j >= 0) {
        StaticJsonBuffer<jsonSize> jsonBuffer;
        JsonObject& interval = jsonBuffer.parseObject(client);
        if (interval.success()) {
          Temperature[j]  = interval["temperature"];
          WindSpeed[j]    = interval["windSpeed"];
          WindBearing[j]  = interval["windBearing"];
          PrecipProb[j]   = interval["precipProbability"];
        }
      } else {
        client.find("},"); // advance to the next interval
      }
    }
  }
  client.stop();
  
  // deal with duplicate panes
  DuplicatePanes();

  #ifdef DEBUG
  Serial.println(F("API Data"));
  Serial.print(F("     UNIX Time: ")); Serial.println(UNIXTime);
  for (int8_t i=0; i < Panes; i++) {
    Serial.print(F(" Temperature ")); Serial.print(i); Serial.print(F(": ")); Serial.print(Temperature[i]); Serial.println(F("°C"));
    Serial.print(F("  Wind Speed ")); Serial.print(i); Serial.print(F(": ")); Serial.print(WindSpeed[i]);   Serial.println(F("m/s"));
    Serial.print(F("Wind Bearing ")); Serial.print(i); Serial.print(F(": ")); Serial.print(WindBearing[i]); Serial.println(F("°"));
    Serial.print(F("      P.o.P. ")); Serial.print(i); Serial.print(F(": ")); Serial.print(100*PrecipProb[i]);  Serial.println(F("%"));
  }
  Serial.println();
  #endif
}

int8_t WeatherIntervalIndex(int8_t element) {
  for (int8_t i = 0; i < Panes; i++) {
    if (WeatherInterval[i] == element) {
      return i;
    }
  }
  return -1;
}

void DuplicatePanes() {
  for (int8_t i = 0; i < Panes; i++) {
    for (int8_t j = i+1; j < Panes; j++) {
      if (WeatherInterval[j] == WeatherInterval[i]) {
        Temperature[j]  = Temperature[i];
        WindSpeed[j]    = WindSpeed[i];
        WindBearing[j]  = WindBearing[i];
        PrecipProb[j]   = PrecipProb[i];
      }
    }
  }
}

void FadeLEDs() {
  // For each step in the fade adjust the values of each pane
  for (int16_t j=0; j < FadeSteps; j++) {
    
    // FadeLEDs is a long blocking loop so call yield() within it
    // https://stackoverflow.com/questions/34497758/what-is-the-secret-of-the-arduino-yieldfunction
    yield();
      
    for (uint8_t i=0; i < Panes; i++) {
      // Previously this was done with interpolation but that was a whole bunch of math and it was slooooow.
      // The modified Bresenham's line algorithm is much faster.
      // Modified Bresenham's line algorithm begins. Replaced "if" with "while" so slopes greater than one can be accomadated.
      HueError[i] -= HueDelta[i];
      while (HueError[i] < 0) { HueCurrent[i] += HueSign[i]; HueError[i] += FadeSteps; }

      BriError[i] -= BriDelta[i];
      while (BriError[i] < 0) { BriCurrent[i] += BriSign[i]; BriError[i] += FadeSteps; }
      
      TwiError[i] -= TwiDelta[i];
      while (TwiError[i] < 0) { TwiCurrent[i] += TwiSign[i]; TwiError[i] += FadeSteps; }

      SatError[i] -= SatDelta[i];
      while (SatError[i] < 0) { SatCurrent[i] += SatSign[i]; SatError[i] += FadeSteps; }

      AngError[i] -= AngDelta[i];
      while (AngError[i] < 0) { AngCurrent[i] += AngSign[i]; AngError[i] += FadeSteps; }
      // Modified Bresenham's line algorithm ends

      // determine LED that indicates the angle
      int16_t Windex = 0; // wind LED index
      while (WindDirArray[Windex] < (AngCurrent[i] % 360)) {
        Windex++;
      }
      WindLED[i] = WindLEDArray[Windex]+(i*PixPerPane);
      
      // For each pixel in the pane set the base colour
      for (int16_t k=i*PixPerPane; k < (i+1)*PixPerPane; k++) {
        strip.SetPixelColor(k, HsbColor(HueCurrent[i]/360.0f, 1.0f, MaxBri/100.0f));
      }
      // Set the wind indicator LED
      strip.SetPixelColor(WindLED[i], HsbColor(HueCurrent[i]/360.0f, SatCurrent[i]/100.0f, MaxBri/100.0f));
    }
    
    #ifdef DEBUG
    Serial.print(F("Step ")); Serial.print(j+1); Serial.print(F(" of ")); Serial.println(FadeSteps);
    for (int8_t i=0; i < Panes; i++) {
      Serial.print(F("       Hue ")); Serial.print(i); Serial.print(F(": ")); Serial.println(HueCurrent[i]);
      Serial.print(F("Brightness ")); Serial.print(i); Serial.print(F(": ")); Serial.print(BriCurrent[i]); Serial.println(F("%"));
      Serial.print(F("   Twinkle ")); Serial.print(i); Serial.print(F(": ")); Serial.print(TwiCurrent[i]); Serial.println(F("%"));
      Serial.print(F("Saturation ")); Serial.print(i); Serial.print(F(": ")); Serial.print(SatCurrent[i]); Serial.println(F("%"));
      Serial.print(F("Wind Angle ")); Serial.print(i); Serial.print(F(": ")); Serial.print(AngCurrent[i]); Serial.println(F("°"));
      Serial.print(F("  Wind LED ")); Serial.print(i); Serial.print(F(": ")); Serial.println(WindLED[i]);
    }
    Serial.println();
    #endif
    
    // Twinkle the LEDs 
    for (int16_t i=0; i < FramesPerStep; i++) {
      TwinkleLEDs();
    }
  }
}

void TwinkleLEDs() {
  for (int8_t i=0; i < Panes; i++) { // for each pane
    int8_t  k = TwinkleIndex + (i*TwinkleFrames);
    
    // set the oldest pixel back to full brightness
    if (TwinkleBuffer[k] >= 0) {
      strip.SetPixelColor(TwinkleBuffer[k], HsbColor(HueCurrent[i]/360.0f, 1.0f, MaxBri/100.0f));
    }
    
    // Test if there is going to be a twinkling LED
    if (random(100) < TwiCurrent[i]) {
      // pick a new random pixel in the pane (except the wind LED) and save it to the oldest spot in the circular buffer
      do {
        TwinkleBuffer[k] = random(i*PixPerPane, (i+1)*PixPerPane);
      } while (TwinkleBuffer[k] == WindLED[i]);
    } else {
      // put a null value in the oldest spot in the circular buffer
      TwinkleBuffer[k] = -1;
    }
    
    // set the brightness of each pixel in the circular buffer. Write oldest to newest so newest overwrite oldest if there is a duplicate pixel. Oldest are the brightest, newest are the dimmest.
    for (int8_t j=1; j <= TwinkleFrames; j++) {
      k = ((j + TwinkleIndex) % TwinkleFrames) + (i*TwinkleFrames);
      if (TwinkleBuffer[k] >= 0) {
        strip.SetPixelColor(TwinkleBuffer[k], HsbColor(HueCurrent[i]/360.0f, 1.0f, ((TwinkleFrames-j)*(MaxBri-BriCurrent[i])/TwinkleFrames + BriCurrent[i])/100.0f));
      }
    }
  }

  // Advance the circular buffer
  TwinkleIndex = (TwinkleIndex + 1) % TwinkleFrames;

  // push data to LEDs
  strip.Show();

  // delay for frame period
  delay(FramePeriod);
}

void SetTargets() {
  // Determine the day of the year
  int16_t Day = (UNIXTime % 31556952)/86400; // This doesn't need to be super accurate. Leap years are accounted for in SecInYear. SecInYear = 365.2425*24*60*60 = 31556952. SecInDay = 24*60*60 = 86400.
  
  int16_t k = 1;
  while (NormalDay[k] < Day) {
    k++;
  }

  // Set seasonal temp limits
  // Interpolate between (k-1) and k
  float MaxTemp  = (((Day-NormalDay[k])*1.0f) * (NormalTemp[k]-NormalTemp[k-1]) / (NormalDay[k]-NormalDay[k-1])) + NormalTemp[k];
  float MinTemp  =  MaxTemp - OffsetTemp;
  MaxTemp        =  MaxTemp + OffsetTemp;

  #ifdef DEBUG
  Serial.println(F("Weather Limits"));
  Serial.print(F("     Day: ")); Serial.println(Day);
  Serial.print(F("Max Temp: ")); Serial.print(MaxTemp); Serial.println(F("°C"));
  Serial.print(F("Min Temp: ")); Serial.print(MinTemp); Serial.println(F("°C"));
  Serial.print(F("Max Wind: ")); Serial.print(MaxWind); Serial.println(F("m/s"));
  Serial.println();
  #endif
  
  for (int8_t i=0; i < Panes; i++) {
    // Save the previous targets as the new current values
    HueCurrent[i] = HueTarget[i];
    BriCurrent[i] = BriTarget[i];
    TwiCurrent[i] = TwiTarget[i];
    SatCurrent[i] = SatTarget[i];
    AngCurrent[i] = AngTarget[i] % 360; // AngCurrent should be between 0 and 359 until we decide if it shouldn't be
    
    // Convert temperature to hue
    // MaxTemp --> MinHue, MinTemp --> MaxHue
    HueTarget[i] = ((Temperature[i]-MinTemp)/(MaxTemp-MinTemp))*(MinHue-MaxHue) + MaxHue;
    if (HueTarget[i] < MinHue) { HueTarget[i] = MinHue; }
    else if (HueTarget[i] > MaxHue) { HueTarget[i] = MaxHue; }

    // Convert precipitation probability to brightness and twinkle probability
    // No Rain --> HiMinBri, MaxRain --> LoMinBri
    BriTarget[i] = PrecipProb[i]*(LoMinBri-HiMinBri) + HiMinBri;
    //if (BriTarget[i] > HiMinBri) { BriTarget[i] = HiMinBri; }
    //else if (BriTarget[i] < LoMinBri) { BriTarget[i] = LoMinBri; }

    // No Rain --> No Twinkle, MaxRain --> MaxTwinkProb
    TwiTarget[i] = PrecipProb[i]*MaxTwinkProb;
    //if (TwiTarget[i] > MaxTwinkProb) { TwiTarget[i] = MaxTwinkProb; }
    //else if (TwiTarget[i] < 0) { TwiTarget[i] = 0; }
    
    // Convert wind speed to saturation
    // 0kph Wind --> MaxSat, MaxWind --> MinSat
    SatTarget[i] = (WindSpeed[i]/MaxWind)*(MinSat-MaxSat) + MaxSat;
    if (SatTarget[i] > MaxSat) { SatTarget[i] = MaxSat; }
    //else if (SatTarget[i] < MinSat) { SatTarget[i] = MinSat; }

    // Convert wind bearing to wind angle
    // If the wind changes from 1 degree to 359 degress or vice-versa we want to interpolate the shortest distance around the circle: 1-->0-->359 or 359-->0-->1
    // So we add 360 to either the target or the current value: 361-->360-->359 or 359-->360-->361
    AngTarget[i] = WindBearing[i];
    if ((AngTarget[i] - AngCurrent[i]) > 180) { AngCurrent[i] += 360; }
    else if ((AngCurrent[i] - AngTarget[i]) > 180) { AngTarget[i] += 360; }

    // On the very first pass of the loop there is no starting point so we need to initialize the current values
    if (FirstPass == true) {
      HueCurrent[i] = HueTarget[i];
      BriCurrent[i] = BriTarget[i];
      TwiCurrent[i] = TwiTarget[i];
      SatCurrent[i] = SatTarget[i];
      AngCurrent[i] = AngTarget[i];
    }
    
    // *** modified Bresenham's line algorithm parameters ***
    // Determine the Deltas for the modified Bresenham's line algorithm
    HueDelta[i] = HueTarget[i]-HueCurrent[i];
    BriDelta[i] = BriTarget[i]-BriCurrent[i];
    TwiDelta[i] = TwiTarget[i]-TwiCurrent[i];
    SatDelta[i] = SatTarget[i]-SatCurrent[i];
    AngDelta[i] = AngTarget[i]-AngCurrent[i];

    // Determine the Signs for the modified Bresenham's line algorithm
    if (HueDelta[i] < 0) { HueSign[i] = -1; HueDelta[i] = -HueDelta[i]; } else { HueSign[i] = 1; }
    if (BriDelta[i] < 0) { BriSign[i] = -1; BriDelta[i] = -BriDelta[i]; } else { BriSign[i] = 1; }
    if (TwiDelta[i] < 0) { TwiSign[i] = -1; TwiDelta[i] = -TwiDelta[i]; } else { TwiSign[i] = 1; }
    if (SatDelta[i] < 0) { SatSign[i] = -1; SatDelta[i] = -SatDelta[i]; } else { SatSign[i] = 1; }
    if (AngDelta[i] < 0) { AngSign[i] = -1; AngDelta[i] = -AngDelta[i]; } else { AngSign[i] = 1; }

    // Zero the errors for the modified Bresenham's line algorithm
    HueError[i] = 0;
    BriError[i] = 0;
    TwiError[i] = 0;
    SatError[i] = 0;
    AngError[i] = 0;
  }
  
  FirstPass = false;
}

void loop() {
  ConnectWiFi();      // Check WiFi connection
  GetWeather();       // get UNIX time and weather data
  SetTargets();       // Set targets and initialize parameters for fade
  FadeLEDs();         // animate the LEDs and yeild()
}

void setup() {
  #ifdef DEBUG
  Serial.begin(115200); delay(1000);
  Serial.println(F("Fade Settings"));
  Serial.print(F("FramePeriod:   ")); Serial.println(FramePeriod);
  Serial.print(F("TwinkleFrames: ")); Serial.println(TwinkleFrames);
  Serial.print(F("MaxTwinkProb:  ")); Serial.println(MaxTwinkProb);
  Serial.print(F("FramesPerStep: ")); Serial.println(FramesPerStep);
  Serial.println();
  #endif
  
  // Set WiFi to station mode and disconnect from an AP if it was previously connected
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  ConnectWiFi(); // Check WiFi connection
  strip.Begin();
  delay(1000);
}

void ConnectWiFi() {
  if (WiFi.status() != WL_CONNECTED) {
    WiFi.begin(WiFiSSID, WiFiPSK);
    while (WiFi.status() != WL_CONNECTED) {
      #ifdef DEBUG
      Serial.print(F("."));
      #endif
      delay(500);
    }
    #ifdef DEBUG
    Serial.println(F(" WiFi connected")); Serial.println();
    #endif
  }
}
