// =============================================================================
// sensor.cpp — Module M1, phần của Bạn A
//
// Thuật toán dùng trong file:
//  1. Quét I2C: thử beginTransmission() với mọi địa chỉ 1..126, địa chỉ nào trả
//     ACK (endTransmission()==0) là có thiết bị.
//  2. Lấy mẫu định kỳ không chặn (non-blocking): so sánh millis() với lần đọc
//     trước thay vì delay(), để loop() vẫn phục vụ MQTT / web / ESP-NOW.
//  3. Kiểm tra hợp lệ (range check): loại số đọc nằm ngoài [CO2_MIN, CO2_MAX].
//  4. Trung bình trượt (moving average) N mẫu bằng bộ đệm vòng (ring buffer):
//     cộng mẫu mới, trừ mẫu cũ nhất => O(1) mỗi lần cập nhật.
// =============================================================================
#include <Wire.h>
#include "sensor.h"
#include "config.h"
#include "app_state.h"
// TODO[M1-A]: include thư viện cảm biến, ví dụ với SCD4x:
// #include <SensirionI2cScd4x.h>

// ----------------------------------------------------------------------------
// Bộ lọc trung bình trượt dùng bộ đệm vòng
// ----------------------------------------------------------------------------
template <int N>
class MovingAverage {
 public:
  // Thêm mẫu mới và trả về giá trị trung bình hiện tại.
  float add(float x) {
    // TODO[M1-A]:
    //  - Nếu bộ đệm chưa đầy: cộng x vào sum_, lưu vào buf_[idx_], tăng count_.
    //  - Nếu đã đầy: sum_ -= buf_[idx_] (mẫu cũ nhất), rồi sum_ += x, buf_[idx_] = x.
    //  - idx_ = (idx_ + 1) % N   (quay vòng)
    //  - return sum_ / count_
    return x;
  }
  void reset() { idx_ = 0; count_ = 0; sum_ = 0; }

 private:
  float buf_[N] = {0};
  int   idx_   = 0;
  int   count_ = 0;
  float sum_   = 0;
};

static MovingAverage<SENSOR_AVG_WINDOW> s_co2Avg;
static MovingAverage<SENSOR_AVG_WINDOW> s_tempAvg;
static MovingAverage<SENSOR_AVG_WINDOW> s_humAvg;
static uint32_t s_lastReadMs = 0;
// TODO[M1-A]: khai báo đối tượng cảm biến, ví dụ: static SensirionI2cScd4x s_scd4x;

void sensorScanI2C() {
  Serial.println(F("[SENSOR] Quet I2C..."));
  // TODO[M1-A]: vòng for addr = 1..126:
  //   Wire.beginTransmission(addr); if (Wire.endTransmission() == 0) in địa chỉ dạng 0x%02X
  //   SCD4x thường ở 0x62.
}

bool sensorBegin() {
  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
  sensorScanI2C();

  // TODO[M1-A]: khởi tạo cảm biến theo example của thư viện, thường gồm:
  //   1. begin(Wire, địa chỉ)
  //   2. stopPeriodicMeasurement() (phòng khi cảm biến đang đo từ lần trước)
  //   3. startPeriodicMeasurement()
  //  Trả về false nếu bước nào báo lỗi.
  return false;
}

// Đọc thô một lần. Trả true nếu có số mới hợp lệ.
static bool readRaw(float& co2, float& temp, float& hum) {
  // TODO[M1-A]:
  //  1. Hỏi cảm biến đã có dữ liệu mới chưa (getDataReadyStatus hoặc tương đương).
  //     Chưa có thì return false.
  //  2. readMeasurement(co2, temp, hum).
  //  3. Range check: co2 nằm ngoài [CO2_MIN_VALID, CO2_MAX_VALID] => return false.
  return false;
}

void sensorLoop() {
  uint32_t now = millis();
  if (now - s_lastReadMs < SENSOR_INTERVAL_MS) return;  // chưa đến chu kỳ
  s_lastReadMs = now;

  float co2, temp, hum;
  if (!readRaw(co2, temp, hum)) {
    // TODO[M1-A]: đếm số lần lỗi liên tiếp; quá 3 lần thì g_state.sensorValid = false
    return;
  }

  g_state.co2         = s_co2Avg.add(co2);
  g_state.temperature = s_tempAvg.add(temp);
  g_state.humidity    = s_humAvg.add(hum);
  g_state.sensorValid = true;

  Serial.printf("[SENSOR] co2=%.0f ppm  t=%.1f C  rh=%.1f %%\n",
                g_state.co2, g_state.temperature, g_state.humidity);
}
