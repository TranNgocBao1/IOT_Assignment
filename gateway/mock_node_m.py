"""
mock_node_m.py — Node M GIẢ bằng Flask, để làm gateway (M7) và giao diện (M8)
khi chưa có board hoặc board đang bận nạp code.

Chạy:   python mock_node_m.py        (mặc định http://127.0.0.1:5000)
Rồi đặt NODE_M_URL=http://127.0.0.1:5000 trong .env

Trả JSON GIỐNG HỆT Node M thật (docs/interfaces.md), nên đổi sang board thật
chỉ cần đổi URL.

Mô phỏng CO2 bằng bước ngẫu nhiên có xu hướng (random walk with drift):
    co2 += drift + noise
    drift = +8 ppm/lần khi quạt tắt và cửa đóng (người thở làm CO2 tăng)
            -25 ppm/lần khi quạt bật hoặc cửa mở
    kẹp co2 trong [420, 3000]
"""
from __future__ import annotations

import random
import time

from flask import Flask, jsonify, request

app = Flask(__name__)

state = {
    "co2": 650.0,
    "temperature": 29.0,
    "humidity": 65.0,
    "window_open": False,
    "fan_state": False,
    "fw_version": "mock-1.0.0",
}
_last_switch = 0.0
FAN_MIN_SWITCH_S = 10


def _level(co2: float) -> str:
    if co2 < 1000:
        return "good"
    if co2 <= 1500:
        return "fair"
    return "poor"


def _step() -> None:
    venting = state["fan_state"] or state["window_open"]
    drift = -25 if venting else 8
    state["co2"] = min(3000, max(420, state["co2"] + drift + random.gauss(0, 5)))
    state["temperature"] += random.gauss(0, 0.05)
    state["humidity"] += random.gauss(0, 0.2)


@app.get("/api/data")
def api_data():
    _step()
    d = {k: (round(v, 1) if isinstance(v, float) else v) for k, v in state.items()}
    d["level"] = _level(state["co2"])
    return jsonify(d)


@app.get("/api/fan")
def api_fan():
    global _last_switch
    on = request.args.get("on")
    if on not in ("0", "1"):
        return jsonify(error="on phai la 0 hoac 1"), 400
    want = on == "1"
    if want != state["fan_state"]:
        if time.time() - _last_switch < FAN_MIN_SWITCH_S:
            return jsonify(ok=False, fan_state=state["fan_state"]), 409
        state["fan_state"] = want
        _last_switch = time.time()
    return jsonify(ok=True, fan_state=state["fan_state"])


@app.get("/mock/window")
def mock_window():
    """Chỉ có ở bản giả: /mock/window?open=1 để giả lập mở cửa."""
    state["window_open"] = request.args.get("open") == "1"
    return jsonify(window_open=state["window_open"])


# TODO[M7-A] (tùy chọn): thêm /mock/co2?value=1600 để ép CO2 lên cao khi test cảnh báo.

if __name__ == "__main__":
    app.run(host="0.0.0.0", port=5000, debug=False)
