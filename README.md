# Weather Forcasting ESP8266 Infinity Mirror
A WiFi connected Infinity Mirror that displays current and forecasted weather information with NeoPixels. Writen with the Arduino IDE.

The infinity mirror has an arbitrary number of panes.
Each pane has an equal but arbitrary number of NeoPixels.
Each pane represents the current weather or the weather for a specified forecast period.
Weather data is from the Open Weather Map API (https://openweathermap.org/api).

Temperature is represented by the hue of the NeoPixels.
Wind Angle is represented by a single NeoPixel whose position on the perimeter of the pane coresponds to the Wind Angle.
Wind Speed is represented by the saturation of the Wind Angle NeoPixel.
Percipitation is represented by a twinkling effect that is proportional to the percipitation volume.

As the weather forecast is updated there are smooth transitions in the appearence of the NeoPixels. All weather data is relative to the seasonal averages to create patterns that change hourly. Without this feature the NeoPixel patterns would only change with the season.

Uses ArduionJson (https://github.com/bblanchon/ArduinoJson) and NeoPixelBus (https://github.com/Makuna/NeoPixelBus).
