// =============================================================================
// coreiot.h — Kết nối MQTT tới CoreIOT (ThingsBoard): telemetry, RPC, attributes
// Module M4 (T11), dùng lại ở M5 (T12/T13) và M6 (T14)
// =============================================================================
#pragma once
#include <Arduino.h>

// ---- Topic MQTT của ThingsBoard (device API) ----
#define TOPIC_TELEMETRY        "v1/devices/me/telemetry"
#define TOPIC_ATTR             "v1/devices/me/attributes"            // cập nhật shared attribute
#define TOPIC_ATTR_REQ_PREFIX  "v1/devices/me/attributes/request/"   // + requestId
#define TOPIC_ATTR_RESP_SUB    "v1/devices/me/attributes/response/+"
#define TOPIC_RPC_REQ_SUB      "v1/devices/me/rpc/request/+"
#define TOPIC_RPC_RESP_PREFIX  "v1/devices/me/rpc/response/"         // + requestId
#define TOPIC_FW_RESP_SUB      "v2/fw/response/+/chunk/+"

void coreiotBegin();

// Gọi trong loop(): giữ kết nối (kết nối lại có backoff), client.loop(), gửi telemetry định kỳ.
void coreiotLoop();

bool coreiotConnected();

// Publish chung — module OTA dùng để xin chunk và báo fw_state.
bool coreiotPublish(const char* topic, const char* payload);
bool coreiotPublishRaw(const char* topic, const uint8_t* data, size_t len);

// Gửi 1 cặp key/value telemetry ngay (vd. fw_state).
bool coreiotSendTelemetryString(const char* key, const char* value);
