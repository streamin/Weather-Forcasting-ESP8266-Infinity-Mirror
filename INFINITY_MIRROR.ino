#include <ESP8266WiFi.h>
#include <ArduinoJson.h>
#include <NeoPixelBus.h>
#include "CONSTANTS.h"


/*  **********************************************************************************
 *  ********************************* Global Variables *******************************
 *  **********************************************************************************/

 
int16_t HueDelta[Panes]; // Hue of LEDs (temperature)
int16_t HueTarget[Panes];
int16_t HueCurrent[Panes];
int8_t  HueSign[Panes];
int16_t HueError[Panes];

int8_t  BriDelta[Panes]; // Low brightness of twinkling LEDs (rain)
int8_t  BriTarget[Panes];
int8_t  BriCurrent[Panes];
int8_t  BriSign[Panes];
int16_t BriError[Panes];

int8_t  TwiDelta[Panes];  // Probability an LED will Twinkle (rain)
int8_t  TwiTarget[Panes];
int8_t  TwiCurrent[Panes];
int8_t  TwiSign[Panes];
int16_t TwiError[Panes];

int8_t  SatDelta[Panes]; // Saturation of wind LED (wind speed)
int8_t  SatTarget[Panes];
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
float   RainDepth[Panes];
float   SnowDepth[Panes];
uint32_t UNIXTime; // int64_t will not compile. use uint32_t, will work till year 2106. int32_t will work till year 2038

int16_t TwinkleBuffer[TwinkleFrames*Panes];  // ring buffer. position in ring determines stage of twinkling
int16_t TwinkleIndex = 0;   // position in the twinkle buffer

// http response buffer
char respBuf[httpBuffSize]; // this is very big. crashes unless i declare it a global

bool FirstPass = true;
bool IsForecast = false;

// pixel colours
NeoPixelBus<NeoGrbFeature, Neo800KbpsMethod> strip(PixelCount, PixelPin);
//NeoPixelBus<NeoRgbFeature, Neo400KbpsMethod> strip(PixelCount, PixelPin);


/*  **********************************************************************************
 *  ********************************* Functions **************************************
 *  **********************************************************************************/


void GetCurrent() { // size of array is already global const "Panes"
  // https://github.com/esp8266/Arduino/blob/master/libraries/ESP8266WiFi/examples/WiFiClient/WiFiClient.ino
  // https://arduinojson.org/v5/example/parser/
  // https://gist.github.com/bbx10/149bba466b1e2cd887bf
  
  WiFiClient client;
  
  // Connect to host
  //Serial.print("connecting to "); Serial.println(API_HOST);
  if (!client.connect(API_HOST, 80)) {
    Serial.println("connection failed");
    return;
  }
  
  // Send current weather GET Request
  Serial.println("Sending GET request for current weather...");
  client.print(String("GET ") + URL_CURRENT + " HTTP/1.1\r\n" +
               "Host: " + API_HOST + "\r\n" +
               "User-Agent: ESP8266\r\n" +
               "Connection: close\r\n\r\n");
  
  // Timeout if no response
  uint32_t timeout = millis();
  while (!client.connected() && !client.available()) {
  //while (!client.connected() || !client.available()) {
  //while (client.available() == 0) {
    if (millis() - timeout > 10000) {
      Serial.println(">>> Client Timeout !");
      client.stop();
      return;
    }
  }
  
  int32_t respLen = 0;
  bool skip_headers = true;
  while (client.connected() || client.available()) {
    if (skip_headers) {
      String aLine = client.readStringUntil('\n');
      //Serial.println(aLine);
      // Blank line denotes end of headers
      if (aLine.length() <= 1) {
        skip_headers = false;
      }
    } else {
      int16_t bytesIn;
      bytesIn = client.read((uint8_t *)&respBuf[respLen], /*sizeof(respBuf)*/httpBuffSize - respLen);
      if (bytesIn > 0) {
        respLen += bytesIn;
        //Serial.print(F("Bytes in ")); Serial.println(bytesIn);
      } else if (bytesIn < 0) {
        Serial.print(F("Read error ")); Serial.println(bytesIn);
        return;
      }
    }
    delay(1);
  }
  client.stop();
  
  if (respLen >= /*sizeof(respBuf)*/ httpBuffSize) {
    Serial.println(F("Response buffer overflow"));
    return;
  }
  
  // Terminate the C string
  respBuf[respLen++] = '\0';
  //Serial.print(F("Length:   ")); Serial.println(respLen);
  Serial.print(F("Response: ")); Serial.println(respBuf);
  Serial.println();
  
  // Allocate JsonBuffer
  DynamicJsonBuffer jsonBuffer(jsonBuffSize);
  // https://arduinojson.org/v5/faq/what-are-the-differences-between-staticjsonbuffer-and-dynamicjsonbuffer/
  
  // Parse JSON object
  JsonObject& root = jsonBuffer.parseObject(respBuf);
  if (!root.success()) {
    Serial.println(F("Parsing failed!")); Serial.println();
    return;
  }
  
  // Extract values
  UNIXTime = root["dt"];
  for (int8_t i=0; i < Panes; i++) {
    if (WeatherInterval[i] < 0) {
      Temperature[i]  = root["main"]["temp"];
      WindSpeed[i]    = root["wind"]["speed"];
      WindBearing[i]  = root["wind"]["deg"];
      RainDepth[i]    = root["rain"]["3h"];  // if this doesn't exist it will be zero.
      SnowDepth[i]    = root["snow"]["3h"];  // if this doesn't exist it will be zero.
    }
  }
}

void GetForecast() {
  // https://github.com/esp8266/Arduino/blob/master/libraries/ESP8266WiFi/examples/WiFiClient/WiFiClient.ino
  // https://arduinojson.org/v5/example/parser/
  // https://gist.github.com/bbx10/149bba466b1e2cd887bf
  
  WiFiClient client;
  
  // Connect to host
  //Serial.print("connecting to "); Serial.println(API_HOST);
  if (!client.connect(API_HOST, 80)) {
    Serial.println("connection failed");
    return;
  }
  
  // Send current weather GET Request
  Serial.println("Sending GET request for forecast weather...");
  client.print(String("GET ") + URL_FORECAST + " HTTP/1.1\r\n" +
               "Host: " + API_HOST + "\r\n" +
               "User-Agent: ESP8266\r\n" +
               "Connection: close\r\n\r\n");
  
  // Timeout if no response
  uint32_t timeout = millis();
  while (!client.connected() && !client.available()) {
  //while (!client.connected() || !client.available()) {
  //while (client.available() == 0) {
    if (millis() - timeout > 10000) {
      Serial.println(">>> Client Timeout !");
      client.stop();
      return;
    }
  }
  
  int32_t respLen = 0;
  bool skip_headers = true;
  while (client.connected() || client.available()) {
    if (skip_headers) {
      String aLine = client.readStringUntil('\n');
      //Serial.println(aLine);
      // Blank line denotes end of headers
      if (aLine.length() <= 1) {
        skip_headers = false;
      }
    } else {
      int16_t bytesIn;
      bytesIn = client.read((uint8_t *)&respBuf[respLen], /*sizeof(respBuf)*/httpBuffSize - respLen);
      if (bytesIn > 0) {
        respLen += bytesIn;
        //Serial.print(F("Bytes in ")); Serial.println(bytesIn);
      } else if (bytesIn < 0) {
        Serial.print(F("Read error ")); Serial.println(bytesIn);
        return;
      }
    }
    delay(1);
  }
  client.stop();
  
  if (respLen >= /*sizeof(respBuf)*/ httpBuffSize) {
    Serial.println(F("Response buffer overflow"));
    return;
  }
  
  // Terminate the C string
  respBuf[respLen++] = '\0';
  //Serial.print(F("Length:   ")); Serial.println(respLen);
  Serial.print(F("Response: ")); Serial.println(respBuf);
  Serial.println();
  
  // Allocate JsonBuffer
  DynamicJsonBuffer jsonBuffer(jsonBuffSize);
  // https://arduinojson.org/v5/faq/what-are-the-differences-between-staticjsonbuffer-and-dynamicjsonbuffer/
  
  // Parse JSON object
  JsonObject& root = jsonBuffer.parseObject(respBuf);
  if (!root.success()) {
    Serial.println(F("Parsing failed!")); Serial.println();
    return;
  }
  
  // Extract values
  for (int8_t i=0; i < Panes; i++) {
    if (WeatherInterval[i] >= 0) {
      Temperature[i]  = root["list"][WeatherInterval[i]]["main"]["temp"];
      WindSpeed[i]    = root["list"][WeatherInterval[i]]["wind"]["speed"];
      WindBearing[i]  = root["list"][WeatherInterval[i]]["wind"]["deg"];
      RainDepth[i]    = root["list"][WeatherInterval[i]]["rain"]["3h"];  // if this doesn't exist it will be zero.
      SnowDepth[i]    = root["list"][WeatherInterval[i]]["snow"]["3h"];  // if this doesn't exist it will be zero.
    }
  }
}

void PrintAPIData() {
  Serial.println(F("API Data"));
  Serial.print(F("     UNIX Time: ")); Serial.println(UNIXTime);
  for (int8_t i=0; i < Panes; i++) {
    Serial.print(F(" Temperature ")); Serial.print(i); Serial.print(F(": ")); Serial.print(Temperature[i]); Serial.println(F("°C"));
    Serial.print(F("  Wind Speed ")); Serial.print(i); Serial.print(F(": ")); Serial.print(WindSpeed[i]);   Serial.println(F("m/s"));
    Serial.print(F("Wind Bearing ")); Serial.print(i); Serial.print(F(": ")); Serial.print(WindBearing[i]); Serial.println(F("°"));
    Serial.print(F("  Rain Depth ")); Serial.print(i); Serial.print(F(": ")); Serial.print(RainDepth[i]);   Serial.println(F("mm"));
    Serial.print(F("  Snow Depth ")); Serial.print(i); Serial.print(F(": ")); Serial.print(SnowDepth[i]);   Serial.println(F("mm"));
  }
  Serial.println();
}

void PrintFadeData(int16_t j) {
  Serial.print(F("Step ")); Serial.print(j); Serial.println(F(" Fade Data"));
  for (int8_t i=0; i < Panes; i++) {
    Serial.print(F("       Hue ")); Serial.print(i); Serial.print(F(": ")); Serial.println(HueCurrent[i]);
    Serial.print(F("Brightness ")); Serial.print(i); Serial.print(F(": ")); Serial.print(BriCurrent[i]); Serial.println(F("%"));
    Serial.print(F("   Twinkle ")); Serial.print(i); Serial.print(F(": ")); Serial.print(TwiCurrent[i]); Serial.println(F("%"));
    Serial.print(F("Saturation ")); Serial.print(i); Serial.print(F(": ")); Serial.print(SatCurrent[i]); Serial.println(F("%"));
    Serial.print(F("Wind Angle ")); Serial.print(i); Serial.print(F(": ")); Serial.print(AngCurrent[i]); Serial.println(F("°"));
    Serial.print(F("  Wind LED ")); Serial.print(i); Serial.print(F(": ")); Serial.println(WindLED[i]);
  }
  Serial.println();
}

void FadeLEDs() {
  // For each step in the fade adjust the values of each pane
  for (int16_t j=0; j < FadeSteps; j++) {
    
    // FadeLEDs is a long blocking loop so call yield() within it
    // https://stackoverflow.com/questions/34497758/what-is-the-secret-of-the-arduino-yieldfunction
    yield();
      
    for (uint8_t i=0; i < Panes; i++) {
      // Previously this was done with interpolation but that was a whole bunch of math and it was slooooow. The modified Bresenham's line algorithm is much faster.
      
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

    PrintFadeData(j);
    
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
  //Serial.print(F("beep"));
  strip.Show();
  //Serial.println(F(" beep"));
  delay(FramePeriod);
}

void SetTargets() {
  // *** Set seasonal weather limits ***
  int16_t Day = (UNIXTime % SecInYear)/SecInDay; // This doesn't need to be super accurate. Leap years are accounted for in SecInYear.
  
  int16_t k = 1;
  while (NormalDay[k] < Day) {
    k++;
  }
  
  // Interpolate between (k-1) and k
  float Weight  = ((NormalDay[k]-Day)*1.0f)/(NormalDay[k]-NormalDay[k-1]);
  float MaxTemp =  NormalHigh[k]*(1-Weight) + NormalHigh[k-1]*Weight + OffsetTemp;
  float MinTemp =  NormalLow[k] *(1-Weight) + NormalLow[k-1] *Weight - OffsetTemp;
  float MaxWind =  NormalWind[k]*(1-Weight) + NormalWind[k-1]*Weight;
  float MaxRain = (NormalRain[k]*(1-Weight) + NormalRain[k-1]*Weight)*RainMulti;
  
  Serial.println(F("Relative Weather"));
  Serial.print(F("      Day: ")); Serial.println(Day);
  //Serial.print(F("Day Index: ")); Serial.println(k);
  //Serial.print(F("   Weight: ")); Serial.println(Weight);
  //Serial.print(F(" 1-Weight: ")); Serial.println((1-Weight));
  Serial.print(F(" Max Temp: ")); Serial.print(MaxTemp); Serial.println(F("°C"));
  Serial.print(F(" Min Temp: ")); Serial.print(MinTemp); Serial.println(F("°C"));
  Serial.print(F(" Max Wind: ")); Serial.print(MaxWind); Serial.println(F("m/s"));
  Serial.print(F(" Max Rain: ")); Serial.print(MaxRain); Serial.println(F("mm"));
  Serial.println();
  
  for (int8_t i=0; i < Panes; i++) {
    // Save the previous targets as the new current values
    HueCurrent[i] = HueTarget[i];
    BriCurrent[i] = BriTarget[i];
    TwiCurrent[i] = TwiTarget[i];
    SatCurrent[i] = SatTarget[i];
    AngCurrent[i] = AngTarget[i];
    
    // *** Convert temperature to hue ***
    // MaxTemp --> MinHue, MinTemp --> MaxHue
    HueTarget[i] = MaxHue - ((Temperature[i]-MinTemp)*(MaxHue-MinHue))/(MaxTemp-MinTemp);
    if (HueTarget[i] < MinHue) { HueTarget[i] = MinHue; }
    else if (HueTarget[i] > MaxHue) { HueTarget[i] = MaxHue; }

    // *** Convert rain and snow to brightness and twinkle probability ***
    // convert snow depth to rain depth
    if (SnowDepth[i] > 0) { // If the json value doesn't exist will this be null, zero, or the previous value? will have to test
      int8_t j = 1;  // start at 1 in case SnowTemp is out of range.
      while (SnowTemp[j] > Temperature[i]) {
        j++;
      }
      // Interpolate and add normalized snow depth to rain depth
      RainDepth[i] = RainDepth[i] + ((Temperature[i]-SnowTemp[j])*(Snow2Rain[j-1]-Snow2Rain[j])/(SnowTemp[j-1]-SnowTemp[j])+Snow2Rain[j])*SnowDepth[i];
    }
    
    // No Rain --> MaxBri, MaxRain --> MinBri
    BriTarget[i] = (RainDepth[i]*(MinBri-MaxBri))/MaxRain + MaxBri;
    if (BriTarget[i] > MaxBri) { BriTarget[i] = MaxBri; }
    else if (BriTarget[i] < MinBri) { BriTarget[i] = MinBri; }

    // No Rain --> No Twinkle, MaxRain --> MaxTwinkProb
    TwiTarget[i] = RainDepth[i]*MaxTwinkProb/MaxRain;
    if (TwiTarget[i] > MaxTwinkProb) { TwiTarget[i] = MaxTwinkProb; }

    // *** Convert wind speed to saturation and wind bearing to wind angle***
    // 0kph Wind --> MaxSat, MaxWind --> MinSat
    SatTarget[i] = (WindSpeed[i]*(MinSat-MaxSat))/MaxWind + MaxSat;
    if (SatTarget[i] > MaxSat) { SatTarget[i] = MaxSat; }
    else if (SatTarget[i] < MinSat) { SatTarget[i] = MinSat; }
    
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

void setup() {
  for (int8_t i=0; i < Panes; i++) {
    if (WeatherInterval[i] >= 0) {
      IsForecast = true;
    }
  }
  Serial.begin(115200); delay(1000);
  ConnectWiFi(); // Check WiFi connection
  strip.Begin();
  delay(1000);
}

void loop() {
  ConnectWiFi();      // Check WiFi connection
  GetCurrent();       // get UNIX time and current weather data
  if (IsForecast == true) {
    GetForecast();  // get forecast weather data
  }
  PrintAPIData();
  SetTargets();       // Set targets and initialize parameters for fade
  FadeLEDs();         // Smooth transition of the LEDs
}

void ConnectWiFi() {
  if (WiFi.status() != WL_CONNECTED) {
    WiFi.begin(WiFiSSID, WiFiPSK);
    while (WiFi.status() != WL_CONNECTED) {
      //Serial.print(F("."));
      delay(500);
    }
    //Serial.println(F(" Connected!"));
    //Serial.println();
  }
}
