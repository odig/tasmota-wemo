/*
  xdrv_ws2812.ino - ws2812 led string support for Sonoff-Tasmota

  Copyright (C) 2017  Heiko Krupp and Theo Arends

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifdef USE_WS2812
/*********************************************************************************************\
 * WS2812 Leds using NeopixelBus library
\*********************************************************************************************/

//#include <NeoPixelBus.h>  // Global defined as also used by Sonoff Led

#ifdef USE_WS2812_DMA
#if (USE_WS2812_CTYPE == 1)
  NeoPixelBus<NeoGrbFeature, Neo800KbpsMethod> *strip = NULL;
#else  // USE_WS2812_CTYPE
  NeoPixelBus<NeoRgbFeature, Neo800KbpsMethod> *strip = NULL;
#endif  // USE_WS2812_CTYPE
#else  // USE_WS2812_DMA
#if (USE_WS2812_CTYPE == 1)
  NeoPixelBus<NeoGrbFeature, NeoEsp8266BitBang800KbpsMethod> *strip = NULL;
#else  // USE_WS2812_CTYPE
  NeoPixelBus<NeoRgbFeature, NeoEsp8266BitBang800KbpsMethod> *strip = NULL;
#endif  // USE_WS2812_CTYPE
#endif  // USE_WS2812_DMA

struct wsColor {
  uint8_t red, green, blue;
};

struct ColorScheme {
  wsColor* colors;
  uint8_t count;
};

wsColor incandescent[2] = { 255, 140, 20, 0, 0, 0 };
wsColor rgb[3] = { 255, 0, 0, 0, 255, 0, 0, 0, 255 };
wsColor christmas[2] = { 255, 0, 0, 0, 255, 0 };
wsColor hanukkah[2] = { 0, 0, 255, 255, 255, 255 };
wsColor kwanzaa[3] = { 255, 0, 0, 0, 0, 0, 0, 255, 0 };
wsColor rainbow[7] = { 255, 0, 0, 255, 128, 0, 255, 255, 0, 0, 255, 0, 0, 0, 255, 128, 0, 255, 255, 0, 255 };
wsColor fire[3] = { 255, 0, 0, 255, 102, 0, 255, 192, 0 };
ColorScheme schemes[7] = {
  incandescent, 2,
  rgb, 3,
  christmas, 2,
  hanukkah, 2,
  kwanzaa, 3,
  rainbow, 7,
  fire, 3 };

uint8_t widthValues[5] = {
    1,     // Small
    2,     // Medium
    4,     // Large
    8,     // Largest
  255 };   // All
uint8_t repeatValues[5] = {
    8,     // Small
    6,     // Medium
    4,     // Large
    2,     // Largest
    1 };   // All
uint8_t speedValues[6] = {
    0,                     // None
    9 * (STATES / 10),     // Slowest
    7 * (STATES / 10),     // Slower
    5 * (STATES / 10),     // Slow
    3 * (STATES / 10),     // Fast
    1 * (STATES / 10) };   // Fastest

uint8_t lany = 0;
RgbColor dcolor;
RgbColor tcolor;
RgbColor lcolor;

uint8_t wakeupDimmer = 0;
uint8_t ws_bit = 0;
uint16_t wakeupCntr = 0;
unsigned long stripTimerCntr = 0;  // Bars and Gradient

void ws2812_setDim(uint8_t myDimmer)
{
  float newDim = 100 / (float)myDimmer;
  float fmyRed = (float)sysCfg.ws_red / newDim;
  float fmyGrn = (float)sysCfg.ws_green / newDim;
  float fmyBlu = (float)sysCfg.ws_blue / newDim;
  dcolor.R = (uint8_t)fmyRed;
  dcolor.G = (uint8_t)fmyGrn;
  dcolor.B = (uint8_t)fmyBlu;
}

void ws2812_setColor(uint16_t led, char* colstr)
{
  HtmlColor hcolor;
  char lcolstr[8];

  snprintf_P(lcolstr, sizeof(lcolstr), PSTR("#%s"), colstr);
  uint8_t result = hcolor.Parse<HtmlColorNames>((char *)lcolstr, 7);
  if (result) {
    if (led) {
      strip->SetPixelColor(led -1, RgbColor(hcolor)); // Led 1 is strip Led 0 -> substract offset 1
      strip->Show();
    } else {
      dcolor = RgbColor(hcolor);

//      snprintf_P(log_data, sizeof(log_data), PSTR("DBG: Red %02X, Green %02X, Blue %02X"), dcolor.R, dcolor.G, dcolor.B);
//      addLog(LOG_LEVEL_DEBUG);

      uint16_t temp = dcolor.R;
      if (temp < dcolor.G) {
        temp = dcolor.G;
      }
      if (temp < dcolor.B) {
        temp = dcolor.B;
      }
      float mDim = (float)temp / 2.55;
      sysCfg.ws_dimmer = (uint8_t)mDim;

      float newDim = 100 / mDim;
      float fmyRed = (float)dcolor.R * newDim;
      float fmyGrn = (float)dcolor.G * newDim;
      float fmyBlu = (float)dcolor.B * newDim;
      sysCfg.ws_red = (uint8_t)fmyRed;
      sysCfg.ws_green = (uint8_t)fmyGrn;
      sysCfg.ws_blue = (uint8_t)fmyBlu;

      lany = 1;
    }
  }
}

void ws2812_getColor(uint16_t led)
{
  RgbColor mcolor;
  char stemp[20];

  if (led) {
    mcolor = strip->GetPixelColor(led -1);
    snprintf_P(stemp, sizeof(stemp), PSTR(D_CMND_LED "%d"), led);
  } else {
    ws2812_setDim(sysCfg.ws_dimmer);
    mcolor = dcolor;
    snprintf_P(stemp, sizeof(stemp), PSTR(D_CMND_COLOR));
  }
  uint32_t color = (uint32_t)mcolor.R << 16;
  color += (uint32_t)mcolor.G << 8;
  color += (uint32_t)mcolor.B;
  snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("{\"%s\":\"%06X\"}"), stemp, color);
}

void ws2812_stripShow()
{
  RgbColor c;

  if (sysCfg.ws_ledtable) {
    for (uint16_t i = 0; i < sysCfg.ws_pixels; i++) {
      c = strip->GetPixelColor(i);
      strip->SetPixelColor(i, RgbColor(ledTable[c.R], ledTable[c.G], ledTable[c.B]));
    }
  }
  strip->Show();
}

void ws2812_resetWakupState()
{
  wakeupDimmer = 0;
  wakeupCntr = 0;
}

void ws2812_resetStripTimer()
{
  stripTimerCntr = 0;
}

int mod(int a, int b)
{
   int ret = a % b;
   if (ret < 0) {
    ret += b;
   }
   return ret;
}

void ws2812_clock()
{
  RgbColor c;

  strip->ClearTo(0);   // Reset strip
  float newDim = 100 / (float)sysCfg.ws_dimmer;
  float f1 = 255 / newDim;
  uint8_t i1 = (uint8_t)f1;
  float f2 = 127 / newDim;
  uint8_t i2 = (uint8_t)f2;
  float f3 = 63 / newDim;
  uint8_t i3 = (uint8_t)f3;

  int j = sysCfg.ws_pixels;
  int clksize = 600 / j;
  int i = (rtcTime.Second * 10) / clksize;

  c = strip->GetPixelColor(mod(i,    j)); c.B = i1; strip->SetPixelColor(mod(i,    j), c);
  i = (rtcTime.Minute * 10) / clksize;
  c = strip->GetPixelColor(mod(i -1, j)); c.G = i3; strip->SetPixelColor(mod(i -1, j), c);
  c = strip->GetPixelColor(mod(i,    j)); c.G = i1; strip->SetPixelColor(mod(i,    j), c);
  c = strip->GetPixelColor(mod(i +1, j)); c.G = i3; strip->SetPixelColor(mod(i +1, j), c);
  i = (rtcTime.Hour % 12) * (50 / clksize);
  c = strip->GetPixelColor(mod(i -2, j)); c.R = i3; strip->SetPixelColor(mod(i -2, j), c);
  c = strip->GetPixelColor(mod(i -1, j)); c.R = i2; strip->SetPixelColor(mod(i -1, j), c);
  c = strip->GetPixelColor(mod(i,    j)); c.R = i1; strip->SetPixelColor(mod(i,    j), c);
  c = strip->GetPixelColor(mod(i +1, j)); c.R = i2; strip->SetPixelColor(mod(i +1, j), c);
  c = strip->GetPixelColor(mod(i +2, j)); c.R = i3; strip->SetPixelColor(mod(i +2, j), c);
  ws2812_stripShow();
}

void ws2812_gradientColor(struct wsColor* mColor, uint16_t range, uint16_t gradRange, uint16_t i)
{
/*
 * Compute the color of a pixel at position i using a gradient of the color scheme.
 * This function is used internally by the gradient function.
 */
  ColorScheme scheme = schemes[sysCfg.ws_scheme -3];
  uint16_t curRange = i / range;
  uint16_t rangeIndex = i % range;
  uint16_t colorIndex = rangeIndex / gradRange;
  uint16_t start = colorIndex;
  uint16_t end = colorIndex +1;
  if (curRange % 2 != 0) {
    start = (scheme.count -1) - start;
    end = (scheme.count -1) - end;
  }
  float newDim = 100 / (float)sysCfg.ws_dimmer;
  float fmyRed = (float)map(rangeIndex % gradRange, 0, gradRange, scheme.colors[start].red, scheme.colors[end].red) / newDim;
  float fmyGrn = (float)map(rangeIndex % gradRange, 0, gradRange, scheme.colors[start].green, scheme.colors[end].green) / newDim;
  float fmyBlu = (float)map(rangeIndex % gradRange, 0, gradRange, scheme.colors[start].blue, scheme.colors[end].blue) / newDim;
  mColor->red = (uint8_t)fmyRed;
  mColor->green = (uint8_t)fmyGrn;
  mColor->blue = (uint8_t)fmyBlu;
}

void ws2812_gradient()
{
/*
 * This routine courtesy Tony DiCola (Adafruit)
 * Display a gradient of colors for the current color scheme.
 *  Repeat is the number of repetitions of the gradient (pick a multiple of 2 for smooth looping of the gradient).
 */
  RgbColor c;

  ColorScheme scheme = schemes[sysCfg.ws_scheme -3];
  if (scheme.count < 2) {
    return;
  }

  uint8_t repeat = repeatValues[sysCfg.ws_width];  // number of scheme.count per ledcount
  uint16_t range = (uint16_t)ceil((float)sysCfg.ws_pixels / (float)repeat);
  uint16_t gradRange = (uint16_t)ceil((float)range / (float)(scheme.count - 1));
  uint16_t offset = speedValues[sysCfg.ws_speed] > 0 ? stripTimerCntr / speedValues[sysCfg.ws_speed] : 0;

  wsColor oldColor, currentColor;
  ws2812_gradientColor(&oldColor, range, gradRange, offset);
  currentColor = oldColor;
  for (uint16_t i = 0; i < sysCfg.ws_pixels; i++) {
    if (repeatValues[sysCfg.ws_width] > 1) {
      ws2812_gradientColor(&currentColor, range, gradRange, i +offset);
    }
    if (sysCfg.ws_speed > 0) {
      // Blend old and current color based on time for smooth movement.
      c.R = map(stripTimerCntr % speedValues[sysCfg.ws_speed], 0, speedValues[sysCfg.ws_speed], oldColor.red, currentColor.red);
      c.G = map(stripTimerCntr % speedValues[sysCfg.ws_speed], 0, speedValues[sysCfg.ws_speed], oldColor.green, currentColor.green);
      c.B = map(stripTimerCntr % speedValues[sysCfg.ws_speed], 0, speedValues[sysCfg.ws_speed], oldColor.blue, currentColor.blue);
    }
    else {
      // No animation, just use the current color.
      c.R = currentColor.red;
      c.G = currentColor.green;
      c.B = currentColor.blue;
    }
    strip->SetPixelColor(i, c);
    oldColor = currentColor;
  }
  ws2812_stripShow();
}

void ws2812_bars()
{
/*
 * This routine courtesy Tony DiCola (Adafruit)
 * Display solid bars of color for the current color scheme.
 * Width is the width of each bar in pixels/lights.
 */
  RgbColor c;
  uint16_t i;

  ColorScheme scheme = schemes[sysCfg.ws_scheme -3];

  uint16_t maxSize = sysCfg.ws_pixels / scheme.count;
  if (widthValues[sysCfg.ws_width] > maxSize) {
    maxSize = 0;
  }

  uint8_t offset = speedValues[sysCfg.ws_speed] > 0 ? stripTimerCntr / speedValues[sysCfg.ws_speed] : 0;

  wsColor mcolor[scheme.count];
  memcpy(mcolor, scheme.colors, sizeof(mcolor));
  float newDim = 100 / (float)sysCfg.ws_dimmer;
  for (i = 0; i < scheme.count; i++) {
    float fmyRed = (float)mcolor[i].red / newDim;
    float fmyGrn = (float)mcolor[i].green / newDim;
    float fmyBlu = (float)mcolor[i].blue / newDim;
    mcolor[i].red = (uint8_t)fmyRed;
    mcolor[i].green = (uint8_t)fmyGrn;
    mcolor[i].blue = (uint8_t)fmyBlu;
  }
  uint8_t colorIndex = offset % scheme.count;
  for (i = 0; i < sysCfg.ws_pixels; i++) {
    if (maxSize) {
      colorIndex = ((i + offset) % (scheme.count * widthValues[sysCfg.ws_width])) / widthValues[sysCfg.ws_width];
    }
    c.R = mcolor[colorIndex].red;
    c.G = mcolor[colorIndex].green;
    c.B = mcolor[colorIndex].blue;
    strip->SetPixelColor(i, c);
  }
  ws2812_stripShow();
}

void ws2812_animate()
{
  uint8_t fadeValue;

  stripTimerCntr++;
  if (0 == bitRead(power, ws_bit)) {  // Power Off
    sleep = sysCfg.sleep;
    stripTimerCntr = 0;
    tcolor = 0;
  }
  else {
    sleep = 0;
    switch (sysCfg.ws_scheme) {
      case 0:  // Power On
        ws2812_setDim(sysCfg.ws_dimmer);
        if (0 == sysCfg.ws_fade) {
          tcolor = dcolor;
        } else {
          if (tcolor != dcolor) {
            uint8_t ws_speed = speedValues[sysCfg.ws_speed];
            if (tcolor.R < dcolor.R) {
              tcolor.R += ((dcolor.R - tcolor.R) / ws_speed) +1;
            }
            if (tcolor.G < dcolor.G) {
              tcolor.G += ((dcolor.G - tcolor.G) / ws_speed) +1;
            }
            if (tcolor.B < dcolor.B) {
              tcolor.B += ((dcolor.B - tcolor.B) / ws_speed) +1;
            }
            if (tcolor.R > dcolor.R) {
              tcolor.R -= ((tcolor.R - dcolor.R) / ws_speed) +1;
            }
            if (tcolor.G > dcolor.G) {
              tcolor.G -= ((tcolor.G - dcolor.G) / ws_speed) +1;
            }
            if (tcolor.B > dcolor.B) {
              tcolor.B -= ((tcolor.B - dcolor.B) / ws_speed) +1;
            }
          }
        }
        break;
      case 1:  // Wake up light
        wakeupCntr++;
        if (0 == wakeupDimmer) {
          tcolor = 0;
          wakeupDimmer++;
        }
        else {
          if (wakeupCntr > ((sysCfg.ws_wakeup * STATES) / sysCfg.ws_dimmer)) {
            wakeupCntr = 0;
            wakeupDimmer++;
            if (wakeupDimmer <= sysCfg.ws_dimmer) {
              ws2812_setDim(wakeupDimmer);
              tcolor = dcolor;
            } else {
              sysCfg.ws_scheme = 0;
            }
          }
        }
        break;
      case 2:  // Clock
        if (((STATES/10)*2 == state) || (lany != 2)) {
          ws2812_clock();
        }
        lany = 2;
        break;
      default:
        if (1 == sysCfg.ws_fade) {
          ws2812_gradient();
        } else {
          ws2812_bars();
        }
        lany = 1;
        break;
    }
  }

  if ((sysCfg.ws_scheme <= 1) || (0 == bitRead(power, ws_bit))) {
    if ((lcolor != tcolor) || lany) {
      lany = 0;
      lcolor = tcolor;

//    snprintf_P(log_data, sizeof(log_data), PSTR("DBG: StripPixels %d, CfgPixels %d, Red %02X, Green %02X, Blue %02X"), strip->PixelCount(), sysCfg.ws_pixels, lcolor.R, lcolor.G, lcolor.B);
//    addLog(LOG_LEVEL_DEBUG);

      if (sysCfg.ws_ledtable) {
        for (uint16_t i = 0; i < sysCfg.ws_pixels; i++) {
          strip->SetPixelColor(i, RgbColor(ledTable[lcolor.R],ledTable[lcolor.G],ledTable[lcolor.B]));
        }
      } else {
        for (uint16_t i = 0; i < sysCfg.ws_pixels; i++) {
          strip->SetPixelColor(i, lcolor);
        }
      }
      strip->Show();
    }
  }
}

void ws2812_update()
{
  lany = 1;
}

void ws2812_pixels()
{
  strip->ClearTo(0);
  strip->Show();
  tcolor = 0;
  lany = 1;
}

void ws2812_init(uint8_t powerbit)
{
  ws_bit = powerbit -1;
#ifdef USE_WS2812_DMA
#if (USE_WS2812_CTYPE == 1)
  strip = new NeoPixelBus<NeoGrbFeature, Neo800KbpsMethod>(WS2812_MAX_LEDS);  // For Esp8266, the Pin is omitted and it uses GPIO3 due to DMA hardware use.
#else  // USE_WS2812_CTYPE
  strip = new NeoPixelBus<NeoRgbFeature, Neo800KbpsMethod>(WS2812_MAX_LEDS);  // For Esp8266, the Pin is omitted and it uses GPIO3 due to DMA hardware use.
#endif  // USE_WS2812_CTYPE
#else  // USE_WS2812_DMA
#if (USE_WS2812_CTYPE == 1)
  strip = new NeoPixelBus<NeoGrbFeature, NeoEsp8266BitBang800KbpsMethod>(WS2812_MAX_LEDS, pin[GPIO_WS2812]);
#else  // USE_WS2812_CTYPE
  strip = new NeoPixelBus<NeoRgbFeature, NeoEsp8266BitBang800KbpsMethod>(WS2812_MAX_LEDS, pin[GPIO_WS2812]);
#endif  // USE_WS2812_CTYPE
#endif  // USE_WS2812_DMA
  strip->Begin();
  ws2812_pixels();
}

/*********************************************************************************************\
 * Hue support
\*********************************************************************************************/

void ws2812_replaceHSB(String *response)
{
  ws2812_setDim(sysCfg.ws_dimmer);
  HsbColor hsb = HsbColor(dcolor);
  response->replace("{h}", String((uint16_t)(65535.0f * hsb.H)));
  response->replace("{s}", String((uint8_t)(254.0f * hsb.S)));
  response->replace("{b}", String((uint8_t)(254.0f * hsb.B)));
}

void ws2812_getHSB(float *hue, float *sat, float *bri)
{
  ws2812_setDim(sysCfg.ws_dimmer);
  HsbColor hsb = HsbColor(dcolor);
  *hue = hsb.H;
  *sat = hsb.S;
  *bri = hsb.B;
}

void ws2812_setHSB(float hue, float sat, float bri)
{
  char rgb[7];

  HsbColor hsb;
  hsb.H = hue;
  hsb.S = sat;
  hsb.B = bri;
  RgbColor tmp = RgbColor(hsb);
  sprintf(rgb,"%02X%02X%02X", tmp.R, tmp.G, tmp.B);
  ws2812_setColor(0,rgb);
}

/*********************************************************************************************\
 * Commands
\*********************************************************************************************/

boolean ws2812_command(char *type, uint16_t index, char *dataBuf, uint16_t data_len, int16_t payload)
{
  boolean serviced = true;

  if (!strcasecmp_P(type, PSTR(D_CMND_PIXELS))) {
    if ((payload > 0) && (payload <= WS2812_MAX_LEDS)) {
      sysCfg.ws_pixels = payload;
      ws2812_pixels();
    }
    snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("{\"" D_CMND_PIXELS "\":%d}"), sysCfg.ws_pixels);
  }
  else if (!strcasecmp_P(type, PSTR(D_CMND_LED)) && (index > 0) && (index <= sysCfg.ws_pixels)) {
    if (6 == data_len) {
      ws2812_setColor(index, dataBuf);
    }
    ws2812_getColor(index);
  }
  else if (!strcasecmp_P(type, PSTR(D_CMND_COLOR))) {
    if (dataBuf[0] == '#') {
      dataBuf++;
      data_len--;
    }
    if (6 == data_len) {
      ws2812_setColor(0, dataBuf);
      bitSet(power, ws_bit);
    }
    ws2812_getColor(0);
  }
  else if (!strcasecmp_P(type, PSTR(D_CMND_DIMMER))) {
    if ((payload >= 0) && (payload <= 100)) {
      sysCfg.ws_dimmer = payload;
      bitSet(power, ws_bit);
#ifdef USE_DOMOTICZ
//      mqtt_publishDomoticzPowerState(index);
//      mqtt_publishDomoticzPowerState(ws_bit +1);
      domoticz_updatePowerState(ws_bit +1);
#endif  // USE_DOMOTICZ
    }
    snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("{\"" D_CMND_DIMMER "\":%d}"), sysCfg.ws_dimmer);
  }
  else if (!strcasecmp_P(type, PSTR(D_CMND_LEDTABLE))) {
    if ((payload >= 0) && (payload <= 2)) {
      switch (payload) {
      case 0: // Off
      case 1: // On
        sysCfg.ws_ledtable = payload;
        break;
      case 2: // Toggle
        sysCfg.ws_ledtable ^= 1;
        break;
      }
      ws2812_update();
    }
    snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("{\"" D_CMND_LEDTABLE "\":\"%s\"}"), getStateText(sysCfg.ws_ledtable));
  }
  else if (!strcasecmp_P(type, PSTR(D_CMND_FADE))) {
    switch (payload) {
    case 0: // Off
    case 1: // On
      sysCfg.ws_fade = payload;
      break;
    case 2: // Toggle
      sysCfg.ws_fade ^= 1;
      break;
    }
    snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("{\"" D_CMND_FADE "\":\"%s\"}"), getStateText(sysCfg.ws_fade));
  }
  else if (!strcasecmp_P(type, PSTR(D_CMND_SPEED))) {  // 1 - fast, 5 - slow
    if ((payload > 0) && (payload <= 5)) {
      sysCfg.ws_speed = payload;
    }
    snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("{\"" D_CMND_SPEED "\":%d}"), sysCfg.ws_speed);
  }
  else if (!strcasecmp_P(type, PSTR(D_CMND_WIDTH))) {
    if ((payload >= 0) && (payload <= 4)) {
      sysCfg.ws_width = payload;
    }
    snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("{\"" D_CMND_WIDTH "\":%d}"), sysCfg.ws_width);
  }
  else if (!strcasecmp_P(type, PSTR(D_CMND_WAKEUP))) {
    if ((payload > 0) && (payload < 3001)) {
      sysCfg.ws_wakeup = payload;
      if (1 == sysCfg.ws_scheme) {
        sysCfg.ws_scheme = 0;
      }
    }
    snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("{\"" D_CMND_WAKEUP "\":%d}"), sysCfg.ws_wakeup);
  }
  else if (!strcasecmp_P(type, PSTR(D_CMND_SCHEME))) {
    if ((payload >= 0) && (payload <= 9)) {
      sysCfg.ws_scheme = payload;
      if (1 == sysCfg.ws_scheme) {
        ws2812_resetWakupState();
      }
      bitSet(power, ws_bit);
      ws2812_resetStripTimer();
    }
    snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("{\"" D_CMND_SCHEME "\":%d}"), sysCfg.ws_scheme);
  }
  else if (!strcasecmp_P(type, PSTR("UNDOCA"))) {  // Theos legacy status
    RgbColor mcolor;
    ws2812_setDim(sysCfg.ws_dimmer);
    mcolor = dcolor;
    uint32_t color = (uint32_t)mcolor.R << 16;
    color += (uint32_t)mcolor.G << 8;
    color += (uint32_t)mcolor.B;
    snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("%06X, %d, %d, %d, %d, %d"),
      color, sysCfg.ws_fade, sysCfg.ws_ledtable, sysCfg.ws_scheme, sysCfg.ws_speed, sysCfg.ws_width);
    mqtt_publish_topic_P(1, type);
    mqtt_data[0] = '\0';
  }
  else {
    serviced = false;  // Unknown command
  }
  return serviced;
}
#endif  // USE_WS2812
