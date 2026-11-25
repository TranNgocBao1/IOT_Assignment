// =============================================================================
// espnow_rx.h — Nhận trạng thái cửa sổ từ Node S qua ESP-NOW (module M2)
// =============================================================================
#pragma once

// Gọi SAU khi WiFi STA đã kết nối router (ESP-NOW dùng chung kênh với WiFi).
bool espnowBegin();

// Gọi trong loop(): xử lý gói đã nhận (callback chỉ lưu tạm), kiểm tra timeout.
void espnowLoop();

// Tỉ lệ mất gói (%) tính từ lúc khởi động — dùng cho bảng đo trong báo cáo.
float espnowLossPercent();
