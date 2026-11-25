// =============================================================================
// 01_filter_fan_on.js — Node "Script" (Filter) trong RuleChain (module M5, Bạn B)
// Trả true khi cần BẬT quạt.
//
// Trước node này đặt node "Originator attributes" lấy shared attributes
// co2_on, co2_off, co2_alarm vào metadata (ThingsBoard thêm tiền tố "shared_").
//
// Thuật toán hysteresis (trễ 2 ngưỡng):
//   BẬT  khi co2 > co2_on  VÀ cửa đóng VÀ quạt đang tắt
//   TẮT  khi co2 < co2_off VÀ quạt đang bật      (xem 02_filter_fan_off.js)
//   Giữa 2 ngưỡng (800..1000) giữ nguyên => quạt không bật tắt liên tục.
// Kiểm tra "quạt đang tắt" dựa vào msg.fan_state do Node M gửi kèm (RuleChain không nhớ trạng thái).
//
// Ghi chú: CoreIOT có thể dùng TBEL hoặc JavaScript cho script node; cú pháp
// dưới đây chạy được ở cả hai với các phép so sánh đơn giản.
// =============================================================================
var co2On = metadata.shared_co2_on ? parseFloat(metadata.shared_co2_on) : 1000;

// TODO[M5-B]:
//  - nếu msg.co2 không có (undefined/null) -> return false
//  - var windowClosed = (msg.window_open == false || msg.window_open == "false");
//  - var fanOff       = (msg.fan_state  == false || msg.fan_state  == "false");
//  - return msg.co2 > co2On && windowClosed && fanOff;
return false;
