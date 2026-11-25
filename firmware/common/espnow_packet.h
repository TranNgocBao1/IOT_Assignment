// =============================================================================
// espnow_packet.h — Cấu trúc gói ESP-NOW dùng CHUNG cho Node S (gửi) và Node M (nhận)
// Module: M2 (T06 ESP-NOW)
//
// File này được include bởi cả hai firmware (build_flags = -I../common).
// Nếu đổi cấu trúc gói thì PHẢI nạp lại cả hai board, và tăng ESPNOW_PKT_VERSION.
// =============================================================================
#pragma once
#include <stdint.h>

// Phiên bản định dạng gói. Node M bỏ qua gói có version khác (tránh đọc sai byte).
#define ESPNOW_PKT_VERSION 1

// __attribute__((packed)): không chèn byte đệm, để kích thước gói giống hệt nhau
// trên cả hai board (ESP32 và ESP32-S3) và không phụ thuộc trình biên dịch.
typedef struct __attribute__((packed)) {
  uint8_t  version;      // = ESPNOW_PKT_VERSION
  uint8_t  node_id;      // id của Node S (mở rộng nhiều cửa sổ sau này)
  uint8_t  window_open;  // 1 = cửa mở, 0 = cửa đóng
  uint32_t seq;          // số thứ tự tăng dần, dùng để đếm gói bị mất
} WindowPacket;

// Kiểm tra lúc biên dịch: gói phải nhỏ hơn giới hạn 250 byte của ESP-NOW.
static_assert(sizeof(WindowPacket) <= 250, "ESP-NOW payload toi da 250 byte");
