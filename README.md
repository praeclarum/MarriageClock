# Marriage Clock

Ever wonder how long you've been married?
Or maybe it's a day before the wedding, and you *still* haven't found a gift.

Look no more, the Marriage Clock is here for you.

This thing is a heart with a clock on it.

It also always says "H+J" in memory of Heather and James' marriage.

## CAD

I designed the heart clock using [OpenJSCAD.org](). It's great, 

Copy the contents of [cad.js]() to 

## That's a lot of code for just a clock

Yes it is! The code does these things:

1. Gets the current time
2. Stores that in the real time clock so it doesn't forget
3. Figures out how long it's been since a certain date and time (say, a wedding)
4. Displays that on it's display
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
*MyHTTPClient.h* and *MyHTTPClient.cpp*.

### Store the time in the Real Time Clock

The RTC is a little device that will keep track of the time
so long as it has power and the ESP32 doesn't get reset.

It even keeps counting while the ESP32 is asleep.



