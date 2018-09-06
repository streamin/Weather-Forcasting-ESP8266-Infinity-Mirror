// ***** WiFi Config *****
const char* WiFiSSID  = "your_ssid";
const char* WiFiPSK   = "your_password";

// ***** API Config *****
#define API_KEY       "your_api_key"
#define API_CITY_ID   "6183235" // 6183235 is Winnipeg. Find a city: https://openweathermap.org/find?q=
#define API_HOST      "api.openweathermap.org"
#define URL_FORECAST  "/data/2.5/forecast?id=" + API_CITY_ID + "&appid=" + API_KEY + "&units=metric"
#define URL_CURRENT   "/data/2.5/weather?id="  + API_CITY_ID + "&appid=" + API_KEY + "&units=metric"

// ***** Software Config Options *****
const int16_t FramePeriod   = 80; // [ms] Refresh period of LEDs
const int8_t  TwinkleFrames = 8;  // frames for LED to resume full brightness after twinkling 
const int8_t  MaxTwinkProb  = 90; // [%]  Probability one LED will twinkle at MaxRain
const int16_t FadeSteps     = 100;  //
const int16_t FramesPerStep = 450;  // FramePeriod*FadeSteps*FramesPerStep ~= 3,600,000ms = 60min between GET requests
const int8_t  OffsetTemp    = 2;    // [°C] degrees to offset normal seasonal highs and lows
const float   RainMulti     = 1.5;  // Rain multiplier. Rainfall numbers are average daily amount. Forecast interval is 3h. Assume all daily rain falls during only 2h of the day: RainMulti = forecast_interval/rain_duration = 3/2

const int32_t httpBuffSize  = (18*1024); // potential number of chars in GET response
const int32_t jsonBuffSize  = (21*1024); // 21*1024 seems to work

const int8_t  WeatherInterval[] = {-1, 3, 7}; // negatives values are current weather. repeats allowed, out of order values allowed. Note: intervals are 3 hours each, 0 is 3h in future

const int32_t SecInYear     = 31556952; // 365.2425*24*60*60 = 31556952
const int32_t SecInDay      = 86400;    // 24*60*60 = 86400

// ***** Colour Settings *****
// http://colorizer.org/
// Red: 0, yellow: 60, green: 120, cyan: 180, blue: 240, magenta: 300, red: 360
const int16_t MinHue  = 0;    // colour of MaxTemp
const int16_t MaxHue  = 240;  // colour of MinTemp

const int8_t  MaxSat  = 100;  // 0 to 100, keep at 100, 0 is white, 100 is full colour
const int8_t  MinSat  = 0;    // 0 to 100, saturation at MaxWind

const int8_t  MaxBri  = 25;   // 0 to 100, tune this number to your liking
const int8_t  MinBri  = 0;    // birghtness of twinkling LED at MaxRain

// ***** Normal Weather Config *****
// We want to use relative weather values otherwise the LEDs will only change with the seasons. All values will be relative to the average value for the day of the year.
// Day 0 and 366 are Dec 31. Days are the 15th and last day of every month in a non leap year.
// http://climate.weather.gc.ca/climate_data/almanac_e.html?StationID=3698&period=30&searchMethod=contains&txtStationName=winnipeg&month=12&day=31
// https://en.wikipedia.org/wiki/Winnipeg#Climate  AVG_MONTHLY_PRECIP_MM/AVG_MONTHLY_PRECIP_DAYS, interpolated to get end of month values
// https://winnipeg.weatherstats.ca/charts/wind_speed-monthly.html
const int16_t NormalDay[]   = {    0,    15,    31,    46,    59,    74,   90,  105,  120,  135,  151,  166,  181,  196,  212,  227,  243,  258,  273,  288,  304,  319,   334,   349,   366};
const float   NormalLow[]   = {-22.2, -23.2, -21.7, -18.7, -15.5, -11.3, -6.7, -2.4,  1.3,  4.6,  8.0, 10.7, 12.5, 13.4, 13.4, 12.3,  9.4,  6.0,  3.2,  0.1, -4.4, -9.4, -14.6, -19.1, -22.2}; // [°C]
const float  NormalHigh[]   = {-12.4, -13.2, -11.5,  -8.4,  -5.7,  -1.7,  4.3, 10.2, 15.3, 19.2, 21.8, 23.3, 24.7, 25.8, 26.3, 25.5, 22.4, 18.6, 15.3, 11.4,  5.3, -1.0,  -5.9,  -9.7, -12.4}; // [°C]
const float  NormalRain[]   = { 1.73,  1.63,  1.67,  1.73,  2.23,  2.66, 3.32, 4.17, 4.64, 4.93, 5.92, 6.77, 6.86, 6.97, 7.08, 7.20, 5.82, 4.40, 4.21, 3.99, 3.17, 2.43,  2.10,  1.82,  1.73}; // [mm]
const float  NormalWind[]  = {  13.9,  13.9,  13.9,  13.9,  13.9,  13.9, 13.9, 13.9, 13.9, 13.9, 13.9, 13.9, 13.9, 13.9, 13.9, 13.9, 13.9, 13.9, 13.9, 13.9, 13.9, 13.9,  13.9,  13.9,  13.9}; // [m/s]

// We must convert snow depth to rain depth. Snow depth is not proportional to rain depth so we must interpolate based on temperature.
// https://spacemath.gsfc.nasa.gov/earth/88Mod11Prob1.pdf
const int8_t SnowTemp[] = {  5,    0,     -4,    -10,   -15,  -25,  -100};
const float Snow2Rain[] = {0.2, 0.12, 0.0667, 0.0333, 0.025, 0.02, 0.02}; // Snow to rain ratio at various temperatures

// ***** Hardware Config Options *****
const int8_t  PixPerPane    = 40;  // pixels per pane
const int8_t  Panes         = sizeof(WeatherInterval);
const int16_t PixelCount    = Panes*PixPerPane;
const int8_t  PixelPin      = 2; // I used the Adafruit Huzzah. I had to remove a diode because it blocked the DMA output on pin 2 (the only DMA output on a ESP8266)

/*
pane 0 example: 0° to 10° is LED 15, or , 45° to 51° is LED 20
pane 1 example: 11° to 20° is 16+(1*PixPerPane) = LED 56

                             N (0°)                                         N (0°)
                             ^                 44°                          ^                 44°
              10 11 12 13 14 | 15 16 17 18 19 /              50 51 52 53 54 | 55 56 57 58 59 /
            9                |   |     |      20 _ 51°    49                |   |     |      60 _ 51°
            8                |  10°   29°     21          48                |  10°   29°     61
            7   Pane 0       |                22 _ 68°    47   Pane 1       |                62 _ 68°         Pane 2...Pane N
            6   LED Index    |                23          46   LED Index    |                63
            5                |                24          45                |                64
W (270°) <-------------------+---------------------------------------------------------------------------------------------------> E (90°)
            4                |                25          44                |                65
            3                |                26          43                |                66
            2                |                27          42                |                67
            1                |                28          41                |                68
            0                |                29          40                |                69
              39 38 37 36 35 | 34 33 32 31 30                79 78 77 76 75 | 74 73 72 71 70
                             v                                              v
                             S (180°)                                       S (180°)
*/
const int16_t WindDirArray[] = {10, 20, 29, 37, 44, 51, 59, 68, 78, 89, 100, 111, 121, 130, 138, 145, 152, 160, 169, 179, 190, 200, 209, 217, 224, 231, 239, 248, 258, 269, 280, 290, 299, 307, 314, 321, 329, 338, 348, 359};
const int8_t  WindLEDArray[] = {15, 16, 17, 18, 19, 20, 21, 22, 23, 24,  25,  26,  27,  28,  29,  30,  31,  32,  33,  34,  35,  36,  37,  38,  39,   0,   1,   2,   3,   4,   5,   6,   7,   8,   9,  10,  11,  12,  13,  14};
