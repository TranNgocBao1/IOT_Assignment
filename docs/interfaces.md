# Hợp đồng giao diện (interface contract)

Mọi tên key, topic, API dưới đây là **thỏa thuận chung** giữa hai người. Muốn đổi thì báo người kia và sửa file này trước.

## Telemetry Node M → CoreIOT (`v1/devices/me/telemetry`, mỗi 5 s)

| Key | Kiểu | Ghi chú |
| --- | --- | --- |
| `co2` | số (ppm) | bỏ trống nếu cảm biến lỗi |
| `temperature` | số (°C) | |
| `humidity` | số (%) | |
| `window_open` | bool | từ Node S qua ESP-NOW |
| `fan_state` | bool | **bắt buộc**, RuleChain cần |
| `fw_version` | chuỗi | |
| `window_link`, `espnow_loss_pct`, `rssi` | tùy chọn | chẩn đoán, phục vụ bảng đo |
| `current_fw_title`, `current_fw_version`, `fw_state`, `fw_error` | chuỗi | OTA |

## Điều khiển và cấu hình

| Loại | Tên | Giá trị |
| --- | --- | --- |
| RPC | `setFan` | `params`: `true` / `false` (hoặc `{"on": true}`) |
| RPC | `getFan` | trả `fan_state` (cho widget nút) |
| Shared attribute | `co2_on`, `co2_off`, `co2_alarm` | mặc định 1000, 800, 1500 |
| Shared attribute (OTA) | `fw_title`, `fw_version`, `fw_size`, `fw_checksum`, `fw_checksum_algorithm` | CoreIOT tự đặt khi gán gói |

## API cục bộ trên Node M

| API | Trả về |
| --- | --- |
| `GET /api/data` | `{co2, temperature, humidity, window_open, fan_state, fw_version, level}` |
| `GET /api/fan?on=1\|0[&src=gateway]` | `{ok, fan_state}`; 400 nếu tham số sai, 409 nếu đổi quá nhanh |

`level`: `good` nếu CO2 < 1000, `fair` từ 1000 đến 1500, `poor` trên 1500.

## Gateway → CoreIOT (device `air-gateway`)

| Key | Ghi chú |
| --- | --- |
| `co2_avg_1m`, `co2_max_1m`, `temperature_avg_1m`, `humidity_avg_1m`, `samples` | gửi kèm `ts` (ms) mỗi phút |

## ESP-NOW (`firmware/common/espnow_packet.h`)

`{version, node_id, window_open, seq}` — gửi khi đổi trạng thái và mỗi 5 s.

## Quy ước chung

- Token và mật khẩu WiFi chỉ nằm trong `secrets.h` (firmware) và `.env` (gateway); cả hai đã có trong `.gitignore`.
- Node M có IP cố định qua DHCP reservation trên router; kênh WiFi router cố định.
- TODO đánh dấu `TODO[Mx-A]` / `TODO[Mx-B]` theo module và người phụ trách.
