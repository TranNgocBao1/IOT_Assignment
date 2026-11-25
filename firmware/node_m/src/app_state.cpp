// =============================================================================
// app_state.cpp — Định nghĩa biến trạng thái toàn cục và hàm tiện ích nhỏ
// =============================================================================
#include "app_state.h"

AppState g_state;

// Thuật toán phân mức: so sánh với 2 ngưỡng cố định.
//   co2 <  LEVEL_FAIR_FROM               -> good
//   LEVEL_FAIR_FROM <= co2 <= POOR_FROM  -> fair
//   co2 >  LEVEL_POOR_FROM               -> poor
const char* airLevel(float co2) {
  if (isnan(co2)) return "unknown";
  // TODO[M3-A]: hoàn thiện 3 nhánh good / fair / poor theo docs/interfaces.md
  return "unknown";
}

const char* fanSourceName(FanSource s) {
  switch (s) {
    case FanSource::RPC_CLOUD: return "rpc_cloud";
    case FanSource::WEB_LOCAL: return "web_local";
    case FanSource::GATEWAY:   return "gateway";
    case FanSource::BOOT:      return "boot";
    default:                   return "none";
  }
}
