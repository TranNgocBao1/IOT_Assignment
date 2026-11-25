// =============================================================================
// app_state.h — Trạng thái dùng chung của toàn bộ Node M
//
// Mọi module (sensor, fan, espnow, coreiot, web, ota) đọc/ghi qua g_state thay
// vì gọi chéo nhau. Nhờ vậy 2 người có thể viết 2 module song song: chỉ cần
// thống nhất các trường trong struct này (xem docs/interfaces.md).
// =============================================================================
#pragma once
#include <Arduino.h>
#include <math.h>
#include "config.h"

// Ai ra lệnh bật/tắt quạt — dùng để ghi log và đưa vào báo cáo.
enum class FanSource : uint8_t {
  NONE,
  RPC_CLOUD,   // RPC setFan từ CoreIOT (nút dashboard hoặc RuleChain)
  WEB_LOCAL,   // trang web trên board (/api/fan)
  GATEWAY,     // gateway Python gọi /api/fan
  BOOT         // trạng thái lúc khởi động
};

struct Thresholds {
  int co2On    = DEFAULT_CO2_ON;
  int co2Off   = DEFAULT_CO2_OFF;
  int co2Alarm = DEFAULT_CO2_ALARM;
};

struct AppState {
  // --- Cảm biến (M1) ---
  float co2         = NAN;
  float temperature = NAN;
  float humidity    = NAN;
  bool  sensorValid = false;

  // --- ESP-NOW (M2) ---
  bool     windowOpen   = false;
  bool     windowLinkOk = false;   // còn nhận được gói từ Node S không
  uint32_t espnowRx     = 0;       // số gói nhận được
  uint32_t espnowLost   = 0;       // số gói bị mất (tính từ seq)

  // --- Quạt (M1, M4, M5) ---
  bool      fanOn     = false;
  FanSource fanSource = FanSource::BOOT;

  // --- Cloud (M4) ---
  Thresholds th;
  bool mqttConnected = false;
};

extern AppState g_state;

// Trả về "good" / "fair" / "poor" theo CO2 (hoặc "unknown" nếu chưa có số).
const char* airLevel(float co2);

// Tên nguồn lệnh quạt, để in log.
const char* fanSourceName(FanSource s);
