// =============================================================================
// config.h — Hằng số cấu hình Node M (chân, chu kỳ, ngưỡng mặc định)
// Không chứa bí mật — bí mật để trong secrets.h.
// =============================================================================
#pragma once

#ifndef FW_TITLE
#define FW_TITLE "air-node-m"
#endif
#ifndef FW_VERSION
#define FW_VERSION "0.0.0-dev"
#endif

// ----------------------------- Chân phần cứng (M1) ---------------------------
// TODO[M1-A]: điền chân I2C đúng theo sơ đồ chân YoloUNO.
#define PIN_I2C_SDA        11
#define PIN_I2C_SCL        12
// TODO[M1-B]: điền chân điều khiển relay quạt.
#define PIN_FAN_RELAY      10
// Nhiều module relay kích ở mức THẤP (LOW = đóng relay). Đo thử rồi chỉnh.
#define RELAY_ACTIVE_LOW   true

// ----------------------------- Chu kỳ (ms) -----------------------------------
#define SENSOR_INTERVAL_MS     5000   // SCD4x chỉ có số mới khoảng mỗi 5 s
#define TELEMETRY_INTERVAL_MS  5000   // gửi CoreIOT mỗi 5 s
#define ESPNOW_TIMEOUT_MS      15000  // không có gói ESP-NOW quá 15 s => mất liên lạc Node S
#define WIFI_RETRY_MS          5000
#define MQTT_BACKOFF_MIN_MS    1000   // backoff kết nối lại MQTT: 1 s -> 2 s -> 4 s ... tối đa
#define MQTT_BACKOFF_MAX_MS    30000

// ----------------------------- Cảm biến --------------------------------------
#define CO2_MIN_VALID      300     // dưới ngưỡng này coi là số đọc lỗi
#define CO2_MAX_VALID      10000
#define SENSOR_AVG_WINDOW  3       // trung bình trượt N mẫu để số đỡ nhảy

// ----------------------------- Quạt ------------------------------------------
// Thời gian tối thiểu giữa 2 lần đổi trạng thái quạt (chống bật tắt liên tục,
// bảo vệ relay). Lệnh đến sớm hơn sẽ bị từ chối.
#define FAN_MIN_SWITCH_MS  10000

// ----------------------------- Ngưỡng mặc định (ppm) -------------------------
// Giá trị thật được ghi đè bằng shared attributes co2_on / co2_off / co2_alarm.
#define DEFAULT_CO2_ON     1000
#define DEFAULT_CO2_OFF    800
#define DEFAULT_CO2_ALARM  1500

// Mức chất lượng cho /api/data (good < 1000 <= fair <= 1500 < poor)
#define LEVEL_FAIR_FROM    1000
#define LEVEL_POOR_FROM    1500

// ----------------------------- MQTT / OTA (M4, M6) ---------------------------
#define OTA_CHUNK_SIZE     4096                    // byte mỗi chunk firmware
#define MQTT_BUFFER_SIZE   (OTA_CHUNK_SIZE + 512)  // phải chứa được 1 chunk + topic
#define OTA_CHUNK_TIMEOUT_MS 10000                 // quá thời gian không nhận chunk => xin lại
#define OTA_MAX_RETRY      3

// ----------------------------- Web server (M3) -------------------------------
#define WEB_PORT           80
