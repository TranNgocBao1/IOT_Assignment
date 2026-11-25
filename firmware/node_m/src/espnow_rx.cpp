// =============================================================================
// espnow_rx.cpp — Module M2, phần của Bạn B (phía Node M)
//
// Thuật toán dùng trong file:
//  1. Mô hình "callback chỉ lưu, loop mới xử lý" (producer/consumer 1 phần tử):
//     callback ESP-NOW chạy trong task WiFi, không được gọi MQTT/Serial nặng.
//     Callback chép gói vào s_pending + bật cờ s_hasPending trong vùng găng
//     (portMUX spinlock); loop() lấy ra và xử lý.
//  2. Đếm mất gói bằng số thứ tự (sequence number):
//       seq == last+1          -> bình thường
//       seq >  last+1          -> mất (seq - last - 1) gói
//       seq <= last            -> Node S khởi động lại (seq về 0) => đặt lại mốc
//     Tỉ lệ mất = lost / (rx + lost) * 100%.
//  3. Phát hiện mất liên lạc bằng timeout (watchdog phần mềm): quá
//     ESPNOW_TIMEOUT_MS không có gói thì windowLinkOk = false.
// =============================================================================
#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include "espnow_rx.h"
#include "espnow_packet.h"
#include "app_state.h"
#include "config.h"

static portMUX_TYPE   s_mux = portMUX_INITIALIZER_UNLOCKED;
static volatile bool  s_hasPending = false;
static WindowPacket   s_pending;
static uint32_t       s_lastPacketMs = 0;
static bool           s_haveLastSeq  = false;
static uint32_t       s_lastSeq      = 0;

// Callback nhận. Chữ ký khác nhau giữa arduino-esp32 v2 và v3.
#if ESP_ARDUINO_VERSION_MAJOR >= 3
static void onRecv(const esp_now_recv_info_t* info, const uint8_t* data, int len) {
#else
static void onRecv(const uint8_t* mac, const uint8_t* data, int len) {
#endif
  if (len != sizeof(WindowPacket)) return;     // sai kích thước -> bỏ
  // TODO[M2-B]: trong vùng găng:
  //   portENTER_CRITICAL(&s_mux);
  //   memcpy(&s_pending, data, sizeof(WindowPacket));
  //   s_hasPending = true;
  //   portEXIT_CRITICAL(&s_mux);
  // KHÔNG Serial.print / publish MQTT ở đây.
}

bool espnowBegin() {
  Serial.printf("[ESPNOW] MAC Node M: %s  kenh WiFi: %d\n",
                WiFi.macAddress().c_str(), WiFi.channel());
  // In MAC này ra để điền vào NODE_M_MAC trong firmware Node S.

  // TODO[M2-B]:
  //  1. esp_now_init() — lỗi thì return false
  //  2. esp_now_register_recv_cb(onRecv)
  //  Node M chỉ NHẬN nên không cần add peer.
  return false;
}

// Cập nhật bộ đếm mất gói theo seq mới nhận.
static void updateLossCounter(uint32_t seq) {
  // TODO[M2-B]: cài thuật toán số 2 ở đầu file:
  //   if (!s_haveLastSeq || seq <= s_lastSeq) { đặt mốc mới }
  //   else if (seq > s_lastSeq + 1) g_state.espnowLost += seq - s_lastSeq - 1;
  //   s_lastSeq = seq; s_haveLastSeq = true; g_state.espnowRx++;
}

void espnowLoop() {
  // 1) Lấy gói đang chờ (nếu có)
  if (s_hasPending) {
    WindowPacket pkt;
    portENTER_CRITICAL(&s_mux);
    pkt = s_pending;
    s_hasPending = false;
    portEXIT_CRITICAL(&s_mux);

    if (pkt.version == ESPNOW_PKT_VERSION) {
      updateLossCounter(pkt.seq);
      // TODO[M2-B]: g_state.windowOpen = pkt.window_open; windowLinkOk = true;
      //             s_lastPacketMs = millis(); in log khi trạng thái cửa ĐỔI.
    }
  }

  // 2) Kiểm tra timeout
  // TODO[M2-B]: nếu g_state.windowLinkOk && millis() - s_lastPacketMs > ESPNOW_TIMEOUT_MS
  //   -> windowLinkOk = false, in cảnh báo.
}

float espnowLossPercent() {
  uint32_t total = g_state.espnowRx + g_state.espnowLost;
  if (total == 0) return 0;
  return 100.0f * g_state.espnowLost / total;
}
