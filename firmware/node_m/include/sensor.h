// =============================================================================
// sensor.h — Đọc cảm biến CO2 / nhiệt độ / độ ẩm qua I2C (module M1)
// =============================================================================
#pragma once
#include <Arduino.h>

// Khởi tạo I2C và cảm biến, bắt đầu đo định kỳ. Trả false nếu không thấy cảm biến.
bool sensorBegin();

// Gọi trong loop(). Tự đọc khi đến chu kỳ, lọc số liệu, ghi vào g_state.
void sensorLoop();

// Quét bus I2C và in địa chỉ tìm thấy (dùng khi mới nối dây để kiểm tra).
void sensorScanI2C();
