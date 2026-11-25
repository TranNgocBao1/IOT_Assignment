"""
fake_telemetry.py — Gửi telemetry GIẢ lên CoreIOT để dựng dashboard (M4) và
test RuleChain / cảnh báo (M5) khi chưa có phần cứng.

Chạy:
    python fake_telemetry.py --token <TOKEN_air-node-m> --scenario rise

Kịch bản (scenario):
    rise    : CO2 tăng 600 -> 1700 rồi giảm về 700 (qua cả 3 ngưỡng 800/1000/1500)
    window  : CO2 cao nhưng cửa mở (RuleChain KHÔNG được bật quạt)
    steady  : CO2 dao động quanh 900 (vùng trễ, quạt không được đổi trạng thái)

Mô phỏng phản hồi của quạt: script cũng subscribe RPC; nhận setFan thì đổi
fan_state giả và trả lời, giống Node M thật => thấy được vòng kín RuleChain.
"""
from __future__ import annotations

import argparse
import json
import time

import paho.mqtt.client as mqtt

state = {"fan_state": False}


def scenario_values(name: str):
    if name == "rise":
        for v in list(range(600, 1701, 50)) + list(range(1700, 699, -50)):
            yield v, False
    elif name == "window":
        for v in range(1000, 1401, 50):
            yield v, True
    elif name == "steady":
        import random
        for _ in range(30):
            yield 900 + random.randint(-80, 80), False
    else:
        raise SystemExit(f"scenario khong hop le: {name}")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--host", default="app.coreiot.io")
    ap.add_argument("--port", type=int, default=1883)
    ap.add_argument("--token", required=True)
    ap.add_argument("--scenario", default="rise")
    ap.add_argument("--interval", type=float, default=5)
    a = ap.parse_args()

    c = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)
    c.username_pw_set(a.token)

    def on_message(client, userdata, msg):
        req_id = msg.topic.rsplit("/", 1)[-1]
        req = json.loads(msg.payload)
        if req.get("method") == "setFan":
            p = req.get("params")
            state["fan_state"] = p.get("on") if isinstance(p, dict) else bool(p)
            print(f"  <- RPC setFan {state['fan_state']}")
        client.publish(f"v1/devices/me/rpc/response/{req_id}", json.dumps({"ok": True}))

    c.on_message = on_message
    c.connect(a.host, a.port, 30)
    c.subscribe("v1/devices/me/rpc/request/+")
    c.loop_start()

    for co2, window_open in scenario_values(a.scenario):
        payload = {
            "co2": co2, "temperature": 29.5, "humidity": 63,
            "window_open": window_open, "fan_state": state["fan_state"], "fw_version": "fake",
        }
        c.publish("v1/devices/me/telemetry", json.dumps(payload))
        print(f"-> co2={co2} window_open={window_open} fan_state={state['fan_state']}")
        time.sleep(a.interval)

    c.loop_stop()
    c.disconnect()


if __name__ == "__main__":
    main()
