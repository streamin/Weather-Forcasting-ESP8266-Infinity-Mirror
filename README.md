# Weather Forcasting ESP8266 Infinity Mirror
A WiFi connected Infinity Mirror that displays current and forecasted weather information with NeoPixels. Writen with the Arduino IDE.

The infinity mirror has an arbitrary number of panes.
Each pane has an equal but arbitrary number of NeoPixels.
Each pane represents the current weather or the weather for a specified forecast period.
Weather data is from the Dark Sky API (https://darksky.net/dev/docs).

Temperature is represented by the hue of the NeoPixels.
Wind Angle is represented by a single NeoPixel whose position on the perimeter of the pane coresponds to the Wind Angle.
Wind Speed is represented by the saturation of the Wind Angle NeoPixel.
Probability of Percipitation is represented by a twinkling effect.

As the weather forecast is updated there are smooth transitions in the appearence of the NeoPixels. Temperature is relative to the seasonal averages so the hue changes frequently. Without this feature the NeoPixel hues would only change with the season.

Uses ArduionJson (https://github.com/bblanchon/ArduinoJson) and NeoPixelBus (https://github.com/Makuna/NeoPixelBus).
