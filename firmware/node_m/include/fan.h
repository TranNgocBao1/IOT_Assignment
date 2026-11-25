// =============================================================================
// fan.h — Điều khiển relay quạt (module M1, dùng lại ở M3, M4, M5, M7)
// =============================================================================
#pragma once
#include "app_state.h"

void fanBegin();

// Yêu cầu bật/tắt quạt. Trả false nếu bị từ chối (đổi trạng thái quá nhanh).
// Mọi nơi muốn điều khiển quạt (RPC, web, gateway) đều phải gọi hàm này.
bool fanSet(bool on, FanSource source);

bool fanIsOn();
