#include <Arduino.h>
#include <ESP8266WiFi.h> // Using ESP8266 v3.0.1
#include <StreamString.h>
#include "SinricPro.h" // Using SinricPro v2.9.16 by Boris Jaeger
#include "SinricProLight.h"
#include <Adafruit_NeoPixel.h> // Using Adafruit NeoPixel v1.10.4

/***************************************
 ************ User Settings ************
 ***************************************/

#define PIN 14 // D5 on the ESP8266
#define NUM_LEDS 90 // Total number of LEDs connected

// If you are wanting to use an additional device to allow for additional presets, set this to true.
// Note: This means you should have two devices setup in Sinric Pro (one for the regular light, and one for the presets)
#define USE_PRESET_DEVICE false 

// WiFi Information
#define WIFI_SSID   ""
#define WIFI_PASS   ""

// Light Information
#define APP_KEY     "" // Should look like "de0bxxxx-1x3x-4x3x-ax2x-5dabxxxxxxxx"
#define APP_SECRET  "" // Should look like "5f36xxxx-x3x7-4x3x-xexe-e86724a9xxxx-4c4axxxx-3x3x-x5xe-x9x3-333d65xxxxxx"
#define LIGHT_ID    "" // Should look like "5dc1564130xxxxxxxxxxxxxx"
#define PRESET_ID   "" // Should look like "5dc1564130xxxxxxxxxxxxxx". Do not worry about this if you are only using the Single Smart Light setup.


/**************************************
 ******** End of User Settings ********
 **************************************/

/* Adafruit_NeoPixel()
 * Parameter 1 = number of pixels in strip
 * Parameter 2 = pin number (most are valid)
 * Parameter 3 = pixel type flags, add together as needed:
 *   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
 *   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
 *   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
 *   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
 */ 
Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_LEDS, PIN, NEO_GRB + NEO_KHZ800);

WiFiClient client;
uint64_t heartbeatTimestamp = 0;
bool isConnected = false;
uint8_t state = 0;
uint32_t pattern = 4000;


/***************************************
 ********** Utility Functions **********
 ***************************************/

/**
 * CheckForNewCommand
 *
 * Checks to see if the power or color information has changed.
 * If so, then inform the function of the status change.
 *
 * @returns True if any information has been updated.
 */
bool CheckForNewCommand() {
  uint8_t currentState = state;
  uint32_t currentPattern = pattern;
  
  SinricPro.handle();

  if ((state != currentState) or (pattern != currentPattern))
    return true;
  else return false;
}

/**
 * Wheel
 *
 * Utility function for the RainbowCycle animation.
 */
byte * Wheel(byte wheelPos) {
  static byte c[3];

  if(wheelPos < 85) {
    c[0] = wheelPos * 3;
    c[1] = 255 - wheelPos * 3;
    c[2] = 0;
  } else if(wheelPos < 170) {
    wheelPos -= 85;
    c[0] = 255 - wheelPos * 3;
    c[1] = 0;
    c[2] = wheelPos * 3;
  } else {
    wheelPos -= 170;
    c[0] = 0;
    c[1] = wheelPos * 3;
    c[2] = 255 - wheelPos * 3;
  }

  return c;
}

/**
 * FadeToBlack
 *
 * Darkens the specified pixel by an amount based on the 
 * fadeValue specified.
 * 
 * @param pixel The pixel we want to fade to black.
 * @param fadeValue The amount in which you want the pixel to fade.
 */
void FadeToBlack(int pixel, byte fadeValue) {
  uint32_t oldColor = GetColour(pixel);
  uint8_t r = (oldColor & 0x00ff0000UL) >> 16;
  uint8_t g = (oldColor & 0x0000ff00UL) >> 8;
  uint8_t b = (oldColor & 0x000000ffUL);

  r = (r <= 10) ? 0 : (int) r - (r * fadeValue / 256);
  g = (g <= 10) ? 0 : (int) g - (g * fadeValue / 256);
  b = (b <= 10) ? 0 : (int) b - (b * fadeValue / 256);
  
  SetPixel(pixel, r, g, b);
}

/**************************************
 ****** Strip-Specific Functions ******
 **************************************/
/**
 * ShowStrip
 *
 * Displays the latest pixel configuration on the LED strip.
 */
void ShowStrip() {
  strip.show();
}

/**
 * SetPixel
 *
 * Sets the specified pixel to the RGB value.
 *
 * @param pixel The pixel you'd like to set.
 * @param red The red value you'd like the pixel to have.
 * @param green The green value you'd like the pixel to have.
 * @param blue The blue value you'd like the pixel to have.
 */
void SetPixel(int pixel, byte red, byte green, byte blue) {
  strip.setPixelColor(pixel, strip.Color(red, green, blue));
}

/**
 * SetBrightness
 *
 * Sets the specified pixel to the RGB value.
 *
 * @param brightness The desired brightness of the strip.
 */
void SetBrightness(uint8_t brightness) {
  strip.setBrightness(brightness);
}

/**
 * GetColour
 *
 * Returns the current colour of the selected pixel.
 *
 * @param pixel The pixel you'd like to get the colour of.
 * 
 * @returns Colour of the pixel.
 */
uint32_t GetColour(int pixel) {
  return strip.getPixelColor(pixel);
}

/**
 * SetAll
 *
 * Sets all the pixels to the specified RGB value.
 * 
 * @param red The red value you'd like the pixel to have.
 * @param green The green value you'd like the pixel to have.
 * @param blue The blue value you'd like the pixel to have.
 */
void SetAll(byte red, byte green, byte blue) {
  for(int i = 0; i < NUM_LEDS; i++ ) {
    if (CheckForNewCommand())
      return;

    SetPixel(i, red, green, blue); 
  }

  ShowStrip();
}

/***************************************
 ********* Animation Functions *********
 ***************************************/

/**
 * RGBLoop
 *
 * Runs a pattern of fading in and out of red, green, then blue.
 */
void RGBLoop(){
  for(int j = 0; j < 3; j++ ) { 
    // Fade IN
    for(int k = 0; k < 256; k++) {
      if (CheckForNewCommand())
        return;

      switch(j) { 
        case 0: SetAll(k, 0, 0); break;
        case 1: SetAll(0, k, 0); break;
        case 2: SetAll(0, 0, k); break;
      }

      ShowStrip();
      delay(3);
    }

    // Fade OUT
    for(int k = 255; k >= 0; k--) { 
      if (CheckForNewCommand())
        return;

      switch(j) { 
        case 0: SetAll(k, 0, 0); break;
        case 1: SetAll(0, k, 0); break;
        case 2: SetAll(0, 0, k); break;
      }

      ShowStrip();
      delay(3);
    }
  }
}

/**
 * Strobe
 *
 * Strobes the specified colour.
 * 
 * Ex: 
 *    Fast: Strobe(0xff, 0xff, 0xff, 10, 50);
 *    Slow: Strobe(0xff, 0x77, 0x00, 10, 100);
 * 
 * @param red The red value you'd like the pixels to have.
 * @param green The green value you'd like the pixels to have.
 * @param blue The blue value you'd like the pixels to have.
 * @param strobeCount The total number of times to blink.
 * @param flashDelay The amount of time to hold the lights on and off.
 */
void Strobe(byte red, byte green, byte blue, int strobeCount, int flashDelay){
  for(int j = 0; j < strobeCount; j++) {
    if (CheckForNewCommand())
      return;

    SetAll(red, green, blue);
    ShowStrip();
    delay(flashDelay);
    SetAll(0, 0, 0);
    ShowStrip();
    delay(flashDelay);
  }
}

/**
 * HalloweenEyes
 *
 * Randomly display a set of pixels to resemble eyes.
 * 
 * Ex: 
 *    Random: HalloweenEyes(0xff, 0x00, 0x00, 1, 4, true, random(5,50), random(50,150), random(1000, 10000));
 *    Fixed: HalloweenEyes(0xff, 0x00, 0x00, 1,4, true, 10, 80, 3000);
 * 
 * @param red The red value you'd like the pixels to have.
 * @param green The green value you'd like the pixels to have.
 * @param blue The blue value you'd like the pixels to have.
 * @param eyeWidth The amount of pixels to use for each eye.
 * @param eyeSpace The amount of pixels between each eye.
 * @param fade Do you want the eyes to fade in and out.
 * @param steps Used to calculate how quickly to fade (if fade is enabled).
 * @param fadeDelay Time to wait between the fades.
 * @param endPause Time to wait after the animation.
 */
void HalloweenEyes(byte red, byte green, byte blue, int eyeWidth, int eyeSpace, boolean fade, int steps, int fadeDelay, int endPause){
  randomSeed(analogRead(0));

  int StartPoint = random(0, NUM_LEDS - (2 * eyeWidth) - eyeSpace);
  int Start2ndEye = StartPoint + eyeWidth + eyeSpace;
  
  for(int i = 0; i < eyeWidth; i++) {
    if (CheckForNewCommand())
      return;

    SetPixel(StartPoint + i, red, green, blue);
    SetPixel(Start2ndEye + i, red, green, blue);
  }
  
  ShowStrip();
  
  if(fade == true) {
    float r, g, b;
  
    for(int j = steps; j >= 0; j--) {
      r = j * (red / steps);
      g = j * (green / steps);
      b = j * (blue / steps);
      
      for(int i = 0; i < eyeWidth; i++) {
        if (CheckForNewCommand())
          return;

        SetPixel(StartPoint + i, r, g, b);
        SetPixel(Start2ndEye + i, r, g, b);
      }
      
      ShowStrip();
      delay(fadeDelay);
    }
  }
  
  SetAll(0, 0, 0); // Set all black
  
  delay(endPause);
}

/**
 * CylonBounce
 *
 * Slides down the strip and bounces back in the given colour.
 * 
 * Ex: 
 *    CylonBounce(0xff, 0, 0, 4, 10, 50);
 * 
 * @param red The red value you'd like the pixels to have.
 * @param green The green value you'd like the pixels to have.
 * @param blue The blue value you'd like the pixels to have.
 * @param size Number of highlighted pixels
 * @param iterationDelay The delay between each iteration.
 * @param holdTime the time to hold the lights at the end stage before bouncing back.
 */
void CylonBounce(byte red, byte green, byte blue, int size, int iterationDelay, int holdTime){
  for(int i = 0; i < (NUM_LEDS - size - 2); i++) {
    if (CheckForNewCommand())
      return;

    SetAll(0, 0, 0);
    SetPixel(i, red / 10, green / 10, blue / 10);
    for(int j = 1; j <= size; j++) {
      if (CheckForNewCommand())
        return;

      SetPixel(i+j, red, green, blue); 
    }
    SetPixel(i + size + 1, red / 10, green / 10, blue / 10);
    ShowStrip();
    delay(iterationDelay);
  }

  delay(holdTime);

  for(int i = (NUM_LEDS - size - 2); i > 0; i--) {
    if (CheckForNewCommand())
      return;

    SetAll(0, 0, 0);
    SetPixel(i, red / 10, green / 10, blue / 10);

    for(int j = 1; j <= size; j++) {
      if (CheckForNewCommand())
        return;

      SetPixel(i + j, red, green, blue); 
    }

    SetPixel(i + size + 1, red / 10, green / 10, blue / 10);
    ShowStrip();
    delay(iterationDelay);
  }
  
  delay(holdTime);
}

/**
 * Twinkle
 *
 * Twinkles the given colour.
 * 
 * Ex: 
 *    Twinkle(0xff, 0, 0, 10, 100, false);
 * 
 * @param red The red value you'd like the pixels to have.
 * @param green The green value you'd like the pixels to have.
 * @param blue The blue value you'd like the pixels to have.
 * @param count Number of highlighted pixels before clearing.
 * @param iterationDelay The delay between each iteration.
 * @param onlyOne Only twinkle one at a time.
 */
void Twinkle(byte red, byte green, byte blue, int count, int iterationDelay, boolean onlyOne) {
  SetAll(0, 0, 0);
  
  for (int i = 0; i < count; i++) {
    if (CheckForNewCommand())
      return;

     SetPixel(random(NUM_LEDS), red, green, blue);
     ShowStrip();
     delay(iterationDelay);
     if(onlyOne) { 
       SetAll(0, 0, 0); 
     }
   }
  
  delay(iterationDelay);
}

/**
 * TwinkleRandom
 *
 * Twinkles but with a random colour.
 * 
 * Ex: 
 *    TwinkleRandom(20, 100, false);
 * 
 * @param count Number of highlighted pixels before clearing.
 * @param iterationDelay The delay between each iteration.
 * @param onlyOne Only twinkle one at a time.
 */
void TwinkleRandom(int count, int iterationDelay, boolean onlyOne) {
  SetAll(0, 0, 0);
  
  for (int i = 0; i < count; i++) {
    if (CheckForNewCommand())
    return;

    SetPixel(random(NUM_LEDS), random(0, 255), random(0, 255), random(0,255));
    ShowStrip();
    delay(iterationDelay);

    if(onlyOne) { 
      SetAll(0, 0, 0); 
    }
  }
  
  delay(iterationDelay);
}

/**
 * Sparkle
 *
 * Sparkles the given colour randomly.
 * 
 * Ex: 
 *    Sparkle(0xff, 0xff, 0xff, 0);
 * 
 * @param red The red value you'd like the pixels to have.
 * @param green The green value you'd like the pixels to have.
 * @param blue The blue value you'd like the pixels to have.
 * @param iterationDelay The delay between each sparkle.
 */
void Sparkle(byte red, byte green, byte blue, int iterationDelay) {
  int pixel = random(NUM_LEDS);
  if (CheckForNewCommand())
    return;

  SetPixel(pixel, red, green, blue);
  ShowStrip();
  delay(iterationDelay);
  SetPixel(pixel, 0, 0, 0);
}

/**
 * SnowSparkle
 *
 * Sets all lights to the given colour and sparkles white randomly.
 * 
 * Ex: 
 *    SnowSparkle(0x10, 0x10, 0x10, 20, random(100,1000));
 * 
 * @param red The red value you'd like the pixels to have.
 * @param green The green value you'd like the pixels to have.
 * @param blue The blue value you'd like the pixels to have.
 * @param sparkleVisible The time that the sparkle is visible.
 * @param sparkleDelay The time to wait until the next sparkle.
 */
void SnowSparkle(byte red, byte green, byte blue, int sparkleVisible, int sparkleDelay) {
  SetAll(red, green, blue);
  if (CheckForNewCommand())
    return;

  int pixel = random(NUM_LEDS);
  SetPixel(pixel, 0xff, 0xff, 0xff);
  ShowStrip();
  delay(sparkleVisible);
  SetPixel(pixel, red, green, blue);
  ShowStrip();
  delay(sparkleDelay);
}

/**
 * RunningLights
 *
 * Slides the specified colour across the strip in a wave-like
 * animation.
 * 
 * Ex: 
 *    RunningLights(0xff,0,0, 50);        // red
 *    RunningLights(0xff,0xff,0xff, 50);  // white
 *    RunningLights(0,0,0xff, 50);        // blue
 * 
 * @param red The red value you'd like the pixels to have.
 * @param green The green value you'd like the pixels to have.
 * @param blue The blue value you'd like the pixels to have.
 * @param waveDelay The time to wait between each iteration.
 */
void RunningLights(byte red, byte green, byte blue, int waveDelay) {
  int Position=0;
  
  for(int j = 0; j < NUM_LEDS * 2; j++) {
    Position++; // = 0; //Position + Rate;
    for(int i = 0; i < NUM_LEDS; i++) {
      if (CheckForNewCommand())
        return;

      // sine wave, 3 offset waves make a rainbow!
      //float level = sin(i+Position) * 127 + 128;
      //SetPixel(i,level,0,0);
      //float level = sin(i+Position) * 127 + 128;
      SetPixel(i,((sin(i + Position) * 127 + 128) / 255) * red,
                  ((sin(i + Position) * 127 + 128) / 255) * green,
                  ((sin(i + Position) * 127 + 128) / 255) * blue);
    }
    
    ShowStrip();
    delay(waveDelay);
  }
}

/**
 * ColorWipe
 *
 * Wipes the specified colour across the whole strip.
 * 
 * Ex: 
 *    ColorWipe(0x00,0xff,0x00, 50); //Fill
 *    ColorWipe(0x00,0x00,0x00, 50); //Clear
 * 
 * @param red The red value you'd like the pixels to have.
 * @param green The green value you'd like the pixels to have.
 * @param blue The blue value you'd like the pixels to have.
 * @param iterationDelay The time to wait between each iteration.
 */
void ColorWipe(byte red, byte green, byte blue, int iterationDelay) {
  for(uint16_t i = 0; i < NUM_LEDS; i++) {
    if (CheckForNewCommand())
      return;

    SetPixel(i, red, green, blue);
    ShowStrip();
    delay(iterationDelay);
  }
}

/**
 * RainbowCycle
 *
 * Gently transitions between the entire rainbow through every pixel.
 * 
 * Ex: 
 *    RainbowCycle(20);
 * 
 * @param cycleDelay The time to wait between each full cycle of colours.
 */
void RainbowCycle(int cycleDelay) {
  byte *c;

  for(uint16_t j = 0; j < 256 * 5; j++) { // 5 cycles of all colors on wheel
    for(uint16_t i = 0; i < NUM_LEDS; i++) {
      if (CheckForNewCommand())
        return;

      c = Wheel(((i * 256 / NUM_LEDS) + j) & 255);
      SetPixel(i, *c, *(c+1), *(c+2));
    }

    ShowStrip();
    delay(cycleDelay);
  }
}

/**
 * TheaterChaseRainbow
 *
 * Alternating pixel movements as it iterates through the rainbow.
 * 
 * Ex: 
 *    TheaterChaseRainbow(50);
 * 
 * @param cycleDelay The time to wait between each full cycle of colours.
 */
void TheaterChaseRainbow(int cycleDelay) {
  byte *c;
  
  for (int j = 0; j < 256; j++) {     // cycle all 256 colors in the wheel
    for (int q = 0; q < 3; q++) {
      for (int i = 0; i < NUM_LEDS; i = i + 3) {
        if (CheckForNewCommand())
          return;

        c = Wheel((i + j) % 255);
        SetPixel(i + q, *c, *(c + 1), *(c + 2));    //turn every third pixel on
      }

      ShowStrip();
      
      delay(cycleDelay);
      
      for (int i = 0; i < NUM_LEDS; i = i + 3) {
        if (CheckForNewCommand())
          return;

        SetPixel(i + q, 0, 0, 0);        //turn every third pixel off
      }
    }
  }
}

/**
 * MeteorRain
 *
 * Simulates a meteor shower.
 * 
 * Ex: 
 *    MeteorRain(0xff,0xff,0xff,10, 64, true, 30);
 * 
 * @param red The red value you'd like the pixels to have.
 * @param green The green value you'd like the pixels to have.
 * @param blue The blue value you'd like the pixels to have.
 * @param meteorSize Number of pixels in each meteor.
 * @param meteorTrailDecay Rate at which the meteor fades out.
 * @param meteorRandomDecay Use a random rate for the metor fades.
 * @param iterationDelay Time between each iteration.
 */
void MeteorRain(byte red, byte green, byte blue, byte meteorSize, byte meteorTrailDecay, boolean meteorRandomDecay, int iterationDelay) {  
  SetAll(0, 0, 0);
  
  for(int i = 0; i < NUM_LEDS+NUM_LEDS; i++) {      
    // fade brightness all LEDs one step
    for(int j = 0; j < NUM_LEDS; j++) {
      if (CheckForNewCommand())
        return;

      if( (!meteorRandomDecay) || (random(10) > 5) ) {
        FadeToBlack(j, meteorTrailDecay);
      }
    }
    
    // draw meteor
    for(int j = 0; j < meteorSize; j++) {
      if (CheckForNewCommand())
        return;

      if( (i - j < NUM_LEDS) && (i - j >= 0) )
        SetPixel(i - j, red, green, blue);
    }
   
    ShowStrip();
    delay(iterationDelay);
  }
}

/**
 * ColorWipe
 *
 * Iterates through all the pixels and sets the pixels one after
 * the other to the specified color.
 * 
 * Ex: 
 *    ColorWipe(0xff, 0xff, 0xff, 10)
 * 
 * @param red The red value you'd like the pixels to have.
 * @param green The green value you'd like the pixels to have.
 * @param blue The blue value you'd like the pixels to have.
 * @param wait Time to wait before setting the next pixel.
 */
void ColorWipe(byte r, byte g, byte b, uint8_t wait) {
  for(uint16_t i = 0; i < NUM_LEDS; i++) {
    if (CheckForNewCommand())
      return;

    SetPixel(i, r, g, b);
    ShowStrip();
    delay(wait);
  }
}

/**************************************
 ********** Sinric Functions **********
 **************************************/

/**
 * LightPowerState
 *
 * Called by the Sinric Pro library when the power state is updated.
 * When called, will set the state of the power.
 * 
 * @param deviceId The ID of the device that received the messege..
 * @param passedState The new power state.
 * 
 * @returns True if successfully processed.
 */
bool LightPowerState(const String &deviceId, bool &passedState) {
  Serial.printf("Light %s power turned %s \r\n", deviceId.c_str(), passedState ? "on" : "off");
  state = passedState;
  return true;
}

/**
 * LightBrightness
 *
 * Called by the Sinric Pro library when the brightness is updated.
 * When called, will set the brightness of the strip.
 * 
 * @param deviceId The ID of the device that received the messege..
 * @param brightness The new brightness level.
 * 
 * @returns True if successfully processed.
 */
bool LightBrightness(const String &deviceId, int &brightness) {
  SetBrightness((255 / 100) * brightness);
  Serial.printf("Light %s brightness level changed to %d\r\n", deviceId.c_str(), brightness);
  return true;
}

/**
 * LightColourTemperature
 *
 * Called by the Sinric Pro library when the colour temperature is updated.
 * When called, the pattern will be triggered that is set for the specific temperature.
 * 
 * @param deviceId The ID of the device that received the messege..
 * @param colorTemperature The new colour temperature.
 * 
 * @returns True if successfully processed.
 */
bool LightColourTemperature(const String &deviceId, int &colorTemperature) {
  pattern = colorTemperature;
  Serial.printf("Light %s colour temperature changed to %d\r\n", deviceId.c_str(), colorTemperature);
  return true;
}

/**
 * LightColour
 *
 * Called by the Sinric Pro library when the colour is updated.
 * When called, will set the colour of the strip and stop any patterns.
 * 
 * @param deviceId The ID of the device that received the messege.
 * @param r The red value you'd like the pixels to have.
 * @param g The green value you'd like the pixels to have.
 * @param b The blue value you'd like the pixels to have.
 * 
 * @returns True if successfully processed.
 */
bool LightColour(const String &deviceId, byte &r, byte &g, byte &b) {
  SetAll(r, g, b);
  ShowStrip();
  pattern = 0;
  Serial.printf("Light %s colour changed to %d, %d, %d (RGB)\r\n", deviceId.c_str(), r, g, b);
  return true;
}

/**
 * PresetPowerState
 *
 * Called by the Sinric Pro library when the power state is updated.
 * When called, we will accept the message, but not actually change anything.
 * 
 * @param deviceId The ID of the device that received the messege..
 * @param passedState The new power state.
 * 
 * @returns True if successfully processed.
 */
bool PresetPowerState(const String &deviceId, bool &passedState) {
  Serial.printf("Ignoring Preset %s power state change.\r\n", deviceId.c_str());
  return true;
}

/**
 * PresetColourTemperature
 *
 * Called by the Sinric Pro library when the colour temperature is updated.
 * When called, we will accept the message, but not actually change anything.
 * 
 * @param deviceId The ID of the device that received the messege..
 * @param colorTemperature The new colour temperature.
 * 
 * @returns True if successfully processed.
 */
bool PresetColourTemperature(const String &deviceId, int &colorTemperature) {
  Serial.printf("Ignoring Preset %s colour temperature change.\r\n", deviceId.c_str());
  return true;
}

/**
 * PresetColour
 *
 * Called by the Sinric Pro library when the colour is updated.
 * When called, we will accept the message, but not actually change anything.
 * 
 * @param deviceId The ID of the device that received the messege.
 * @param r The red value you'd like the pixels to have.
 * @param g The green value you'd like the pixels to have.
 * @param b The blue value you'd like the pixels to have.
 * 
 * @returns True if successfully processed.
 */
bool PresetColour(const String &deviceId, byte &r, byte &g, byte &b) {
  Serial.printf("Ignoring Preset %s colour change.\r\n", deviceId.c_str());
  return true;
}

/**
 * PresetBrightness
 *
 * Called by the Sinric Pro library when the brightness is updated.
 * When called, will set the preset to the specified value.
 * 
 * @param deviceId The ID of the device that received the messege..
 * @param brightness The new brightness level but is interpreted as the desired preset.
 * 
 * @returns True if successfully processed.
 */
bool PresetBrightness(const String &deviceId, int &brightness) {
  pattern = brightness;
  Serial.printf("Preset %s brightness level changed to %d. Setting preset to %d\r\n", deviceId.c_str(), brightness, brightness);
  return true;
}

/**
 * SetupSinricPro
 *
 * Set up all Sinric Pro lights and connect to the service.
 */
/***************************************
 *********** Setup Functions ***********
 ***************************************/
void SetupSinricPro() {
  // Get a new Light device from SinricPro
  SinricProLight &light = SinricPro[LIGHT_ID];

  // Set callback functions to device
  light.onPowerState(LightPowerState);
  light.onBrightness(LightBrightness);
  light.onColor(LightColour);
  light.onColorTemperature(LightColourTemperature);

  if (USE_PRESET_DEVICE) {
    // Get a new Light device from SinricPro 
    SinricProLight &preset = SinricPro[PRESET_ID];

    // Set callback functions to device
    preset.onPowerState(PresetPowerState);
    preset.onBrightness(PresetBrightness);
    preset.onColor(PresetColour);
    preset.onColorTemperature(PresetColourTemperature);
  }

  // setup SinricPro
  SinricPro.onConnected([](){
    Serial.printf("Connected to SinricPro\r\n"); 
  }); 
  
  SinricPro.onDisconnected([](){ 
    Serial.printf("Disconnected from SinricPro\r\n");
  });

  SinricPro.begin(APP_KEY, APP_SECRET);
}

/**
 * SetupWiFi
 *
 * Set up the Wi-Fi connection and connect to the AP.
 */
void SetupWiFi() {
  Serial.printf("\r\n[Wifi]: Connecting");
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  while (WiFi.status() != WL_CONNECTED) {
    Serial.printf(".");
    delay(250);
  }

  IPAddress localIP = WiFi.localIP();
  Serial.printf("connected!\r\n[WiFi]: IP-Address is %d.%d.%d.%d\r\n", localIP[0], localIP[1], localIP[2], localIP[3]);
}

/**
 * setup
 *
 * Arduino required setup loop. Runs all setup functions.
 */
void setup() {
  Serial.begin(115200);
   // NeoPixel
  strip.begin();
  SetBrightness(255); // Set brightness to max
  ShowStrip(); // Initialize all pixels to 'off'
  
  SetupWiFi();
  SetupSinricPro();
}

/***************************************
 ************** Main Loop **************
 ***************************************/
void loop() {
  if (state == 0)
    SetAll(0, 0, 0);
  else {
    switch (pattern) {
      // Start of basic presets controlled using the 5 colour temperatures
      // of the primary light device.
      case 4000: 
        RainbowCycle(20);
        break;
        
      case 2200: 
        TheaterChaseRainbow(50);
        break;
        
      case 2700: 
        RunningLights(0xff, 0, 0, 50);        // red
        RunningLights(0xff, 0xff, 0xff, 50);  // white
        RunningLights(0, 0, 0xff, 50);        // blue
        break;
        
      case 7000: 
        MeteorRain(0xff, 0xff, 0xff, 10, 64, true, 30);
        break;
        
      case 5500: 
        ColorWipe(255, 0, 0, 50); // Red
        ColorWipe(0, 255, 0, 50); // Green
        ColorWipe(0, 0, 255, 50); // Blue
        ColorWipe(0, 0, 0, 50); // Black
        break;

      // Start of the additional presets controlled using the brightness of
      // the preset light device.
      case 1:
        RGBLoop();
        break;
        
      case 2:
        Strobe(0xff, 0xff, 0xff, 10, 50); // Fast strobe
        break;
        
      case 3:
        HalloweenEyes(0xff, 0x00, 0x00, 1, 4, true, random(5,50), random(50,150), random(1000, 10000));
        break;
        
      case 4:
        CylonBounce(0xff, 0, 0, 4, 10, 50);
        break;
        
      case 5:
        MeteorRain(0xff,0xff,0xff,10, 64, true, 30);
        break;
        
      case 6:
        Twinkle(0xff, 0, 0, 10, 100, false);
        break;
        
      case 7:
        TwinkleRandom(20, 100, false);
        break;
        
      case 8:
        Sparkle(0xff, 0xff, 0xff, 0);
        break;
        
      case 9:
        SnowSparkle(0x10, 0x10, 0x10, 20, random(100,1000));
        break;
        
      case 10:
        TheaterChaseRainbow(50);
        break;
      
      // Default case used for when pattern is set to 0. Mainly used for when you set a specific colour
      // using the primary light control.
      default:
        CheckForNewCommand();
        break;
    }
  }
}