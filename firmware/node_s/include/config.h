// =============================================================================
// config.h — Cấu hình Node S
// =============================================================================
#pragma once

// TODO[M2-A]: chép MAC của Node M (in ra serial Node M lúc khởi động: "[ESPNOW] MAC Node M")
static const uint8_t NODE_M_MAC[6] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

// TODO[M2-A]: kênh WiFi của router (Node M in ra "kenh N"). Nên cố định kênh trên router.
#define ESPNOW_CHANNEL      6

#define NODE_ID             1

// TODO[M1-B]: chân nối công tắc từ (một đầu vào chân này, đầu kia xuống GND)
#define PIN_REED            4
// Công tắc từ thường đóng khi có nam châm (cửa đóng). INPUT_PULLUP:
//   nam châm gần -> mạch kín -> đọc LOW -> cửa ĐÓNG
#define REED_CLOSED_LEVEL   LOW

#define DEBOUNCE_MS         50     // chống dội tiếp điểm
#define HEARTBEAT_MS        5000   // gửi lại định kỳ dù cửa không đổi
