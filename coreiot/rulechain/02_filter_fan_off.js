// =============================================================================
// 02_filter_fan_off.js — Node "Script" (Filter): trả true khi cần TẮT quạt.
// (Nhánh thứ hai của hysteresis, xem 01_filter_fan_on.js)
// =============================================================================
var co2Off = metadata.shared_co2_off ? parseFloat(metadata.shared_co2_off) : 800;

// TODO[M5-B]:
//  - var fanOn = (msg.fan_state == true || msg.fan_state == "true");
//  - return msg.co2 != null && msg.co2 < co2Off && fanOn;
//  Câu hỏi để nhóm tự quyết: khi MỞ CỬA lúc quạt đang chạy có nên tắt quạt
//  ngay không? Nếu có thì thêm điều kiện "|| (fanOn && msg.window_open == true)".
return false;
