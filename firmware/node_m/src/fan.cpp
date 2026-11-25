// =============================================================================
// fan.cpp — Module M1, phần của Bạn B
//
// Thuật toán dùng trong file:
//  1. Đảo logic relay (active-low): nếu RELAY_ACTIVE_LOW thì "bật" = ghi LOW.
//  2. Chống đổi trạng thái liên tục (minimum switch interval / anti short-cycle):
//     từ chối lệnh nếu chưa đủ FAN_MIN_SWITCH_MS kể từ lần đổi trước. Bảo vệ
//     relay và quạt khi RuleChain, dashboard và web cùng gửi lệnh dồn dập.
//  3. Idempotent: lệnh trùng trạng thái hiện tại thì bỏ qua, coi như thành công.
// =============================================================================
#include <Arduino.h>
#include "fan.h"
#include "config.h"

static uint32_t s_lastSwitchMs = 0;
static bool     s_everSwitched = false;

static void writeRelay(bool on) {
  // TODO[M1-B]: ghi chân PIN_FAN_RELAY, có xét RELAY_ACTIVE_LOW
  //   level = RELAY_ACTIVE_LOW ? !on : on;  digitalWrite(PIN_FAN_RELAY, level ? HIGH : LOW);
}

void fanBegin() {
  pinMode(PIN_FAN_RELAY, OUTPUT);
  writeRelay(false);            // khởi động luôn ở trạng thái TẮT (an toàn)
  g_state.fanOn     = false;
  g_state.fanSource = FanSource::BOOT;
}

bool fanSet(bool on, FanSource source) {
  // 1) Lệnh trùng trạng thái -> không làm gì
  if (on == g_state.fanOn) return true;

  // 2) Chống bật tắt liên tục
  uint32_t now = millis();
  // TODO[M1-B]: nếu s_everSwitched && (now - s_lastSwitchMs < FAN_MIN_SWITCH_MS)
  //   -> in log "[FAN] tu choi, doi qua nhanh" và return false

  // 3) Thực hiện
  writeRelay(on);
  g_state.fanOn     = on;
  g_state.fanSource = source;
  s_lastSwitchMs    = now;
  s_everSwitched    = true;
  Serial.printf("[FAN] %s (nguon: %s)\n", on ? "BAT" : "TAT", fanSourceName(source));
  return true;
}

bool fanIsOn() { return g_state.fanOn; }
