
#include <sys/time.h>
#include <Arduino.h>

#include <Adafruit_GFX.h>
#include <Fonts/FreeMono12pt7b.h>
#include <Fonts/FreeSans12pt7b.h>
#include <Fonts/FreeSerif12pt7b.h>
#include <Fonts/FreeSans18pt7b.h>
#include <Fonts/FreeSerif18pt7b.h>
#include <Fonts/FreeSans24pt7b.h>
#include "Adafruit_SSD1306.h"

#include "Display.h"

#define OLED_RESET -1

using namespace std;

Adafruit_SSD1306 display (OLED_RESET);

void displayBegin() {
  display.begin(SSD1306_SWITCHCAPVCC, SSD1306_I2C_ADDRESS);
}

void displayText(const char *text) {
  auto width = display.width();
  auto height = display.height();
  
  display.clearDisplay();
  display.setFont (&FreeSans12pt7b);
  display.setTextSize(1);
  //display.startscrollleft(0x00, 0x0F);
  display.stopscroll();
  display.setTextColor(WHITE);
  display.setTextWrap(false);
  int16_t bx, by;
  uint16_t bw, bh;
  display.getTextBounds (text, 0, 0, &bx, &by, &bw, &bh);

  display.setCursor ((width - bw) / 2, (height - bh) / 2 + bh);
  display.print (text);
  
  display.display();
}

void displayTime(const reltime &rel) {
  auto width = display.width();
  auto height = display.height();

  char f[128];
  String msg;
  if (rel.invert) {
    msg += "-";
  }
  if (rel.y > 0) {
    sprintf(f, "H + J %dy", rel.y);
    msg = f;
  }
  else {
    msg = "H + J";
  }
  const char *text = msg.c_str();
  display.clearDisplay();
  display.setFont (&FreeSerif18pt7b);
  display.setTextSize(1);
  //display.startscrollleft(0x00, 0x0F);
  display.stopscroll();
  display.setTextColor(WHITE);
  display.setTextWrap(false);
  
  int16_t bx, by;
  uint16_t bw, bh;
  
  display.getTextBounds (text, 0, 0, &bx, &by, &bw, &bh);
  display.setCursor ((width - bw) / 2, 24);
  display.print (text);

  const char *head = "";
  msg = "";
  if (rel.invert) {
    msg += "-";
  }
  if (rel.m > 0) {
    sprintf(f, "%s%dm", head, rel.m);
    head = " ";
    msg += f;
  }
  sprintf(f, "%s%.1fd", head, rel.d + (rel.h * 60 * 60 + rel.i * 60 + rel.s)/(24.0*60.0*60.0));
  msg += f;
  
  text = msg.c_str();
  display.setFont (&FreeSans12pt7b);
  display.getTextBounds (text, 0, 0, &bx, &by, &bw, &bh);
  display.setCursor ((width - bw) / 2, 58);
  display.print (text);
  
  display.display();
}
