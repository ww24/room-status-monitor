// Copyright (c) 2023 Takenori Nakagawa <ww24gm+oss@gmail.com>

#include <M5Stack.h>

#include "../include/App.hpp"
#include "../include/Display.hpp"

auto display = m5util::Display(&M5.Lcd);
auto ble = app::BLE(&display);

void setup() {
  M5.begin();
  M5.Lcd.setBrightness(80);
  M5.Lcd.fillScreen(TFT_BLACK);

  Serial.begin(9600);
  Serial.println("\nstarted");

  M5.Speaker.begin();
  M5.Speaker.setVolume(1);

  display.begin();

  ble.begin();
  Serial.println("ble gatt server started");
}

void loop() {
  M5.update();

  if (M5.BtnA.wasPressed()) {
    ble.cancel();
  } else if (M5.BtnB.wasPressed()) {
    ble.call();
  } else if (M5.BtnC.wasPressed()) {
    ble.yo();
  }

  display.update();

  delay(100);
}
