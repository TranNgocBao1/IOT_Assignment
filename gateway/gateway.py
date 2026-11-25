"""
gateway.py — Chương trình chính của gateway trên PC (module M7, T08).

Chạy:   python gateway.py

Mỗi POLL_INTERVAL_S giây:
   1. hỏi Node M  GET /api/data           (node_client.py)
   2. lưu CSV                             (storage.py)
   3. đưa vào bộ tổng hợp 1 phút          (aggregator.py)
   4. hết phút -> gửi co2_avg_1m ... lên CoreIOT qua hàng đợi (coreiot_client.py)
   5. gửi bù dữ liệu tồn khi có mạng lại  (offline_queue.py)
Đồng thời: RPC "setFan" từ CoreIOT -> gọi Node M /api/fan.

Thuật toán: vòng lặp hỏi vòng định kỳ (periodic polling) có bù thời gian xử lý:
   sleep(max(0, interval - thời_gian_đã_dùng)) để chu kỳ đều, không trôi dần.
Ghi trạng thái ra data/gateway_status.json cho giao diện (M8) đọc.
"""
from __future__ import annotations

import json
import logging
import signal
import time

import config
from aggregator import MinuteAggregator
from coreiot_client import CoreIotClient
from node_client import NodeMClient
from offline_queue import OfflineQueue
from storage import CsvStorage

logging.basicConfig(level=logging.INFO, format="%(asctime)s %(name)-8s %(levelname)s %(message)s")
log = logging.getLogger("gateway")

_running = True


def _stop(*_):
    global _running
    _running = False


def make_rpc_handler(node: NodeMClient):
    """Tạo hàm xử lý RPC; truyền vào CoreIotClient."""

    def handle(method: str, params) -> dict:
        if method == "setFan":
            # params có thể là bool hoặc {"on": bool} tùy widget
            on = params.get("on") if isinstance(params, dict) else bool(params)
            # TODO[M7-A]: ok = node.set_fan(bool(on)); return {"ok": ok, "fan_state": bool(on) if ok else None}
            return {"ok": False, "error": "chua cai dat"}
        if method == "getFan":
            # TODO[M7-A]: đọc node.get_data() và trả {"fan_state": ...}
            return {"fan_state": None}
        return {"error": f"unknown method {method}"}

    return handle


def write_status(status: dict) -> None:
    tmp = config.STATUS_FILE.with_suffix(".tmp")
    tmp.write_text(json.dumps(status, ensure_ascii=False), encoding="utf-8")
    tmp.replace(config.STATUS_FILE)


def main() -> None:
    signal.signal(signal.SIGINT, _stop)
    signal.signal(signal.SIGTERM, _stop)

    node = NodeMClient()
    storage = CsvStorage()
    agg = MinuteAggregator()
    queue = OfflineQueue()
    cloud = CoreIotClient(queue, make_rpc_handler(node))
    cloud.start()
    log.info("gateway chay: Node M=%s, chu ky %.0f s, hang doi ton %d",
             config.NODE_M_URL, config.POLL_INTERVAL_S, len(queue))

    stats = {"polls": 0, "poll_fail": 0, "sent_minutes": 0}

    while _running:
        t0 = time.time()

        sample = node.get_data()
        if sample is None:
            stats["poll_fail"] += 1
        else:
            stats["polls"] += 1
            storage.append(sample, t0)
            minute = agg.add(sample, t0)
            if minute:
                log.info("tong hop 1 phut: %s", minute["values"])
                cloud.send_telemetry(minute)
                stats["sent_minutes"] += 1

        # TODO[M7-B]: gọi cloud.flush() định kỳ để gửi bù khi vừa có mạng lại
        #             (send_telemetry chỉ chạy mỗi phút một lần).

        write_status({
            "updated": time.strftime("%Y-%m-%d %H:%M:%S"),
            "node_online": sample is not None,
            "cloud_connected": cloud.connected,
            "queue_len": len(queue),
            "last_sample": sample,
            **stats,
        })

        elapsed = time.time() - t0
        time.sleep(max(0.0, config.POLL_INTERVAL_S - elapsed))

    cloud.stop()
    log.info("gateway dung")


if __name__ == "__main__":
    main()
