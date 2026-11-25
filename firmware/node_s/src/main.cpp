// =============================================================================
// main.cpp — Firmware Node S: đọc công tắc từ cửa sổ, gửi qua ESP-NOW
// Module M1 (Bạn B: đọc công tắc) + M2 (Bạn A: gửi ESP-NOW)
//
// Thuật toán dùng trong file:
//  1. Chống dội phím (debounce) theo thời gian: chỉ chấp nhận trạng thái mới khi
//     nó giữ ổn định liên tục DEBOUNCE_MS. Tiếp điểm cơ khí khi đóng/mở có thể
//     nảy hàng chục lần trong vài ms.
//  2. Gửi theo sự kiện + nhịp tim (event-driven + heartbeat): gửi ngay khi cửa
//     đổi trạng thái, và gửi lại mỗi HEARTBEAT_MS để Node M biết Node S còn sống
//     và để đo tỉ lệ mất gói.
//  3. Số thứ tự tăng dần (seq) đóng dấu mỗi gói để Node M đếm gói mất.
//  4. Ép kênh WiFi (channel pinning): Node S không vào router nên phải tự đặt
//     kênh trùng kênh router mà Node M đang dùng, nếu không sẽ không nhận được.
// =============================================================================
#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include "config.h"
#include "espnow_packet.h"

static uint32_t s_seq          = 0;
static bool     s_stableOpen   = false;   // trạng thái đã qua debounce
static bool     s_lastRaw      = false;
static uint32_t s_rawChangedMs = 0;
static uint32_t s_lastSendMs   = 0;
static uint32_t s_sendOk = 0, s_sendFail = 0;

// Callback kết quả gửi (ACK tầng MAC). Chữ ký khác nhau giữa core v2 và v3.
#if ESP_ARDUINO_VERSION_MAJOR >= 3
static void onSent(const wifi_tx_info_t* info, esp_now_send_status_t status) {
#else
static void onSent(const uint8_t* mac, esp_now_send_status_t status) {
#endif
  if (status == ESP_NOW_SEND_SUCCESS) s_sendOk++; else s_sendFail++;
}

static bool readRawOpen() {
  // TODO[M1-B]: return digitalRead(PIN_REED) != REED_CLOSED_LEVEL;
  return false;
}

// Debounce: trả true nếu trạng thái ổn định vừa thay đổi
static bool updateDebounce() {
  bool raw = readRawOpen();
  uint32_t now = millis();
  // TODO[M1-B]:
  //   if (raw != s_lastRaw) { s_lastRaw = raw; s_rawChangedMs = now; }
  //   if (raw != s_stableOpen && now - s_rawChangedMs >= DEBOUNCE_MS) {
  //       s_stableOpen = raw; return true;
  //   }
  return false;
}

static void sendState() {
  WindowPacket pkt;
  pkt.version     = ESPNOW_PKT_VERSION;
  pkt.node_id     = NODE_ID;
  pkt.window_open = s_stableOpen ? 1 : 0;
  pkt.seq         = s_seq++;
  // TODO[M2-A]: esp_now_send(NODE_M_MAC, (uint8_t*)&pkt, sizeof(pkt));
  //             kiểm tra mã trả về, in log seq + trạng thái.
  s_lastSendMs = millis();
}

static bool setupEspNow() {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  // Ép kênh: bật promiscuous tạm để được phép đổi kênh, rồi tắt.
  // TODO[M2-A]:
  //   esp_wifi_set_promiscuous(true);
  //   esp_wifi_set_channel(ESPNOW_CHANNEL, WIFI_SECOND_CHAN_NONE);
  //   esp_wifi_set_promiscuous(false);
  // (Nâng cao, tùy chọn: quét WiFi.scanNetworks() tìm SSID router để tự lấy kênh.)

  // TODO[M2-A]:
  //   1. esp_now_init() — lỗi thì return false
  //   2. esp_now_register_send_cb(onSent)
  //   3. esp_now_peer_info_t peer = {}; memcpy(peer.peer_addr, NODE_M_MAC, 6);
  //      peer.channel = ESPNOW_CHANNEL; peer.encrypt = false;
  //      esp_now_add_peer(&peer)
  return false;
}

void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println(F("\n=== Node S (cua so) ==="));
  pinMode(PIN_REED, INPUT_PULLUP);
  s_lastRaw = s_stableOpen = readRawOpen();

  if (!setupEspNow()) Serial.println(F("[ESPNOW] khoi tao loi"));
  Serial.printf("[ESPNOW] MAC Node S: %s, kenh %d\n",
                WiFi.macAddress().c_str(), ESPNOW_CHANNEL);
  sendState();   // gửi trạng thái ban đầu
}

void loop() {
  if (updateDebounce()) {
    Serial.printf("[REED] cua %s\n", s_stableOpen ? "MO" : "DONG");
    sendState();                                   // gửi theo sự kiện
  } else if (millis() - s_lastSendMs >= HEARTBEAT_MS) {
    sendState();                                   // nhịp tim
  }
  // TODO[M2-A] (tùy chọn): mỗi 60 s in s_sendOk / s_sendFail để đưa vào báo cáo.
  delay(5);
}
