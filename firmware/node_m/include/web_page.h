// =============================================================================
// web_page.h — Trang HTML + JavaScript nhúng trong firmware (module M3, Bạn B)
//
// Trang TỰ VIẾT (yêu cầu T07), không dùng template có sẵn.
// Kỹ thuật:
//  - Polling định kỳ: setInterval(refresh, 3000) gọi fetch('/api/data').
//  - Cập nhật DOM theo JSON, đổi màu theo level (good/fair/poor).
//  - Nút quạt gọi fetch('/api/fan?on=1|0') rồi refresh ngay.
// Lưu trong flash bằng PROGMEM để không tốn RAM.
// =============================================================================
#pragma once
#include <Arduino.h>

static const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="vi">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Chất lượng không khí</title>
<style>
  body { font-family: system-ui, sans-serif; margin: 0; padding: 16px; background: #f4f5f7; }
  .card { background: #fff; border-radius: 12px; padding: 16px; margin-bottom: 12px; }
  .big  { font-size: 48px; font-weight: 700; }
  .good { color: #1a7f37; } .fair { color: #b58100; } .poor { color: #c62828; }
  /* TODO[M3-B]: tự thiết kế thêm giao diện (lưới các ô, icon cửa sổ, trạng thái quạt...) */
</style>
</head>
<body>
  <h2>Giám sát chất lượng không khí</h2>

  <div class="card">
    <div>CO2 (ppm)</div>
    <div id="co2" class="big">--</div>
    <div id="level">--</div>
  </div>

  <div class="card">
    <div>Nhiệt độ: <b id="temp">--</b> °C</div>
    <div>Độ ẩm: <b id="hum">--</b> %</div>
    <div>Cửa sổ: <b id="win">--</b></div>
    <div>Quạt: <b id="fan">--</b></div>
  </div>

  <div class="card">
    <button onclick="setFan(1)">Bật quạt</button>
    <button onclick="setFan(0)">Tắt quạt</button>
    <div id="msg"></div>
  </div>

  <small>Firmware: <span id="fw">--</span></small>

<script>
async function refresh() {
  try {
    const r = await fetch('/api/data');
    const d = await r.json();
    // TODO[M3-B]:
    //  - ghi d.co2, d.temperature, d.humidity vào các ô (null -> hiện "--")
    //  - d.window_open -> "Mở" / "Đóng"; d.fan_state -> "Đang chạy" / "Tắt"
    //  - đổi class của #co2 theo d.level ('good' | 'fair' | 'poor')
    //  - ghi d.fw_version vào #fw
  } catch (e) {
    document.getElementById('msg').textContent = 'Mất kết nối với thiết bị';
  }
}

async function setFan(on) {
  // TODO[M3-B]:
  //  - gọi fetch('/api/fan?on=' + on)
  //  - nếu HTTP 409: báo "Đổi trạng thái quá nhanh, thử lại sau vài giây"
  //  - sau đó gọi refresh()
}

refresh();
setInterval(refresh, 3000);
</script>
</body>
</html>
)rawliteral";
