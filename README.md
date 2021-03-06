# Arduino FrSky Telemetry Display

[![Build Status](https://travis-ci.org/xythobuz/FrSky-Telemetry-Arduino.svg?branch=master)](https://travis-ci.org/xythobuz/FrSky-Telemetry-Arduino)

I've modified my RC transmitter by adding an [Arduino Pro Mini](https://www.arduino.cc/en/Main/ArduinoBoardProMini) with [this HobbyKing 0.96" OLED](http://www.hobbyking.com/hobbyking/store/__46373__Multiwii_OLED_Display_Module_I2C_128x64_Dot_MWC_.html).

[![Photo 1](http://i.imgur.com/Uuro1FN.png)](http://i.imgur.com/U6wNsI4.jpg)
[![Photo 2](http://i.imgur.com/MOI2jv5.png)](http://i.imgur.com/cExsBPu.jpg)
[![Photo 3](http://i.imgur.com/ve6bCwT.png)](http://i.imgur.com/C18T33u.jpg)

Unfortunately, this display isn't really standard-conformant, so I had to resort to using the only library I've found that get's it to work: [this one from the HobbyKing comments](http://www.hobbyking.com/hobbyking/store/uploads/121771368X1172162X54.zip).

The FrSky protocol parsing code has been included from my previous [Saitek-X52 USB to PPM converter project](https://github.com/xythobuz/Saitek-X52-PPM).

The Battery voltage calculated and displayed is, of course, only applicable to my specific model. You can easily change the formulas used.

You can connect a simple Piezo Buzzer to Pin 4 and an active-high LED to Pin 3, they will also sound or light-up with voltage alarms. Connect a pushbutton to each of Pin 8 and 9 to control the built-in menu and change configuration options on-the-fly.
This also allows selecting between multiple models, each with its own warning/alarm voltages and conversion factors.

[![Fritzing Schematic](https://i.imgur.com/pa8UJW8.png)](https://i.imgur.com/4W3O9zr.png)

## Replacing the Boot Logo

You can also easily replace the included boot logo. It can be converted, from a PNG image to the required C-Header file format, by using the included `icon/convert.py` utility. It depends on [pypng](https://pypi.python.org/pypi/pypng). Simply call the `icon/update.sh` script to automatically replace `logo.h` with the contents of `icon/icon.png`.

![Included Logo](https://raw.githubusercontent.com/xythobuz/FrSky-Telemetry-Arduino/master/icon/icon.png)

## Licensing

    ----------------------------------------------------------------------------
    "THE BEER-WARE LICENSE" (Revision 42):
    <xythobuz@xythobuz.de> wrote this file.  As long as you retain this notice
    you can do whatever you want with this stuff. If we meet some day, and you
    think this stuff is worth it, you can buy me a beer in return.   Thomas Buck
    ----------------------------------------------------------------------------

