# Giám sát chất lượng không khí trong phòng — IoT (CoreIOT + ESP32-S3)

Đồ án môn IoT của nhóm 2 người. Hệ thống đo **CO2, nhiệt độ, độ ẩm**, biết **cửa sổ đang mở hay đóng**, tự **bật/tắt quạt** theo luật trên CoreIOT, gửi **cảnh báo** khi CO2 quá cao, cập nhật firmware **qua mạng (OTA)**, và có **gateway trên PC** tổng hợp số liệu.

> Repo này là **khung code**: cấu trúc, hàm, luồng xử lý đã có sẵn; phần logic cốt lõi để trống ở các dòng `TODO[Mx-A]` / `TODO[Mx-B]` để nhóm tự viết. Liệt kê toàn bộ TODO:
>
> ```bash
> grep -rn "TODO\[" --include=*.cpp --include=*.h --include=*.py --include=*.js .
> grep -rn "TODO\[M4-A\]" .      # chỉ việc của Bạn A trong module M4
> ```

---

## Mục lục

1. [Kiến trúc hệ thống](#1-kiến-trúc-hệ-thống)
2. [Element chấm điểm và nơi cài đặt](#2-element-chấm-điểm-và-nơi-cài-đặt)
3. [Cấu trúc thư mục](#3-cấu-trúc-thư-mục)
4. [Cài đặt và chạy](#4-cài-đặt-và-chạy)
5. [Mô tả từng file và thuật toán](#5-mô-tả-từng-file-và-thuật-toán)
6. [Bảng tổng hợp thuật toán](#6-bảng-tổng-hợp-thuật-toán)
7. [Quy ước TODO và phân công](#7-quy-ước-todo-và-phân-công)
8. [Thứ tự test đề xuất](#8-thứ-tự-test-đề-xuất)
9. [Lỗi thường gặp](#9-lỗi-thường-gặp)

---

## 1. Kiến trúc hệ thống

```
  ┌──────────────┐  ESP-NOW (không cần router)   ┌────────────────────────────────┐
  │   Node S     │ ────────────────────────────► │  Node M  (YoloUNO ESP32-S3)     │
  │ ESP32 +      │   {version,id,window_open,seq}│  - cảm biến CO2/T/RH (I2C)      │
  │ công tắc từ  │                               │  - relay quạt                   │
  └──────────────┘                               │  - web server  /api/data, /     │
                                                 │  - MQTT tới CoreIOT, OTA        │
                                                 └───────┬──────────────▲──────────┘
                              MQTT telemetry / RPC / OTA │              │ HTTP (LAN)
                                                         ▼              │
  ┌───────────────────────────────────────┐      ┌───────────────────────┴──────────┐
  │ CoreIOT (ThingsBoard)                 │◄─────│ Gateway Python trên PC           │
  │ - device air-node-m, air-gateway      │ MQTT │ - hỏi /api/data mỗi 5 s, lưu CSV │
  │ - dashboard (T11), RuleChain (T12)    │─────►│ - trung bình 1 phút -> CoreIOT   │
  │ - alarm + thông báo (T13), OTA (T14)  │ RPC  │ - RPC setFan -> /api/fan         │
  └───────────────────────────────────────┘      │ - giao diện Streamlit (T10)      │
                                                 └──────────────────────────────────┘
```

Luồng tự động chính: Node M gửi `co2`, `window_open`, `fan_state` mỗi 5 s → RuleChain so ngưỡng (hysteresis) → gửi RPC `setFan` → Node M bật relay → gói tiếp theo báo `fan_state=true`.

---

## 2. Element chấm điểm và nơi cài đặt

| Element | Điểm | Chức năng trong sản phẩm | File chính |
| --- | --- | --- | --- |
| T14 OTA | 2.0 | Cập nhật firmware Node M từ CoreIOT, không cắm cáp | `firmware/node_m/src/ota.cpp`, `partitions_ota.csv`, `tools/ota_checksum.py` |
| T06 ESP-NOW | 1.5 | Node S báo trạng thái cửa sổ cho Node M | `firmware/node_s/src/main.cpp`, `firmware/node_m/src/espnow_rx.cpp`, `firmware/common/espnow_packet.h` |
| T07 Web server trên thiết bị | 1.0 | Xem số liệu, điều khiển quạt khi mất internet | `firmware/node_m/src/web_server.cpp`, `include/web_page.h` |
| T08 Gateway | 1.0 | PC tổng hợp số liệu, chuyển lệnh từ cloud xuống | `gateway/gateway.py` và các module đi kèm |
| T10 Giao diện gateway | 1.0 | Trang Streamlit của gateway | `gateway/ui_app.py` |
| T11 Dashboard | 1.0 | Dashboard CoreIOT | `firmware/node_m/src/coreiot.cpp` + cấu hình trên CoreIOT |
| T12 RuleChain | 1.0 | Tự bật/tắt quạt theo CO2 và cửa sổ | `coreiot/rulechain/*.js` |
| T13 Cảnh báo | 0.5 | Alarm + thông báo khi CO2 > 1500 ppm | `coreiot/rulechain/04_filter_alarm.js` |
| **Tổng** | **9.0** | Trần tính 8.0, dư 1.0 (nếu trễ: bỏ T10 trước) | |

---

## 3. Cấu trúc thư mục

```
air-quality-iot/
├── README.md                      ← file này
├── .gitignore                     ← chặn secrets.h, .env, .pio, data/
├── docs/
│   └── interfaces.md              ← hợp đồng giao diện (key, topic, API) — đọc ĐẦU TIÊN
├── firmware/
│   ├── common/
│   │   └── espnow_packet.h        ← struct gói ESP-NOW dùng chung 2 node
│   ├── node_m/                    ← YoloUNO ESP32-S3
│   │   ├── platformio.ini
│   │   ├── partitions_ota.csv
│   │   ├── include/
│   │   │   ├── secrets.h.example  ← mẫu, chép thành secrets.h
│   │   │   ├── config.h           ← chân, chu kỳ, ngưỡng
│   │   │   ├── app_state.h        ← trạng thái dùng chung
│   │   │   ├── sensor.h  fan.h  espnow_rx.h  coreiot.h  ota.h  web_server.h
│   │   │   └── web_page.h         ← HTML + JS nhúng
│   │   └── src/
│   │       ├── main.cpp           ← setup()/loop()
│   │       ├── app_state.cpp
│   │       ├── sensor.cpp         ← M1
│   │       ├── fan.cpp            ← M1
│   │       ├── espnow_rx.cpp      ← M2
│   │       ├── web_server.cpp     ← M3
│   │       ├── coreiot.cpp        ← M4, M5
│   │       └── ota.cpp            ← M6
│   └── node_s/                    ← ESP32 ở cửa sổ
│       ├── platformio.ini
│       ├── include/config.h
│       └── src/main.cpp           ← M1 + M2
├── gateway/                       ← Python trên PC (M7, M8)
│   ├── requirements.txt
│   ├── .env.example
│   ├── config.py
│   ├── gateway.py                 ← chương trình chính
│   ├── node_client.py
│   ├── aggregator.py
│   ├── storage.py
│   ├── offline_queue.py
│   ├── coreiot_client.py
│   ├── mock_node_m.py             ← Node M giả (Flask)
│   └── ui_app.py                  ← giao diện Streamlit
├── coreiot/
│   └── rulechain/                 ← script dán vào RuleChain (M5)
│       ├── README_rulechain.md
│       ├── 01_filter_fan_on.js
│       ├── 02_filter_fan_off.js
│       ├── 03_transform_rpc_fan.js
│       └── 04_filter_alarm.js
└── tools/
    ├── fake_telemetry.py          ← telemetry giả để test dashboard/RuleChain
    ├── ota_checksum.py            ← SHA-256 của firmware.bin
    └── measure_rpc_latency.py     ← đo độ trễ điều khiển cho báo cáo
```

---

## 4. Cài đặt và chạy

### 4.1 Firmware (PlatformIO trên VS Code)

```bash
cd firmware/node_m
cp include/secrets.h.example include/secrets.h     # điền WiFi + token air-node-m
pio run -t upload && pio device monitor

cd ../node_s
# điền NODE_M_MAC và ESPNOW_CHANNEL trong include/config.h (lấy từ serial Node M)
pio run -t upload && pio device monitor
```

Trước khi build lần đầu: sửa `board` trong `platformio.ini` và chân trong `config.h` theo tài liệu YoloUNO.

### 4.2 Gateway (Python 3.10+)

```bash
cd gateway
python -m venv .venv && source .venv/bin/activate     # Windows: .venv\Scripts\activate
pip install -r requirements.txt
cp .env.example .env                                  # điền NODE_M_URL, GATEWAY_TOKEN

python mock_node_m.py          # (tùy chọn) Node M giả khi chưa có board
python gateway.py              # terminal 1
streamlit run ui_app.py        # terminal 2
```

### 4.3 CoreIOT

1. Tạo 2 device: `air-node-m`, `air-gateway`; lấy access token.
2. Shared attributes của `air-node-m`: `co2_on=1000`, `co2_off=800`, `co2_alarm=1500`.
3. Dựng RuleChain theo `coreiot/rulechain/README_rulechain.md`.
4. Dựng dashboard (thử trước bằng `tools/fake_telemetry.py`).
5. OTA: build bản `FW_VERSION="1.0.1"`, upload `firmware.bin` vào **OTA updates**, title `air-node-m`, gán cho device.

---

## 5. Mô tả từng file và thuật toán

### 5.1 `firmware/common/espnow_packet.h`

**Công dụng:** định nghĩa struct `WindowPacket` mà Node S gửi và Node M nhận. Để ở thư mục chung để hai firmware luôn dùng cùng một định dạng.

**Kỹ thuật:**
- `__attribute__((packed))`: bỏ byte đệm để hai chip ra cùng kích thước struct.
- Trường `version`: Node M bỏ gói khác phiên bản, tránh đọc sai khi một bên chưa nạp lại.
- `static_assert` kiểm tra lúc biên dịch rằng gói ≤ 250 byte (giới hạn ESP-NOW).

### 5.2 `firmware/node_m/platformio.ini` và `partitions_ota.csv`

**Công dụng:** cấu hình build Node M: board, thư viện (`PubSubClient`, `ArduinoJson`, `Sensirion SCD4x`), cờ `FW_TITLE`/`FW_VERSION`, bảng phân vùng.

**Kỹ thuật:** bảng phân vùng **A/B hai slot** (`app0`, `app1`). Firmware mới ghi vào slot đang không chạy; `otadata` chỉ đổi slot boot khi firmware mới ghi đủ và đúng checksum. Lỗi giữa chừng thì bản cũ vẫn chạy.

### 5.3 `include/secrets.h.example`

**Công dụng:** mẫu cho WiFi, host/port MQTT và access token. Bản thật `secrets.h` không được commit (đã có trong `.gitignore`).

### 5.4 `include/config.h`

**Công dụng:** gom mọi hằng số (chân, chu kỳ, ngưỡng mặc định, kích thước chunk OTA, buffer MQTT) vào một chỗ để chỉnh không phải tìm trong code.

**Lưu ý:** `MQTT_BUFFER_SIZE = OTA_CHUNK_SIZE + 512` vì PubSubClient bỏ qua tin lớn hơn buffer; chunk OTA 4096 byte cần buffer > 4096.

### 5.5 `include/app_state.h`, `src/app_state.cpp`

**Công dụng:** struct `AppState g_state` chứa toàn bộ trạng thái (số đo, cửa sổ, quạt, ngưỡng, kết nối). Module ghi phần của mình và đọc phần của module khác qua đây, nên hai người làm hai module song song mà không phụ thuộc hàm của nhau.

**Thuật toán:** `airLevel()` phân mức bằng so sánh 2 ngưỡng (good < 1000 ≤ fair ≤ 1500 < poor). `FanSource` ghi nguồn lệnh quạt để log và giải thích trong báo cáo.

### 5.6 `src/sensor.cpp` (M1 – Bạn A)

**Công dụng:** khởi tạo I2C, cảm biến SCD4x, đọc CO2/nhiệt độ/độ ẩm mỗi 5 s, ghi vào `g_state`.

**Thuật toán:**
1. **Quét I2C**: thử mọi địa chỉ 1–126, địa chỉ trả ACK là có thiết bị. Dùng để kiểm tra nối dây.
2. **Lấy mẫu không chặn**: so `millis()` với lần đọc trước thay cho `delay()`.
3. **Kiểm tra miền giá trị**: loại số ngoài 300–10000 ppm.
4. **Trung bình trượt N mẫu bằng bộ đệm vòng** (`MovingAverage<N>`): giữ tổng chạy, mỗi mẫu mới cộng vào, mẫu cũ nhất trừ ra, chi phí O(1). Làm số liệu đỡ nhảy, giúp RuleChain không phản ứng với nhiễu.

### 5.7 `src/fan.cpp` (M1 – Bạn B)

**Công dụng:** điều khiển relay quạt. Mọi nguồn (RPC, web, gateway) đều phải gọi `fanSet()`.

**Thuật toán:**
1. **Đảo logic active-low**: nhiều module relay đóng khi chân ở mức LOW.
2. **Chống bật tắt liên tục (anti short-cycle)**: từ chối lệnh nếu chưa đủ `FAN_MIN_SWITCH_MS` (10 s) từ lần đổi trước. Bảo vệ relay khi dashboard, RuleChain và web gửi lệnh cùng lúc.
3. **Idempotent**: lệnh trùng trạng thái hiện tại được coi là thành công, không ghi lại chân.
4. Khởi động luôn **tắt** (trạng thái an toàn).

### 5.8 `src/espnow_rx.cpp` (M2 – Bạn B)

**Công dụng:** nhận gói `WindowPacket` từ Node S, cập nhật `windowOpen`, đếm mất gói, phát hiện mất liên lạc.

**Thuật toán:**
1. **Callback chỉ lưu, loop xử lý (producer/consumer)**: callback ESP-NOW chạy trong task WiFi nên chỉ `memcpy` vào biến tạm trong vùng găng `portMUX`; `espnowLoop()` lấy ra xử lý. Tránh gọi MQTT/Serial trong callback (dễ treo hoặc crash).
2. **Đếm mất gói bằng số thứ tự**: `seq` nhảy từ `last` lên `seq` thì mất `seq − last − 1` gói; `seq ≤ last` nghĩa là Node S khởi động lại, đặt mốc mới. Tỉ lệ mất = `lost / (rx + lost)`. Số này dùng cho bảng đo trong báo cáo.
3. **Timeout (watchdog phần mềm)**: 15 s không có gói thì `windowLinkOk = false`.

**Lưu ý:** ESP-NOW và WiFi STA dùng chung radio, nên Node S phải ở **đúng kênh router**.

### 5.9 `src/web_server.cpp`, `include/web_page.h` (M3)

**Công dụng:** web server trên Node M (port 80): trang `/` cho người dùng, `/api/data` và `/api/fan` cho trang web và gateway. Hoạt động trong LAN kể cả khi mất internet.

**Thuật toán / kỹ thuật:**
- **Định tuyến HTTP** theo đường dẫn (`WebServer::on`).
- **Kiểm tra đầu vào**: `on` chỉ nhận `0`/`1`, sai trả **400**; bị chống bật tắt liên tục từ chối thì trả **409**.
- **Header CORS** để ứng dụng khác gọi được API.
- Trang web (Bạn B): **polling** `fetch('/api/data')` mỗi 3 s bằng `setInterval`, cập nhật DOM, đổi màu theo `level`. HTML để trong flash bằng `PROGMEM` để tiết kiệm RAM.

### 5.10 `src/coreiot.cpp` (M4 – Bạn A, dùng tiếp ở M5, M6)

**Công dụng:** toàn bộ giao tiếp MQTT với CoreIOT: kết nối (username = token), gửi telemetry mỗi 5 s, nhận RPC `setFan`/`getFan`, nhận shared attributes (ngưỡng và thông tin firmware), chuyển chunk firmware cho module OTA.

**Thuật toán:**
1. **Kết nối lại với exponential backoff**: thời gian chờ nhân đôi sau mỗi lần thất bại (1 → 2 → 4 … ≤ 30 s), thành công thì đặt lại. Không chặn `loop()`, không dội server.
2. **Định tuyến theo topic**: một callback, so tiền tố topic để gọi `handleRpc` / `handleAttributes` / `otaOnChunk`.
3. **Tách requestId** từ cuối topic để trả lời RPC đúng topic `rpc/response/{id}`.
4. **Xin shared attributes khi kết nối** (`attributes/request/1`) để có ngay ngưỡng và thông tin OTA, rồi nhận cập nhật đẩy xuống ở `v1/devices/me/attributes`. Xử lý được cả dạng `{"shared":{...}}` lẫn dạng phẳng.
5. **Kiểm tra ngưỡng hợp lệ** (`co2_off < co2_on < co2_alarm`) trước khi áp dụng.
6. **Gửi kèm `fan_state` trong mọi gói**, vì RuleChain không lưu trạng thái giữa các tin nhắn.

### 5.11 `src/ota.cpp` (M6 – Bạn A)

**Công dụng:** client OTA theo giao thức firmware của ThingsBoard qua MQTT.

**Thuật toán:**
1. **Máy trạng thái hữu hạn**: `IDLE → DOWNLOADING → DOWNLOADED → VERIFIED → UPDATING → (khởi động lại) → UPDATED`, lỗi thì `FAILED` kèm `fw_error`. Mỗi lần đổi trạng thái gửi `fw_state` lên CoreIOT để trang OTA hiển thị tiến trình.
2. **So phiên bản**: chỉ tải khi `fw_title` khớp `FW_TITLE` và `fw_version` khác `FW_VERSION`. Nếu bằng nhau nghĩa là vừa cập nhật xong, báo `UPDATED`.
3. **Tải theo khối kiểu stop-and-wait**: xin chunk `i` (`v2/fw/request/{id}/chunk/{i}`, payload = kích thước chunk), nhận ở `v2/fw/response/...`, ghi xong mới xin chunk `i+1`. Chỉ cần 1 buffer 4 KB, hợp với RAM của vi điều khiển.
4. **Băm SHA-256 tăng dần** (`mbedtls_sha256_update` mỗi chunk), cuối cùng so với `fw_checksum`. Sai thì `Update.abort()`, bản cũ vẫn chạy.
5. **Timeout và thử lại có giới hạn**: 10 s không có chunk thì xin lại, quá 3 lần thì `FAILED`.
6. Ghi vào slot rảnh (`Update.begin/write/end`), `Update.end(true)` đổi slot boot, rồi `ESP.restart()`.

### 5.12 `src/main.cpp` (Node M)

**Công dụng:** khởi tạo theo đúng thứ tự (quạt tắt → cảm biến → WiFi → ESP-NOW → web → MQTT → OTA) và chạy vòng lặp chính.

**Thuật toán:** **lập lịch hợp tác** — mỗi module có `xxxLoop()` ngắn, tự kiểm `millis()`, không `delay()` dài. Nhờ vậy cảm biến, ESP-NOW, web, MQTT, OTA cùng chạy trên một vòng `loop()` mà không cần task FreeRTOS riêng. `WiFi.setSleep(false)` để ESP-NOW nhận ổn định.

### 5.13 `firmware/node_s/include/config.h`, `src/main.cpp`

**Công dụng:** Node S đọc công tắc từ ở cửa sổ và gửi trạng thái cho Node M qua ESP-NOW.

**Thuật toán:**
1. **Chống dội tiếp điểm theo thời gian**: trạng thái mới chỉ được nhận khi giữ ổn định ≥ 50 ms.
2. **Gửi theo sự kiện + nhịp tim**: gửi ngay khi cửa đổi, và gửi lại mỗi 5 s để Node M biết Node S còn sống và để đo mất gói.
3. **Đóng dấu `seq` tăng dần** mỗi gói.
4. **Ép kênh WiFi**: Node S không vào router nên phải đặt kênh bằng `esp_wifi_set_channel()` (bật promiscuous tạm thời để được phép đổi kênh).
5. Đọc kết quả gửi trong callback `onSent` (ACK tầng MAC) để thống kê.

### 5.14 `gateway/config.py`

**Công dụng:** đọc `.env` (URL Node M, token `air-gateway`, chu kỳ, thư mục dữ liệu) và tạo thư mục `data/`. Mọi file khác lấy cấu hình từ đây.

### 5.15 `gateway/node_client.py` (M7 – Bạn A)

**Công dụng:** lớp `NodeMClient` gọi `GET /api/data` và `GET /api/fan`.

**Thuật toán:** **thử lại có giới hạn với thời gian chờ tăng dần** (0.5 s, 1 s); **timeout** mọi request để gateway không treo khi Node M tắt; `requests.Session` giữ kết nối TCP. Kiểm tra đủ key trước khi trả dữ liệu.

### 5.16 `gateway/aggregator.py` (M7 – Bạn A)

**Công dụng:** gộp mẫu 5 s thành số liệu 1 phút: `co2_avg_1m`, `co2_max_1m`, `temperature_avg_1m`, `humidity_avg_1m`, `samples`.

**Thuật toán:** **cửa sổ cuộn không chồng lấn (tumbling window)**: `window_start = floor(ts / 60) × 60`. Mẫu cùng cửa sổ cộng dồn tổng, số lượng và max; mẫu đầu của cửa sổ mới tới thì đóng cửa sổ cũ và tính trung bình. Chạy trực tuyến, bộ nhớ O(1), bỏ qua `None`. Timestamp kết quả tính bằng **mili-giây** theo yêu cầu của ThingsBoard. Có sẵn đoạn tự kiểm `python aggregator.py`.

### 5.17 `gateway/storage.py` (M7 – Bạn A, M8 – Bạn A)

**Công dụng:** lưu mỗi mẫu thô vào CSV, đọc lại cho giao diện.

**Thuật toán:** **ghi nối (append-only)** nên an toàn khi tắt đột ngột; **xoay file theo ngày** (`raw_YYYY-MM-DD.csv`); đọc **N dòng cuối** bằng pandas cho biểu đồ.

### 5.18 `gateway/offline_queue.py` (M7 – Bạn B)

**Công dụng:** giữ telemetry khi mất internet và gửi bù khi có mạng lại.

**Thuật toán:**
- **Store-and-forward với hàng đợi FIFO lưu trên đĩa** (`.jsonl`), không mất dữ liệu khi khởi động lại gateway.
- **Giới hạn kích thước, bỏ phần tử cũ nhất** khi đầy.
- **peek → gửi → ack**: chỉ xóa khỏi hàng đợi sau khi gửi thành công (đảm bảo ít nhất một lần).
- **Ghi nguyên tử** (ghi file tạm rồi `replace`) để file không hỏng nếu tắt giữa chừng.
- `threading.Lock` vì luồng MQTT và luồng chính cùng dùng.
- Mỗi phần tử giữ **timestamp gốc** nên dữ liệu gửi bù hiện đúng thời điểm trên biểu đồ.

### 5.19 `gateway/coreiot_client.py` (M7 – Bạn B)

**Công dụng:** kết nối MQTT bằng token `air-gateway`, gửi telemetry tổng hợp, nhận RPC và trả lời.

**Thuật toán / kỹ thuật:** paho-mqtt chạy **vòng mạng ở luồng nền** và **tự kết nối lại có backoff** (1–30 s). **Mọi telemetry đi qua hàng đợi** rồi `flush()`, nên lúc có mạng và mất mạng dùng chung một đường. Gửi **theo lô** (mảng `[{ts, values}, ...]` trong một lần publish). RPC chuyển cho hàm do `gateway.py` truyền vào (**dependency injection**), nên file này không phụ thuộc Node M.

### 5.20 `gateway/gateway.py` (M7)

**Công dụng:** chương trình chính. Mỗi 5 s: hỏi Node M, lưu CSV, tổng hợp, gửi CoreIOT, gửi bù; xử lý RPC `setFan` bằng cách gọi `/api/fan`; ghi `data/gateway_status.json` cho giao diện.

**Thuật toán:** **polling định kỳ có bù thời gian xử lý**: `sleep(max(0, chu_kỳ − thời_gian_đã_dùng))` để chu kỳ đều, không trôi dần. Bắt `SIGINT/SIGTERM` để dừng gọn (ngắt MQTT đúng cách). Ghi file trạng thái nguyên tử.

### 5.21 `gateway/mock_node_m.py`

**Công dụng:** Node M giả bằng Flask, API giống hệt board thật. Dùng để làm gateway và giao diện khi chưa có phần cứng. Có thêm `/mock/window?open=1` để giả lập mở cửa.

**Thuật toán:** **bước ngẫu nhiên có xu hướng (random walk with drift)**: CO2 tăng khoảng 8 ppm mỗi lần khi phòng kín, giảm khoảng 25 ppm khi quạt chạy hoặc cửa mở, cộng nhiễu Gauss, kẹp trong [420, 3000]. Có luôn luật chống bật tắt liên tục 10 s và mã 409 giống firmware.

### 5.22 `gateway/ui_app.py` (M8)

**Công dụng:** giao diện Streamlit của gateway: trạng thái kết nối (Node M, CoreIOT, hàng đợi), số liệu hiện tại, biểu đồ lịch sử từ CSV, nút bật/tắt quạt.

**Kỹ thuật:** `st.fragment(run_every=5)` tự làm mới phần hiển thị mỗi 5 s; đọc dữ liệu qua file (`gateway_status.json`, CSV), **tách khỏi tiến trình gateway**, nên giao diện lỗi không làm dừng gateway.

### 5.23 `coreiot/rulechain/*.js` (M5 – Bạn B)

**Công dụng:** script dán vào các node Script trong RuleChain của CoreIOT. `README_rulechain.md` có sơ đồ cách nối node.

| File | Node | Vai trò |
| --- | --- | --- |
| `01_filter_fan_on.js` | Filter | true khi `co2 > co2_on` và cửa đóng và quạt đang tắt |
| `02_filter_fan_off.js` | Filter | true khi `co2 < co2_off` và quạt đang bật |
| `03_transform_rpc_fan.js` | Transformation | tạo `{method:"setFan", params:true/false}` cho node RPC call request |
| `04_filter_alarm.js` | Filter | tạo / xóa alarm CO2 cao (có trễ 100 ppm) |

**Thuật toán:** **điều khiển trễ hai ngưỡng (hysteresis)** — bật ở 1000, tắt ở 800, vùng giữa giữ nguyên nên quạt không nhấp nháy khi CO2 dao động quanh một ngưỡng. Cảnh báo cũng có trễ (tạo > 1500, xóa < 1400). Ngưỡng lấy từ shared attributes qua node **Originator attributes** (metadata `shared_co2_on`…), nên đổi ngưỡng trên dashboard không phải sửa code.

### 5.24 `tools/fake_telemetry.py`

**Công dụng:** giả làm Node M trên PC: gửi telemetry theo kịch bản `rise` (qua cả 3 ngưỡng), `window` (cửa mở, quạt không được bật), `steady` (dao động trong vùng trễ); đồng thời nhận RPC `setFan` và đổi `fan_state` giả, nên thấy được cả vòng kín của RuleChain. Dùng để dựng dashboard (M4) và test RuleChain/cảnh báo (M5) trước khi có phần cứng.

### 5.25 `tools/ota_checksum.py`

**Công dụng:** in kích thước, số chunk và SHA-256 của `firmware.bin` để đối chiếu với CoreIOT và ghi vào báo cáo.

**Thuật toán:** băm SHA-256 **theo khối 64 KB**, không nạp cả file vào RAM; cùng nguyên lý băm tăng dần mà Node M làm khi nhận OTA.

### 5.26 `tools/measure_rpc_latency.py`

**Công dụng:** đo thời gian khứ hồi của `/api/fan` N lần (xen kẽ bật/tắt, cách nhau > 10 s), in min, trung bình, trung vị (TODO: p95). Đưa kết quả vào bảng đánh giá của báo cáo.

### 5.27 `docs/interfaces.md`

**Công dụng:** hợp đồng giao diện: tên key telemetry, RPC, shared attributes, API cục bộ, gói ESP-NOW, quy ước bí mật. Hai người phải thống nhất file này **trước khi** chia việc trong mỗi module.

---

## 6. Bảng tổng hợp thuật toán

| Thuật toán / kỹ thuật | File | Giải quyết vấn đề gì |
| --- | --- | --- |
| Trung bình trượt (ring buffer) | `sensor.cpp` | Lọc nhiễu số đo CO2 |
| Kiểm tra miền giá trị | `sensor.cpp` | Loại số đọc lỗi |
| Lập lịch hợp tác bằng `millis()` | `main.cpp`, mọi `xxxLoop()` | Chạy nhiều việc trên 1 vòng lặp, không chặn |
| Chống bật tắt liên tục | `fan.cpp`, `mock_node_m.py` | Bảo vệ relay khi nhiều nguồn lệnh |
| Chống dội tiếp điểm theo thời gian | `node_s/main.cpp` | Công tắc từ nảy khi đóng/mở |
| Gửi theo sự kiện + nhịp tim | `node_s/main.cpp` | Phản hồi nhanh và phát hiện mất node |
| Số thứ tự đếm mất gói | `node_s/main.cpp`, `espnow_rx.cpp` | Đo chất lượng ESP-NOW |
| Producer/consumer trong vùng găng | `espnow_rx.cpp` | Callback WiFi không được làm việc nặng |
| Timeout liên lạc | `espnow_rx.cpp` | Biết Node S mất kết nối |
| Ép kênh WiFi | `node_s/main.cpp` | ESP-NOW cùng kênh với router |
| Exponential backoff | `coreiot.cpp`, `coreiot_client.py`, `node_client.py` | Kết nối lại MQTT/HTTP hợp lý |
| Định tuyến theo topic | `coreiot.cpp` | Một callback cho RPC / attribute / OTA |
| Máy trạng thái OTA | `ota.cpp` | Báo tiến trình, xử lý lỗi rõ ràng |
| Tải khối stop-and-wait | `ota.cpp` | Tải firmware lớn với RAM nhỏ |
| SHA-256 tăng dần | `ota.cpp`, `ota_checksum.py` | Xác minh firmware toàn vẹn |
| Phân vùng A/B | `partitions_ota.csv` | OTA lỗi không làm hỏng thiết bị |
| Hysteresis hai ngưỡng | `rulechain/01–02.js`, `04.js` | Quạt và cảnh báo không nhấp nháy |
| Tumbling window | `aggregator.py` | Trung bình / max theo phút |
| Store-and-forward FIFO trên đĩa | `offline_queue.py` | Không mất dữ liệu khi mất internet |
| Ghi nguyên tử | `offline_queue.py`, `gateway.py` | File không hỏng khi tắt đột ngột |
| Ghi nối + xoay file theo ngày | `storage.py` | Lưu lịch sử đơn giản, an toàn |
| Polling có bù thời gian | `gateway.py` | Chu kỳ đều 5 s |
| Random walk with drift | `mock_node_m.py` | Dữ liệu giả có hành vi giống thật |

---

## 7. Quy ước TODO và phân công

Mỗi TODO có dạng `TODO[<module>-<người>]`, khớp bảng phân công trong kế hoạch 2 tuần.

| Module | Bạn A | Bạn B |
| --- | --- | --- |
| M0 Nền tảng | repo, `.gitignore`, linh kiện | CoreIOT, `board` trong `platformio.ini` |
| M1 Phần cứng | `sensor.cpp` | `fan.cpp`, đọc công tắc trong `node_s/main.cpp` |
| M2 ESP-NOW | gửi trong `node_s/main.cpp` | nhận trong `espnow_rx.cpp` |
| M3 Web server | `web_server.cpp`, `airLevel()` | `web_page.h` |
| M4 CoreIOT | `coreiot.cpp` (MQTT, RPC, backoff, telemetry) | dashboard, `getFan` cho widget |
| M5 RuleChain | `applySharedAttributes()` trong `coreiot.cpp` | `coreiot/rulechain/*.js` |
| M6 OTA | `ota.cpp` | build bản 1.0.1, `ota_checksum.py`, upload gói |
| M7 Gateway | `node_client.py`, `aggregator.py`, `storage.py`, RPC handler trong `gateway.py` | `offline_queue.py`, `coreiot_client.py`, `flush()` trong `gateway.py` |
| M8 Giao diện | `storage.read_recent()`, nút quạt trong `ui_app.py` | phần hiển thị trong `ui_app.py` |
| M9 Đo đạc | đo ESP-NOW, OTA | `measure_rpc_latency.py` |

**Git:** mỗi người làm trên branch riêng (`feat/m4-a-mqtt`), commit nhỏ, mở pull request để người kia review rồi mới merge vào `main`. Commit của cả hai người là điều kiện chấm điểm.

---

## 8. Thứ tự test đề xuất

1. `pio run` build được Node M và Node S (dù còn TODO).
2. M1: serial in số đo; relay kêu tách khi gọi `fanSet`.
3. M4: `fake_telemetry.py` → dashboard có số → thay bằng Node M thật → nút quạt chạy.
4. M5: `fake_telemetry.py --scenario rise` → quan sát RPC `setFan` và alarm → thử `window`, `steady`.
5. M6: chạy 1.0.0 → gán gói 1.0.1 → theo dõi `fw_state` → thử gói checksum sai phải ra `FAILED`.
6. M2: mở/đóng cửa → `window_open` đổi trên dashboard; đo mất gói 5 phút ở 2 khoảng cách.
7. M3: rút cáp WAN của router, mở trang Node M trên điện thoại, bấm quạt.
8. M7: `mock_node_m.py` + `gateway.py` → board thật → ngắt mạng 3 phút rồi nối lại, kiểm tra dữ liệu gửi bù.
9. M8: `streamlit run ui_app.py`.
10. Test lại OTA trên **firmware cuối cùng** trước khi quay video.

---

## 9. Lỗi thường gặp

| Hiện tượng | Nguyên nhân hay gặp | Cách xử lý |
| --- | --- | --- |
| MQTT `state=5` | sai token | chép lại access token, kiểm tra `secrets.h` |
| MQTT `state=-2` | sai host/port hoặc mạng chặn cổng | lấy host/port từ ví dụ trên LMS; thử WiFi điện thoại |
| OTA dừng ở chunk 0 | buffer MQTT nhỏ hơn chunk | `setBufferSize(MQTT_BUFFER_SIZE)` phải được gọi trước `connect` |
| `Update.begin` lỗi | thiếu bảng phân vùng 2 slot hoặc firmware quá lớn | kiểm tra `board_build.partitions`, kích thước `app0/app1` |
| ESP-NOW không nhận | khác kênh WiFi | cố định kênh router, đặt `ESPNOW_CHANNEL` đúng |
| Dashboard trống dù MQTT kết nối | xóa mất node `Save timeseries` trong RuleChain | nối lại node đó |
| Quạt bật tắt liên tục | thiếu `fan_state` trong telemetry hoặc 2 ngưỡng quá gần | gửi `fan_state` mỗi gói; giữ khoảng cách ≥ 200 ppm |
| Lỡ push token lên GitHub | `secrets.h` / `.env` bị commit | đổi token trên CoreIOT ngay, xóa file khỏi repo |
