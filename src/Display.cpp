// Copyright (c) 2023 Takenori Nakagawa <ww24gm+oss@gmail.com>

#include "../include/Display.hpp"

using m5util::Display;
using m5util::Font;
using m5util::Mutex;

#include <sdfonts.h>
#define SD_PN 4

Display::Display(M5Display* display, uint8_t offset, uint16_t width,
                 uint16_t height)
    : sprite(TFT_eSprite(display)),
      offset(offset),
      width(width),
      height(height) {
  sprite.createSprite(width, height);
  sprite.setSwapBytes(true);
  sprite.setTextColor(WHITE);
}

Display::~Display() { prefs.end(); }

void Display::begin(void) {
  bool initialized = prefs.begin(pref_name.c_str(), false);
  if (!initialized && prefs.clear()) {
    Serial.println("NVS cleared");
  }

  setStatus("");
  setContent("");
  setButtons("", "", "");
}

void Display::setStatus(std::string status) {
  mutex.lock();
  const auto bg = TFT_NAVY;

  auto sprite_header = TFT_eSprite(sprite);
  sprite_header.createSprite(width, 60);
  sprite_header.setSwapBytes(true);
  sprite_header.fillScreen(bg);
  sprite_header.pushSprite(0, 0);
  sprite_header.deleteSprite();

  font.setFontSize(24);
  font.setTextColor(TFT_WHITE);
  font.setBackgroundColor(bg);
  font.pushCentreString(&sprite_header, width / 2, 16, status.c_str());

  mutex.unlock();
}

void Display::setContent(std::string content) {
  const uint16_t offset_top = 60;
  const auto bg = TFT_DARKGREY;

  auto sprite_content = TFT_eSprite(sprite);
  sprite_content.createSprite(width, 120);
  sprite_content.setSwapBytes(true);
  sprite_content.fillScreen(bg);
  sprite_content.pushSprite(0, 60);
  sprite_content.deleteSprite();

  font.setFontSize(16);
  font.setTextColor(TFT_WHITE);
  font.setBackgroundColor(bg);
  font.pushString(&sprite_content, offset, offset_top + offset,
                  content.c_str());
}

void Display::setButtons(std::string btnA, std::string btnB, std::string btnC) {
  const uint16_t offset_top = 180;
  // for DEBUG
  // auto bg = TFT_MAROON;
  const auto bg = TFT_DARKGREY;

  auto sprite_footer = TFT_eSprite(sprite);
  sprite_footer.createSprite(width, 60);
  sprite_footer.setSwapBytes(true);
  sprite_footer.fillScreen(bg);
  sprite_footer.pushSprite(0, offset_top);
  sprite_footer.deleteSprite();

  font.setFontSize(20);
  font.setTextColor(TFT_WHITE);
  font.setBackgroundColor(bg);
  if (btnA.length() > 0) {
    font.pushCentreString(&sprite_footer, 68, offset_top + 36, btnA.c_str());
  }
  if (btnB.length() > 0) {
    font.pushCentreString(&sprite_footer, width / 2, offset_top + 36,
                          btnB.c_str());
  }
  if (btnC.length() > 0) {
    font.pushCentreString(&sprite_footer, 252, offset_top + 36, btnC.c_str());
  }
}

void Display::update(void) { sprite.pushSprite(0, 0); }

Font::Font(uint8_t font_size, uint16_t text_color, uint16_t background_color,
           uint16_t line_feed_width)
    : font_size(font_size),
      text_color(text_color),
      background_color(background_color),
      line_feed_width(line_feed_width) {
  // initialize JP font lib
  if (!SDfonts.init(SD_PN)) {
    Serial.println("sdfonts init error");
    exit(1);
  }
  SDfonts.setFontSize(font_size);
}

// print a character.
void Font::mputc(TFT_eSPI* tft, uint16_t x, uint16_t y, uint8_t* buf,
                 uint16_t fg, uint16_t bg) {
  uint16_t w = SDfonts.getWidth();
  uint16_t h = SDfonts.getHeight();
  uint16_t byteWidth = (w + 7) >> 3;
  uint8_t byte = 0;

  tft->setAddrWindow(x, y, w, h);
  for (int16_t j = 0; j < h; j++, y++) {
    for (int16_t i = 0; i < w; i++) {
      byte = (i & 7) ? byte << 1 : buf[j * byteWidth + (i >> 3)];
      tft->pushColor((byte & 0x80) ? fg : bg);
    }
  }
}

// print some characters.
void Font::mprint(TFT_eSPI* tft, uint16_t x, uint16_t y, const char* str,
                  uint16_t fg, uint16_t bg) {
  uint8_t buf[MAXFONTLEN];
  int16_t x0 = x, y0 = y;
  char* pUTF8 = const_cast<char*>(str);

  SDfonts.open();
  while ((pUTF8 = SDfonts.getFontData(buf, pUTF8))) {
    mputc(tft, x, y, buf, fg, bg);
    if (x + SDfonts.getWidth() * 2 < line_feed_width) {
      x += SDfonts.getWidth() + 1;
    } else {
      x = x0;
      if (y + SDfonts.getHeight() * 2 < TFT_WIDTH) {
        y += SDfonts.getHeight() + 2;
      } else {
        y = y0;
        tft->fillScreen(TFT_BLACK);
      }
    }
  }
  SDfonts.close();
}

uint16_t Font::getFontWidth(const char* str) {
  uint16_t width = 0;
  uint8_t buf[MAXFONTLEN];
  char* pUTF8 = const_cast<char*>(str);

  SDfonts.open();
  while ((pUTF8 = SDfonts.getFontData(buf, pUTF8))) {
    if (width + SDfonts.getWidth() * 2 < line_feed_width) {
      width += SDfonts.getWidth() + 1;
    } else {
      return line_feed_width;
    }
  }
  SDfonts.close();

  return width;
}

uint16_t Font::utf8count(std::string str) {
  uint16_t count = 0;
  uint8_t c_count;

  for (std::string::iterator it = str.begin(); it != str.end(); it += c_count) {
    auto c = *it;
    if (c < 0x80) {
      c_count = 1;
    } else if (c < 0xE0) {
      c_count = 2;
    } else if (c < 0xF0) {
      c_count = 3;
    } else {
      c_count = 4;
    }
    count++;
  }

  return count;
}

void Font::setFontSize(uint8_t size) { SDfonts.setFontSize(size); }

void Font::setTextColor(uint16_t color) { text_color = color; }

void Font::setBackgroundColor(uint16_t color) { background_color = color; }

void Font::setLineFeedWidth(uint16_t width) { line_feed_width = width; }

void Font::pushString(TFT_eSPI* tft, uint16_t poX, uint16_t poY,
                      const String& str) {
  int16_t len = str.length() + 2;
  auto buffer = new char[len];
  str.toCharArray(buffer, len);
  mprint(tft, poX, poY, buffer, text_color, background_color);
  delete[] buffer;
}

void Font::pushCentreString(TFT_eSPI* tft, uint16_t dX, uint16_t poY,
                            const String& str) {
  auto width = getFontWidth(str.c_str());
  pushString(tft, dX - width / 2, poY, str);
}

void Mutex::lock() noexcept {
  while (locked.exchange(true)) {
    delay(1);
  }
}

void Mutex::unlock() noexcept {
  locked.store(false, std::memory_order_release);
}
