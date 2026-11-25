// =============================================================================
// 04_filter_alarm.js — Điều kiện tạo / xóa cảnh báo CO2 cao (module M5, T13)
//
// Cách 1 (dễ, khuyến nghị): dùng "Alarm rules" trong Device profile:
//   Create: co2 > co2_alarm (giá trị lấy từ shared attribute co2_alarm)
//   Clear:  co2 < co2_alarm - 100   (có trễ để cảnh báo không nhấp nháy)
//   Rồi bật Notification (email / web) cho alarm đó.
//
// Cách 2: tự làm trong RuleChain bằng 2 filter dưới đây nối vào
//   node "Create alarm" và "Clear alarm" (alarm type: "High CO2").
// =============================================================================

// ---- Filter cho "Create alarm" ----
var co2Alarm = metadata.shared_co2_alarm ? parseFloat(metadata.shared_co2_alarm) : 1500;
// TODO[M5-B]: return msg.co2 != null && msg.co2 > co2Alarm;
return false;

// ---- Filter cho "Clear alarm" (chép sang 1 script node riêng) ----
// TODO[M5-B]: return msg.co2 != null && msg.co2 < co2Alarm - 100;
