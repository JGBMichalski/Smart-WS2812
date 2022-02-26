<h1 align="center">Smart WS2812</h1>
<p align="center">
  <a href="https://github.com/JGBMichalski/Kijiji-Scraper"><img alt="Github Hits" src="https://hits.seeyoufarm.com/api/count/incr/badge.svg?url=https%3A%2F%2Fgithub.com%2FJGBMichalski%2FSmart-WS2812&count_bg=%2379C83D&title_bg=%23555555&icon=github.svg&icon_color=%23E7E7E7&title=Hits&edge_flat=false" height="20"/></a>
</p>
<p align="center">
  <b>
  An Arduino program that uses <i>Sinric Pro</i> to control <i>WS2812</i> individually addressable LEDs through Amazon Alexa & Google Home.
  </b>
  
<br />

---

## Dependencies

### Boards

This project requires the following `Arduino Boards` repository: 

* [`ESP8266 v3.0.1`](https://github.com/esp8266/Arduino)

### Libraries

This project requires the following `Arduino Libraries`: 

* `SinricPro v2.9.16` by Boris Jaeger
* `Adafruit NeoPixel v1.10.4`

---

## Setup

Make sure to fill in the following sections in the code:
- `WIFI_SSID`
- `WIFI_PASS`
- `APP_KEY` - Should look like `de0bxxxx-1x3x-4x3x-ax2x-5dabxxxxxxxx`
- `APP_SECRET` - Should look like `5f36xxxx-x3x7-4x3x-xexe-e86724a9xxxx-4c4axxxx-3x3x-x5xe-x9x3-333d65xxxxxx`
- `LIGHT_ID` - Should look like `5dc1564130xxxxxxxxxxxxxx`
- `PRESET_ID` - Should look like `5dc1564130xxxxxxxxxxxxxx`. Do not worry about this if you are only using the Single Smart Light setup.

You can choose to setup Sinric Pro in one of two ways:
1. Single *Smart Light* setup in Sinric Pro.
2. Double *Smart Light* setup in Sinric Pro - for additional presets.

### Single Smart Light Setup
The single smart light setup allows you to control the power, brightness, colour, and 5 presets of the strip. This is done by setting up a new *Smart Light* in Sinric Pro. 

The 5 presets can be accessed by using the *colour temperature* feature of the smart light. 

To use this mode and **NOT** the double smart light setup, you must set `USE_PRESET_DEVICE` to `false`. Otherwise, you might run into issues when the program attempts to connect to the second smart light.

### Double Smart Light Setup
The double smart light setup uses the same smart light that the previous section discussed, but it also adds another one to control additional presets through the brightness control. This allows you to have access to up to 100 more presets. 

To use this mode, you must:
1. Set `USE_PRESET_DEVICE` to `true`.
2. Set the `PRESET_ID` to the preset device ID.