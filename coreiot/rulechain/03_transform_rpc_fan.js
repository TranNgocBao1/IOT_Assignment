// =============================================================================
// 03_transform_rpc_fan.js — Node "Script" (Transformation) biến telemetry thành
// lệnh RPC, đặt trước node "RPC call request".
//
// Dùng 2 bản: một bản params = true (sau nhánh BẬT), một bản params = false
// (sau nhánh TẮT). Node "RPC call request" yêu cầu msg có dạng
//   {"method": "...", "params": ...}
// =============================================================================
var newMsg = {
  method: "setFan",
  params: true      // TODO[M5-B]: bản cho nhánh TẮT đổi thành false
};
return { msg: newMsg, metadata: metadata, msgType: msgType };
