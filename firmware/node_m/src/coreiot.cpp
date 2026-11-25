// =============================================================================
// coreiot.cpp — Module M4, phần của Bạn A (firmware MQTT)
//
// Thuật toán dùng trong file:
//  1. Kết nối lại với backoff lũy thừa (exponential backoff): mỗi lần thất bại
//     thời gian chờ nhân đôi (1 s, 2 s, 4 s ... tối đa 30 s), thành công thì
//     đặt lại. Tránh dội server và tránh chặn loop() bằng vòng while.
//  2. Định tuyến tin nhắn theo topic (topic dispatch): một callback duy nhất,
//     so khớp tiền tố topic để gọi handler RPC / attribute / firmware chunk.
//  3. Tách requestId từ cuối topic (ThingsBoard trả lời RPC phải dùng đúng id).
//  4. Gửi telemetry định kỳ bằng bộ hẹn giờ millis() (non-blocking).
//  5. fan_state gửi KÈM mọi gói telemetry vì RuleChain không nhớ trạng thái
//     (stateless) — RuleChain cần biết quạt đang bật hay tắt để áp hysteresis.
// =============================================================================
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "coreiot.h"
#include "config.h"
#include "secrets.h"
#include "app_state.h"
#include "fan.h"
#include "ota.h"
#include "espnow_rx.h"

static WiFiClient   s_net;
static PubSubClient s_mqtt(s_net);
static uint32_t     s_lastTelemetryMs = 0;
static uint32_t     s_nextConnectMs   = 0;
static uint32_t     s_backoffMs       = MQTT_BACKOFF_MIN_MS;

// ----------------------------------------------------------------------------
// Tiện ích
// ----------------------------------------------------------------------------
// Lấy phần sau dấu '/' cuối cùng của topic, vd ".../rpc/request/42" -> "42"
static String lastTopicSegment(const char* topic) {
  const char* p = strrchr(topic, '/');
  return p ? String(p + 1) : String();
}

bool coreiotPublish(const char* topic, const char* payload) {
  if (!s_mqtt.connected()) return false;
  return s_mqtt.publish(topic, payload);
}

bool coreiotPublishRaw(const char* topic, const uint8_t* data, size_t len) {
  if (!s_mqtt.connected()) return false;
  return s_mqtt.publish(topic, data, len);
}

bool coreiotSendTelemetryString(const char* key, const char* value) {
  JsonDocument doc;
  doc[key] = value;
  char buf[128];
  serializeJson(doc, buf, sizeof(buf));
  return coreiotPublish(TOPIC_TELEMETRY, buf);
}

bool coreiotConnected() { return s_mqtt.connected(); }

// ----------------------------------------------------------------------------
// Handler: RPC từ server (nút dashboard, RuleChain)
// payload ví dụ: {"method":"setFan","params":true}
// ----------------------------------------------------------------------------
static void handleRpc(const char* topic, const uint8_t* payload, unsigned int len) {
  String reqId = lastTopicSegment(topic);
  JsonDocument req;
  if (deserializeJson(req, payload, len)) {
    Serial.println(F("[MQTT] RPC JSON loi"));
    return;
  }
  const char* method = req["method"] | "";
  JsonDocument resp;

  if (strcmp(method, "setFan") == 0) {
    // TODO[M4-A]:
    //  - đọc params: có thể là bool (true/false) hoặc object {"on":true} tùy cách
    //    cấu hình widget; hỗ trợ cả 2 cho chắc.
    //  - bool ok = fanSet(on, FanSource::RPC_CLOUD);
    //  - resp["ok"] = ok; resp["fan_state"] = g_state.fanOn;
  } else if (strcmp(method, "getFan") == 0) {
    // Widget nút bật/tắt của ThingsBoard thường gọi getValue để lấy trạng thái.
    // TODO[M4-B]: kiểm tra tên method widget dùng (xem cấu hình widget), trả về g_state.fanOn
  } else {
    resp["error"] = "unknown method";
  }

  String out;
  serializeJson(resp, out);
  String respTopic = String(TOPIC_RPC_RESP_PREFIX) + reqId;
  coreiotPublish(respTopic.c_str(), out.c_str());

  // Gửi luôn fan_state mới để dashboard và RuleChain cập nhật ngay.
  s_lastTelemetryMs = 0;
}

// ----------------------------------------------------------------------------
// Handler: shared attributes (ngưỡng + thông tin firmware OTA)
// - Cập nhật đẩy xuống:  topic "v1/devices/me/attributes", JSON phẳng {"co2_on":1100}
// - Trả lời khi xin:     topic ".../attributes/response/1", JSON {"shared":{...}}
// ----------------------------------------------------------------------------
static void applySharedAttributes(JsonObjectConst shared) {
  // TODO[M5-A]: nếu có key thì cập nhật:
  //   if (shared["co2_on"].is<int>())    g_state.th.co2On    = shared["co2_on"];
  //   ... co2_off, co2_alarm
  //   Kiểm tra hợp lệ: co2Off < co2On < co2Alarm, sai thì giữ giá trị cũ và in cảnh báo.

  // Chuyển thông tin firmware cho module OTA (nếu có fw_title / fw_version)
  if (shared["fw_title"].is<const char*>() || shared["fw_version"].is<const char*>()) {
    otaOnFirmwareInfo(shared);
  }
}

static void handleAttributes(const char* topic, const uint8_t* payload, unsigned int len) {
  JsonDocument doc;
  if (deserializeJson(doc, payload, len)) return;
  if (doc["shared"].is<JsonObjectConst>()) {
    applySharedAttributes(doc["shared"].as<JsonObjectConst>());   // dạng response
  } else {
    applySharedAttributes(doc.as<JsonObjectConst>());             // dạng cập nhật phẳng
  }
}

// ----------------------------------------------------------------------------
// Callback chung — định tuyến theo topic
// ----------------------------------------------------------------------------
static void onMqttMessage(char* topic, byte* payload, unsigned int len) {
  if (strncmp(topic, "v1/devices/me/rpc/request/", 26) == 0) {
    handleRpc(topic, payload, len);
  } else if (strncmp(topic, "v1/devices/me/attributes", 24) == 0) {
    handleAttributes(topic, payload, len);
  } else if (strncmp(topic, "v2/fw/response/", 15) == 0) {
    // topic: v2/fw/response/{requestId}/chunk/{chunkIndex}
    int chunkIndex = lastTopicSegment(topic).toInt();
    otaOnChunk(chunkIndex, payload, len);
  } else {
    Serial.printf("[MQTT] topic la: %s\n", topic);
  }
}

// ----------------------------------------------------------------------------
// Kết nối
// ----------------------------------------------------------------------------
static void requestSharedAttributes() {
  // Xin toàn bộ shared attribute cần dùng ngay sau khi kết nối.
  const char* payload =
      "{\"sharedKeys\":\"co2_on,co2_off,co2_alarm,"
      "fw_title,fw_version,fw_size,fw_checksum,fw_checksum_algorithm\"}";
  coreiotPublish(TOPIC_ATTR_REQ_PREFIX "1", payload);
}

static bool tryConnect() {
  String clientId = String("air-node-m-") + String((uint32_t)ESP.getEfuseMac(), HEX);
  Serial.printf("[MQTT] ket noi %s:%d ...\n", COREIOT_HOST, COREIOT_PORT);
  // ThingsBoard: username = access token, password để trống.
  if (!s_mqtt.connect(clientId.c_str(), COREIOT_TOKEN, nullptr)) {
    Serial.printf("[MQTT] that bai, state=%d\n", s_mqtt.state());
    return false;
  }
  Serial.println(F("[MQTT] da ket noi"));
  // TODO[M4-A]: subscribe TOPIC_RPC_REQ_SUB, TOPIC_ATTR, TOPIC_ATTR_RESP_SUB, TOPIC_FW_RESP_SUB
  requestSharedAttributes();
  otaOnConnected();   // báo current_fw_title / current_fw_version
  return true;
}

void coreiotBegin() {
  s_mqtt.setServer(COREIOT_HOST, COREIOT_PORT);
  s_mqtt.setCallback(onMqttMessage);
  s_mqtt.setBufferSize(MQTT_BUFFER_SIZE);   // đủ lớn cho 1 chunk OTA
  s_mqtt.setKeepAlive(30);
}

static void maintainConnection() {
  if (s_mqtt.connected() || WiFi.status() != WL_CONNECTED) return;
  uint32_t now = millis();
  if (now < s_nextConnectMs) return;

  if (tryConnect()) {
    s_backoffMs = MQTT_BACKOFF_MIN_MS;
  } else {
    // TODO[M4-A]: exponential backoff:
    //   s_nextConnectMs = now + s_backoffMs;
    //   s_backoffMs = min(s_backoffMs * 2, (uint32_t)MQTT_BACKOFF_MAX_MS);
    s_nextConnectMs = now + MQTT_BACKOFF_MIN_MS;
  }
}

// ----------------------------------------------------------------------------
// Telemetry
// ----------------------------------------------------------------------------
static void publishTelemetry() {
  JsonDocument doc;
  // TODO[M4-A]: điền đúng các key trong docs/interfaces.md:
  //   if (g_state.sensorValid) { doc["co2"] = ...; doc["temperature"] = ...; doc["humidity"] = ...; }
  //   doc["window_open"] = g_state.windowOpen;
  //   doc["fan_state"]   = g_state.fanOn;      // BẮT BUỘC cho RuleChain (M5)
  //   doc["fw_version"]  = FW_VERSION;
  // Tùy chọn (chẩn đoán, phục vụ bảng đo trong báo cáo):
  //   doc["window_link"] = g_state.windowLinkOk;
  //   doc["espnow_loss_pct"] = espnowLossPercent();
  //   doc["rssi"] = WiFi.RSSI();
  char buf[384];
  size_t n = serializeJson(doc, buf, sizeof(buf));
  if (n == 0) return;
  coreiotPublish(TOPIC_TELEMETRY, buf);
}

void coreiotLoop() {
  maintainConnection();
  g_state.mqttConnected = s_mqtt.connected();
  if (!s_mqtt.connected()) return;
  s_mqtt.loop();

  uint32_t now = millis();
  if (s_lastTelemetryMs == 0 || now - s_lastTelemetryMs >= TELEMETRY_INTERVAL_MS) {
    s_lastTelemetryMs = now;
    publishTelemetry();
  }
}
