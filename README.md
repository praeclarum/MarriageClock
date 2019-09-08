# Marriage Clock

<img src="https://github.com/praeclarum/MarriageClock/raw/master/Images/clock.jpg" width="256" /> <img src="https://github.com/praeclarum/MarriageClock/raw/master/Images/insides.jpg" width="256" />

Ever wonder how long you've been married?
Or maybe it's a day before the wedding, and you *still* haven't found a gift.

Look no more, the Marriage Clock is here for you.

This thing is a heart-shaped clock that will tell you how many years, months, and days you've been married.

It also always says "H+J" in honor of Heather and James' marriage.

## Bill of Materials

1. $19.95 **[Adafruit HUZZAH32](https://www.adafruit.com/product/3405)** I love ESP32s and the Adafruit dev kit one of my favorites because it includes a USB connector and a battery management unit.
2. $5.495 **[OLED Screen](https://www.amazon.com/gp/product/B0761LV1SD)** Small OLED modules are pretty cheap these days!
3. $9.95 **[Battery](https://www.adafruit.com/product/258)** I forgot which battery I used, but this one looks nice. The bigger the battery, the better the clock.
4. $∞ **[3D Printer](https://en.wikipedia.org/wiki/MakerBot#Replicator_2_Desktop_3D_Printer)** to print the heart

## CAD

<img src="https://github.com/praeclarum/MarriageClock/raw/master/Images/cad.jpg" width="256" />

I designed the heart clock using [OpenJSCAD.org](https://openjscad.org).
It's great, you write code to make 3D objects.

Copy the contents of [cad.js](https://github.com/praeclarum/MarriageClock/blob/master/cad.js) to OpenJSCAD
to see the full box.

To print, change `main()` to only be `printBottom()` or `printTop()` to export print STL files.

## That's a lot of code for just a clock

Yes it is! The main code ([MarriageClock.ino](https://github.com/praeclarum/MarriageClock/blob/master/MarriagleClock.ino)) does these things:

1. Gets the current time
2. Stores that in the real time clock so it doesn't forget
3. Figures out how long it's been since a certain date and time (say, a wedding)
4. Displays that on its display
5. Go to sleep for 10 minutes
6. Go to #3

### Getting the current time

*Technically* the current time is in the ether and with the right
antenna you can pull it out of thin air.

But I'm not that good of an engineer. Instead, this puppy connects
to every open WiFi network it can find, starts making HTTP requests,
and then hopes that server will be kind enough to tell it the time.

Sounds easy right? It is, but I had to hack the ESP32 HTTPClient
to return the current date from the server. That's why you see
[MyHTTPClient.cpp](https://github.com/praeclarum/MarriageClock/blob/master/MyHTTPClient.cpp).

### Store the time in the Real Time Clock

The RTC is a little device that will keep track of the time
so long as it has power and the ESP32 doesn't get reset.

It even keeps counting while the ESP32 is asleep.

### Diffing date times

I want to display the number of years and months since a certain date.

C does not include a time diff function that can do this, so I ended up including PHP's timelib. Go PHP!

### Display the time

The code in [Display.cpp](https://github.com/praeclarum/MarriageClock/blob/master/Display.cpp) draws the time on the OLED screen
using Adafruit's graphics libraries.

Infinite fun can be had here.

## Contributing

If you have an enhancement, please don't hesitate to send a PR!




