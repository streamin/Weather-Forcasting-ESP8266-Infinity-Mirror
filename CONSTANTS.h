// ***** WiFi Config *****
const char* WiFiSSID  = "your_ssid";
const char* WiFiPSK   = "your_password";

// ***** API Config *****
const char* APIHost = "api.darksky.net";
const char* APIURL  = "/forecast/your_api_key/latitude,longitude?exclude=minutely,daily,alerts,flags&units=si";
const int jsonSize  = 750;

// ***** Timing and Fade Config Options *****
#define       FramesPerSecond 30
const int16_t FramePeriod   = 1000/FramesPerSecond; // [ms] Refresh period of LEDs
#define       RainDropLength  750 // [ms]
const int8_t  TwinkleFrames = RainDropLength/FramePeriod; // frames for LED to resume full brightness after twinkling
#define       MaxDropsPerSec  15 // Rain drops per second at MaxRain
const int8_t  MaxTwinkProb  = MaxDropsPerSec*FramePeriod/10; // [%]  Probability an LED will twinkle at MaxRain // keep less than or equal to 100
const int16_t FadeSteps     = 10;  //
#define       APIRefreshTime  600000 // [ms] // 600,000ms = 10min between GET requests
const int16_t FramesPerStep = APIRefreshTime/(FramePeriod*FadeSteps); // Frames per fade step

// ***** Colour Settings *****
// http://colorizer.org/
// Red: 0, yellow: 60, green: 120, cyan: 180, blue: 240, magenta: 300, red: 360
const int16_t MinHue  = 0;    // colour of MaxTemp
const int16_t MaxHue  = 240;  // colour of MinTemp

const int8_t  MaxSat  = 100;  // 0 to 100, keep at 100, 0 is white, 100 is full colour
const int8_t  MinSat  = 0;    // 0 to 100, keep at 0, saturation at MaxWind

const int8_t   MaxBri  = 25;          // ~10 to ~40, tune this number to your liking
const int8_t HiMinBri  = MaxBri*0.6;  // birghtness of twinkling LED at No Rain // keep it less than MaxBri
const int8_t LoMinBri  = 0;           // birghtness of twinkling LED at MaxRain

// ***** Weather Settings *****
// We want to use relative temperature otherwise the LED colour will only change with the seasons. All values will be relative to the average value for the day of the year.
// Day 0 and 366 are Dec 31. Days are the 15th and last day of every month in a non leap year.
// http://climate.weather.gc.ca/climate_data/almanac_e.html?StationID=3698&period=30&searchMethod=contains&txtStationName=winnipeg&month=12&day=31
const int16_t NormalDay[]   = {    0,    15,    31,    46,    59,    74,   90,  105,  120,  135,  151,  166,  181,  196,  212,  227,  243,  258,  273,  288,  304,  319,   334,   349,   366};
const float   NormalLow[]   = {-22.2, -23.2, -21.7, -18.7, -15.5, -11.3, -6.7, -2.4,  1.3,  4.6,  8.0, 10.7, 12.5, 13.4, 13.4, 12.3,  9.4,  6.0,  3.2,  0.1, -4.4, -9.4, -14.6, -19.1, -22.2}; // [°C]
const float   NormalHigh[]  = {-12.4, -13.2, -11.5,  -8.4,  -5.7,  -1.7,  4.3, 10.2, 15.3, 19.2, 21.8, 23.3, 24.7, 25.8, 26.3, 25.5, 22.4, 18.6, 15.3, 11.4,  5.3, -1.0,  -5.9,  -9.7, -12.4}; // [°C]
const float   MaxWind       = 8.33; // [m/s] // https://winnipeg.weatherstats.ca/charts/wind_speed-monthly.html
const int8_t  OffsetTemp    = 4;    // [°C] degrees to offset normal seasonal highs and lows
const int8_t  WeatherInterval[] = {24, 12, 0};  // Forecast interval shown on each pane. 0 is current weather. Repeats allowed, any order allowed. Intervals are 1 hour each.

// ***** Hardware Config Settings *****
const int8_t  PixPerPane    = 40;  // pixels per pane
const int8_t  Panes         = sizeof(WeatherInterval);
const int16_t PixelCount    = Panes*PixPerPane;
const int8_t  PixelPin      = 2; // I used the Adafruit HUZZAH. I had to remove the diode protecting the 3V3 RX pin from 5V signals. The RX pin needs to be used as an output because it is the only DMA output on an ESP8266. Labeled Pin 2 on the HUZZAH.

/**************** My LED arrangment *********************
                                                             N (0°)                                         N (0°)
                                                             ^                 44°                          ^                 44°
                        ... 90                59 58 57 56 55 | 54 53 52 51 50 /              19 18 17 16 15 | 14 13 12 11 10 /
                               89          60                |   |     |      49 _ 51°    20                |   |     |       9 _ 51°
                               88          61                |  10°   29°     48          21                |  10°   29°      8
           Pane 2              87          62   Pane 1       |                47 _ 68°    22   Pane 0       |                 7 _ 68°
           LED Index           86          63   LED Index    |                46          23   LED Index    |                 6
                               85          64                |                45          24                |                 5
W (270°) <---------------------------------------------------+----------------------------------------------+-------------------------> E (90°)
                               84          65                |                44          25                |                 4
                               83          66                |                43          26                |                 3
                               82          67                |                42          27                |                 2
                               81          68                |                41          28                |                 1
                               80          69                |                40          29                |                 0
                       ... 119                70 71 72 73 74 | 75 76 77 78 79                30 31 32 33 34 | 35 36 37 38 39
                                                             v                                              v
                                                             S (180°)                                       S (180°)
*/

const int16_t WindDirArray[] = {10, 20, 29, 37, 44, 51, 59, 68, 78, 89, 100, 111, 121, 130, 138, 145, 152, 160, 169, 179, 190, 200, 209, 217, 224, 231, 239, 248, 258, 269, 280, 290, 299, 307, 314, 321, 329, 338, 348, 359};
const int8_t  WindLEDArray[] = {14, 13, 12, 11, 10,  9,  8,  7,  6,  5,   4,   3,   2,   1,   0,  39,  38,  37,  36,  35,  34,  33,  32,  31,  30,  29,  28,  27,  26,  25,  24,  23,  22,  21,  20,  19,  18,  17,  16,  15};
// pane 0 example: 0° to 10° is LED 14, or , 45° to 51° is LED 9
// pane 1 example: 11° to 20° is 13+(1*PixPerPane) = LED 53
