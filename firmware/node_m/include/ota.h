// =============================================================================
// ota.h — Cập nhật firmware qua mạng từ CoreIOT (module M6, T14 = 2.0 điểm)
// =============================================================================
#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>

// Trạng thái gửi lên CoreIOT qua telemetry "fw_state" (ThingsBoard dùng đúng các chuỗi này)
enum class OtaState : uint8_t { IDLE, DOWNLOADING, DOWNLOADED, VERIFIED, UPDATING, UPDATED, FAILED };

void otaBegin();
void otaLoop();                                  // kiểm tra timeout chunk, thử lại
void otaOnConnected();                           // gọi khi MQTT vừa kết nối
void otaOnFirmwareInfo(JsonObjectConst shared);  // nhận fw_title, fw_version, fw_size, fw_checksum...
void otaOnChunk(int chunkIndex, const uint8_t* data, size_t len);
OtaState otaState();
