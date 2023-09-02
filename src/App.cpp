// Copyright (c) 2023 Takenori Nakagawa <ww24gm+oss@gmail.com>

#include "../include/App.hpp"

#include "../include/Display.hpp"

#define BLE_DEVICE_NAME "RoomStatusMonitor"
#define BLE_GATT_SERVICE_UUID "d3e6a1bb-2f35-4853-9f02-ba02b91044f1"
#define BLE_CHARACTERISTIC_STATUS_UUID "e9229875-87d4-4b24-b703-361649ad9ad6"
#define BLE_CHARACTERISTIC_CONTENT_UUID "c310ce58-d663-4dcd-9e37-8a5431f6550d"
#define BLE_CHARACTERISTIC_CALL_UUID "5a8e96e4-f950-4512-9a50-1008c6629d43"

#define MSG_BLE_WAIT_CONNECTING "Bluetooth 接続待機中"
#define MSG_BLE_CONNECTED "Bluetooth 接続完了！"
#define MSG_CALLING "呼出中…"

using app::BLE;
using app::CharacteristicCallbacks;
using app::ServerCallbacks;

CharacteristicCallbacks::CharacteristicCallbacks(
    std::function<void(BLECharacteristic *)> onWriteFn)
    : BLECharacteristicCallbacks(), onWriteFn(onWriteFn) {}

void CharacteristicCallbacks::onRead(BLECharacteristic *pCharacteristic) {
  auto uuid = pCharacteristic->getUUID().toString();
  Serial.printf("read characteristic by central: %s\n", uuid.c_str());
}

void CharacteristicCallbacks::onWrite(BLECharacteristic *pCharacteristic) {
  auto uuid = pCharacteristic->getUUID().toString();
  Serial.printf("write characteristic by central: %s\n", uuid.c_str());
  onWriteFn(pCharacteristic);
}

ServerCallbacks::ServerCallbacks(m5util::Display *display, BLE *ble)
    : BLEServerCallbacks(), display(display), ble(ble) {}

void ServerCallbacks::onConnect(BLEServer *pServer) {
  Serial.println("ble connected");
  ble->beep(100);

  ble->setStatus(MSG_BLE_CONNECTED);
  display->setButtons("取消", "呼出", "Yo");
}

void ServerCallbacks::onDisconnect(BLEServer *pServer) {
  Serial.println("ble disconnected");
  ble->setStatus(MSG_BLE_WAIT_CONNECTING);
  display->setButtons("", "", "");
  pServer->startAdvertising();
}

BLE::BLE(m5util::Display *display) : display(display) {}

BLE::~BLE() {}

void BLE::setStatus(std::string status) {
  pCharacteristicStatus->setValue(status);
  pCharacteristicStatus->notify(true);
  display->setStatus(status);
}

void BLE::setContent(std::string content) {
  pCharacteristicContent->setValue(content);
  display->setContent(content);
}

void BLE::begin(void) {
  // create GATT server
  BLEDevice::init(BLE_DEVICE_NAME);
  BLEDevice::setPower(ESP_PWR_LVL_P9, ESP_BLE_PWR_TYPE_DEFAULT);
  BLEDevice::setPower(ESP_PWR_LVL_P9, ESP_BLE_PWR_TYPE_ADV);
  BLEDevice::setPower(ESP_PWR_LVL_P9, ESP_BLE_PWR_TYPE_SCAN);
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new ServerCallbacks(display, this));

  // create GATT service
  pService = pServer->createService(BLE_GATT_SERVICE_UUID);

  // create status characteristic
  pCharacteristicStatus = pService->createCharacteristic(
      BLE_CHARACTERISTIC_STATUS_UUID, BLECharacteristic::PROPERTY_READ |
                                          BLECharacteristic::PROPERTY_WRITE |
                                          BLECharacteristic::PROPERTY_NOTIFY);
  pCharacteristicStatus->addDescriptor(new BLE2902());
  auto disp = display;
  auto statusCallback =
      new CharacteristicCallbacks([disp](BLECharacteristic *pCharacteristic) {
        std::string val = pCharacteristic->getValue();
        Serial.printf("change status: %s\n", val.c_str());
        disp->setStatus(val.c_str());
      });
  pCharacteristicStatus->setCallbacks(statusCallback);
  pCharacteristicStatus->setValue("");

  // create content characteristic
  pCharacteristicContent = pService->createCharacteristic(
      BLE_CHARACTERISTIC_CONTENT_UUID,
      BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE);
  auto contentCallback = new CharacteristicCallbacks(
      [disp, this](BLECharacteristic *pCharacteristic) {
        std::string val = pCharacteristic->getValue();
        Serial.printf("write content: %s\n", val.c_str());
        disp->setContent(val.c_str());

        delay(3000);
        Serial.println(std::difftime(std::time(0), last_canceled));
        if (std::difftime(std::time(0), last_canceled) > 3) {
          cancel();
        }
      });
  pCharacteristicContent->setCallbacks(contentCallback);
  pCharacteristicContent->setValue("");

  // create call characteristic
  pCharacteristicCall = pService->createCharacteristic(
      BLE_CHARACTERISTIC_CALL_UUID,
      BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);
  pCharacteristicCall->addDescriptor(new BLE2902());
  pCharacteristicCall->setValue(NONE);

  pService->start();
  BLEAdvertising *advertising = pServer->getAdvertising();
  advertising->addServiceUUID(pService->getUUID());
  advertising->start();

  setStatus(MSG_BLE_WAIT_CONNECTING);
}

void BLE::beep(uint16_t duration) {
  M5.Speaker.beep();
  delay(duration);
  M5.Speaker.mute();
}

void BLE::yo(void) {
  auto status = pCharacteristicStatus->getValue();
  if (status == MSG_BLE_WAIT_CONNECTING) {
    return;
  }

  beep(100);

  if (pCharacteristicCall->getValue() == NONE) {
    last_status = status;
  }

  pCharacteristicCall->setValue(YO);
  pCharacteristicCall->notify(true);

  setStatus(MSG_CALLING);
}

void BLE::call(void) {
  auto status = pCharacteristicStatus->getValue();
  if (status == MSG_BLE_WAIT_CONNECTING) {
    return;
  }

  beep(100);

  if (pCharacteristicCall->getValue() == NONE) {
    last_status = pCharacteristicStatus->getValue();
  }

  pCharacteristicCall->setValue(CALLING);
  pCharacteristicCall->notify(true);

  setStatus(MSG_CALLING);
}

void BLE::cancel(void) {
  auto status = pCharacteristicStatus->getValue();
  if (status != MSG_CALLING) {
    return;
  }

  beep(100);

  last_canceled = std::time(0);

  pCharacteristicCall->setValue(NONE);
  pCharacteristicCall->notify(true);

  if (last_status != "") {
    setStatus(last_status);
  }
  setContent("");
}
