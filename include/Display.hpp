// Copyright (c) 2023 Takenori Nakagawa <ww24gm+oss@gmail.com>

#ifndef INCLUDE_DISPLAY_HPP_
#define INCLUDE_DISPLAY_HPP_

#include <M5Stack.h>
#include <Preferences.h>

#include <atomic>
#include <string>

namespace m5util {

class Mutex {
 private:
  std::atomic<bool> locked{false};

 public:
  void lock() noexcept;
  void unlock() noexcept;
};

class Font {
 private:
  uint8_t font_size;
  uint16_t text_color;
  uint16_t background_color;
  uint16_t line_feed_width;
  void mputc(TFT_eSPI* tft, uint16_t x, uint16_t y, uint8_t* buf, uint16_t fg,
             uint16_t bg);
  void mprint(TFT_eSPI* tft, uint16_t x, uint16_t y, const char* str,
              uint16_t fg, uint16_t bg);
  uint16_t utf8count(std::string str);
  uint16_t getFontWidth(const char* str);

 public:
  Font(uint8_t font_size = 24, uint16_t text_color = TFT_WHITE,
       uint16_t background_color = TFT_BLACK,
       uint16_t line_feed_width = TFT_HEIGHT);
  void setFontSize(uint8_t size);
  void setTextColor(uint16_t color);
  void setBackgroundColor(uint16_t color);
  void setLineFeedWidth(uint16_t width);
  void pushString(TFT_eSPI* tft, uint16_t poX, uint16_t poY, const String& str);
  void pushCentreString(TFT_eSPI* tft, uint16_t dX, uint16_t poY,
                        const String& str);
};

class Display {
 private:
  TFT_eSprite sprite;
  uint8_t offset;
  uint16_t width;
  uint16_t height;
  Preferences prefs;
  const std::string pref_name = "monitor";
  Font font;
  Mutex mutex;

 public:
  Display(M5Display* display, uint8_t offset = 5, uint16_t width = TFT_HEIGHT,
          uint16_t height = TFT_WIDTH);
  ~Display();
  void begin(void);
  void setStatus(std::string status);
  void setContent(std::string content);
  void setButtons(std::string btnA, std::string btnB, std::string btnC);
  void update(void);
};

}  // namespace m5util

#endif  // INCLUDE_DISPLAY_HPP_
