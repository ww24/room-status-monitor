// Copyright (c) 2023 Takenori Nakagawa <ww24gm+oss@gmail.com>

#ifndef INCLUDE_APP_HPP_
#define INCLUDE_APP_HPP_

#include <BLE2902.h>
#include <BLEDevice.h>
#include <BLEServer.h>

#include <ctime>
#include <string>

#include "./Display.hpp"

namespace app {
class BLE {
 private:
  m5util::Display *display;
  std::string last_status;
  BLEServer *pServer;
  BLEService *pService;
  BLECharacteristic *pCharacteristicStatus;
  BLECharacteristic *pCharacteristicContent;
  BLECharacteristic *pCharacteristicCall;
  std::time_t last_canceled;

 public:
  inline static const std::string CALLING = "calling";
  inline static const std::string YO = "yo";
  inline static const std::string NONE = "none";
  explicit BLE(m5util::Display *display);
  ~BLE();
  void setStatus(std::string status);
  void setContent(std::string content);
  void begin(void);
  void yo(void);
  void call(void);
  void cancel(void);
};

class CharacteristicCallbacks : public BLECharacteristicCallbacks {
 private:
  std::function<void(BLECharacteristic *)> onWriteFn;

 public:
  explicit CharacteristicCallbacks(
      std::function<void(BLECharacteristic *)> onWriteFn);
  void onRead(BLECharacteristic *pCharacteristic) override;
  void onWrite(BLECharacteristic *pCharacteristic) override;
};

class ServerCallbacks : public BLEServerCallbacks {
 private:
  m5util::Display *display;
  BLE *ble;

 public:
  ServerCallbacks(m5util::Display *display, BLE *ble);
  void onConnect(BLEServer *pServer) override;
  void onDisconnect(BLEServer *pServer) override;
};

}  // namespace app

#endif  // INCLUDE_APP_HPP_
