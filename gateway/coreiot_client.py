"""
coreiot_client.py — Kết nối MQTT của gateway tới CoreIOT (module M7, Bạn B).

Gateway là một device riêng tên "air-gateway" trên CoreIOT (token riêng).
  - Gửi telemetry tổng hợp:  v1/devices/me/telemetry
      payload dạng có timestamp: {"ts": 1700000060000, "values": {...}}
  - Nhận RPC:                v1/devices/me/rpc/request/+   ({"method":"setFan","params":true})
  - Trả lời RPC:             v1/devices/me/rpc/response/{id}

Thuật toán / kỹ thuật:
  - paho-mqtt chạy vòng mạng ở luồng nền (loop_start) và TỰ kết nối lại
    (reconnect_delay_set: backoff 1 s -> 30 s).
  - Gửi qua hàng đợi: luôn push vào OfflineQueue trước, rồi flush() gửi lần lượt
    khi đang kết nối => cùng một đường đi cho cả lúc có mạng và lúc mất mạng.
  - RPC xử lý bằng hàm callback do gateway.py truyền vào (dependency injection),
    nên file này không cần biết Node M là gì.
"""
from __future__ import annotations

import json
import logging
from typing import Callable

import paho.mqtt.client as mqtt

import config
from offline_queue import OfflineQueue

log = logging.getLogger("coreiot")

TOPIC_TELEMETRY = "v1/devices/me/telemetry"
TOPIC_RPC_REQ = "v1/devices/me/rpc/request/+"
TOPIC_RPC_RESP = "v1/devices/me/rpc/response/{}"

RpcHandler = Callable[[str, object], dict]   # (method, params) -> dict trả lời


class CoreIotClient:
    def __init__(self, queue: OfflineQueue, rpc_handler: RpcHandler):
        self.queue = queue
        self.rpc_handler = rpc_handler
        self.connected = False
        self.client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2, client_id="air-gateway-py")
        # ThingsBoard: username = access token, không có password
        self.client.username_pw_set(config.GATEWAY_TOKEN)
        self.client.on_connect = self._on_connect
        self.client.on_disconnect = self._on_disconnect
        self.client.on_message = self._on_message
        self.client.reconnect_delay_set(min_delay=1, max_delay=30)

    # ---------------------------------------------------------------- kết nối
    def start(self) -> None:
        if not config.GATEWAY_TOKEN:
            raise RuntimeError("Chua dat GATEWAY_TOKEN trong .env")
        self.client.connect_async(config.COREIOT_HOST, config.COREIOT_PORT, keepalive=30)
        self.client.loop_start()

    def stop(self) -> None:
        self.client.loop_stop()
        self.client.disconnect()

    def _on_connect(self, client, userdata, flags, reason_code, properties):
        if reason_code != 0:
            log.error("ket noi CoreIOT that bai: %s", reason_code)
            return
        self.connected = True
        log.info("da ket noi CoreIOT")
        # TODO[M7-B]: client.subscribe(TOPIC_RPC_REQ, qos=1)

    def _on_disconnect(self, client, userdata, flags, reason_code, properties):
        self.connected = False
        log.warning("mat ket noi CoreIOT: %s", reason_code)

    # ---------------------------------------------------------------- RPC
    def _on_message(self, client, userdata, msg):
        if not msg.topic.startswith("v1/devices/me/rpc/request/"):
            return
        req_id = msg.topic.rsplit("/", 1)[-1]
        try:
            req = json.loads(msg.payload.decode())
        except ValueError:
            log.warning("RPC JSON loi: %r", msg.payload)
            return
        method = req.get("method", "")
        params = req.get("params")
        log.info("RPC %s %s", method, params)
        # TODO[M7-B]:
        #   resp = self.rpc_handler(method, params)
        #   client.publish(TOPIC_RPC_RESP.format(req_id), json.dumps(resp), qos=1)
        # Lưu ý: callback này chạy trong luồng MQTT; rpc_handler gọi HTTP tới Node M
        # (có timeout 3 s) nên vẫn chấp nhận được. Nếu muốn chặt chẽ, đẩy vào queue.Queue.
        _ = (req_id, method, params)

    # ---------------------------------------------------------------- gửi
    def send_telemetry(self, item: dict) -> None:
        """item = {"ts": ms, "values": {...}}. Luôn vào hàng đợi rồi flush."""
        self.queue.push(item)
        self.flush()

    def flush(self, batch: int = 50) -> int:
        """Gửi bù các phần tử đang chờ. Trả số phần tử đã gửi."""
        if not self.connected:
            return 0
        sent = 0
        # TODO[M7-B]:
        #   while True:
        #       items = self.queue.peek_batch(batch)
        #       if not items: break
        #       # ThingsBoard nhận được mảng [{"ts":..,"values":..}, ...] trong 1 lần publish
        #       info = self.client.publish(TOPIC_TELEMETRY, json.dumps(items), qos=1)
        #       if info.rc != mqtt.MQTT_ERR_SUCCESS: break
        #       self.queue.ack(len(items)); sent += len(items)
        return sent
