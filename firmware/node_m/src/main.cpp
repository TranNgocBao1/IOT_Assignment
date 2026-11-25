// =============================================================================
// main.cpp — Điểm vào firmware Node M (YoloUNO ESP32-S3)
//
// Node M làm gì:
//   đọc cảm biến (M1) -> nhận trạng thái cửa từ Node S qua ESP-NOW (M2)
//   -> phục vụ web cục bộ (M3) -> gửi/nhận CoreIOT qua MQTT (M4, M5) -> OTA (M6)
//
// Thuật toán: lập lịch hợp tác (cooperative scheduling) trong loop().
//   Mỗi module có hàm xxxLoop() ngắn, KHÔNG dùng delay() dài; mỗi hàm tự kiểm
//   tra millis() xem đến lượt mình chưa. Nhờ vậy MQTT, web, ESP-NOW, OTA chạy
//   "đồng thời" trên 1 lõi mà không cần FreeRTOS task riêng.
//
// Thứ tự khởi tạo QUAN TRỌNG:
//   WiFi STA kết nối xong rồi mới esp_now_init(), vì ESP-NOW phải dùng đúng
//   kênh của router. Node S phải đặt cùng kênh đó.
// =============================================================================
#include <Arduino.h>
#include <WiFi.h>
#include "config.h"
#include "secrets.h"
#include "app_state.h"
#include "sensor.h"
#include "fan.h"
#include "espnow_rx.h"
#include "web_server.h"
#include "coreiot.h"
#include "ota.h"

static bool connectWiFi(uint32_t timeoutMs) {
  WiFi.mode(WIFI_STA);   // STA để vừa vào router vừa dùng ESP-NOW
  WiFi.setSleep(false);  // tắt modem sleep: ESP-NOW nhận ổn định hơn
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.printf("[WIFI] ket noi %s", WIFI_SSID);
  uint32_t start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < timeoutMs) {
    delay(300);
    Serial.print('.');
  }
  Serial.println();
  if (WiFi.status() != WL_CONNECTED) return false;
  Serial.printf("[WIFI] IP %s  kenh %d  RSSI %d\n",
                WiFi.localIP().toString().c_str(), WiFi.channel(), WiFi.RSSI());
  return true;
}

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.printf("\n=== Node M %s %s ===\n", FW_TITLE, FW_VERSION);

  fanBegin();                       // quạt TẮT ngay khi khởi động
  if (!sensorBegin()) Serial.println(F("[SENSOR] khong tim thay cam bien!"));

  if (!connectWiFi(20000)) {
    Serial.println(F("[WIFI] that bai, se thu lai trong loop()"));
  }
  if (!espnowBegin()) Serial.println(F("[ESPNOW] khoi tao loi"));

  webBegin();
  coreiotBegin();
  otaBegin();
}

void loop() {
  // TODO[M4-A]: tự kết nối lại WiFi nếu rớt (WiFi.reconnect() mỗi WIFI_RETRY_MS,
  //             không chặn). Lưu ý: kênh có thể đổi nếu router đổi kênh -> cố định kênh router.

  sensorLoop();    // M1
  espnowLoop();    // M2
  webLoop();       // M3
  coreiotLoop();   // M4, M5
  otaLoop();       // M6

  delay(2);        // nhường CPU cho task WiFi
}
