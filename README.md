# Weather-Forcasting-ESP8266-Infinity-Mirror
An WiFi connected Infinity Mirror that displays current and forecasted weather information with NeoPixels.

An infinity mirror with an arbitrary number of panes.
Each pane has an equal but arbitrary number of NeoPixels.
Each pane represents the current weather or the weather for a specified forecast period.

Weather data is from the openweathermap.org API (darksky.net is https only and wunderground.com requires too much ram).
Temperature is represented by the hue of the NeoPixels.
Wind Angle is represented by a single NeoPixel whose position on the perimeter of the pane coresponds to the Wind Angle.
Wind Speed is represented by the saturation of the Wind Angle NeoPixel.
Percipitation is represented by a twinkling effect (changing brightness of the NeoPixels).

As the weather forcast is updated there are smooth transitions in the appearence of the NeoPixels. All weather data is relative to the seasonal averages to create patterns that change hourly. Without this feature the patterns would only change with the season.
