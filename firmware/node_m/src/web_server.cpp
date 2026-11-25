// =============================================================================
// web_server.cpp — Module M3, phần của Bạn A (backend)
//
// API (khớp docs/interfaces.md):
//   GET /            -> trang HTML (web_page.h, Bạn B viết)
//   GET /api/data    -> JSON {co2, temperature, humidity, window_open, fan_state,
//                             fw_version, level, ...}
//   GET /api/fan?on=1|0[&src=gateway] -> bật/tắt quạt, trả JSON {ok, fan_state}
//
// Thuật toán / kỹ thuật dùng trong file:
//  - Định tuyến HTTP (routing) theo đường dẫn bằng WebServer::on().
//  - Tuần tự hóa JSON (serialization) bằng ArduinoJson.
//  - Kiểm tra tham số đầu vào (input validation): chỉ chấp nhận on=0 hoặc 1,
//    sai thì trả HTTP 400.
//  - Header CORS "Access-Control-Allow-Origin: *" để trang/ứng dụng khác gọi được.
// =============================================================================
#include <WebServer.h>
#include <ArduinoJson.h>
#include "web_server.h"
#include "web_page.h"
#include "config.h"
#include "app_state.h"
#include "fan.h"
#include "espnow_rx.h"

static WebServer s_server(WEB_PORT);

static void sendJson(int code, JsonDocument& doc) {
  String out;
  serializeJson(doc, out);
  s_server.sendHeader("Access-Control-Allow-Origin", "*");
  s_server.send(code, "application/json", out);
}

static void handleRoot() {
  s_server.send_P(200, "text/html; charset=utf-8", INDEX_HTML);
}

static void handleData() {
  JsonDocument doc;
  // TODO[M3-A]: điền đủ các trường theo docs/interfaces.md
  //   doc["co2"] = g_state.sensorValid ? g_state.co2 : nullptr;   (nullptr -> JSON null)
  //   doc["temperature"], doc["humidity"], doc["window_open"], doc["fan_state"]
  //   doc["fw_version"] = FW_VERSION;  doc["level"] = airLevel(g_state.co2);
  // Trường chẩn đoán (tùy chọn): window_link, espnow_loss_pct, mqtt, uptime_s
  sendJson(200, doc);
}

static void handleFan() {
  JsonDocument doc;
  // TODO[M3-A]:
  //  1. if (!s_server.hasArg("on")) -> doc["error"]="missing on"; sendJson(400, doc); return;
  //  2. String v = s_server.arg("on"); chỉ nhận "1" hoặc "0", sai -> 400
  //  3. FanSource src = (s_server.arg("src") == "gateway") ? FanSource::GATEWAY : FanSource::WEB_LOCAL;
  //  4. bool ok = fanSet(v == "1", src);
  //  5. doc["ok"] = ok; doc["fan_state"] = g_state.fanOn; sendJson(ok ? 200 : 409, doc);
  //     (409 = bị từ chối do đổi trạng thái quá nhanh)
  sendJson(501, doc);
}

static void handleNotFound() {
  s_server.send(404, "text/plain", "Not found");
}

void webBegin() {
  s_server.on("/", HTTP_GET, handleRoot);
  s_server.on("/api/data", HTTP_GET, handleData);
  s_server.on("/api/fan", HTTP_GET, handleFan);
  s_server.onNotFound(handleNotFound);
  s_server.begin();
  Serial.printf("[WEB] http://%s/\n", WiFi.localIP().toString().c_str());
}

void webLoop() { s_server.handleClient(); }
