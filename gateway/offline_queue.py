"""
offline_queue.py — Hàng đợi gửi bù khi mất internet (module M7, Bạn B).

Thuật toán: lưu-rồi-chuyển (store-and-forward) với hàng đợi FIFO trên đĩa.
  - push(item): nối 1 dòng JSON vào file .jsonl (sống sót qua khởi động lại).
  - Giới hạn kích thước: quá QUEUE_MAX_ITEMS thì bỏ phần tử CŨ NHẤT
    (drop-oldest), giữ số liệu mới.
  - Khi có mạng lại: peek_batch(k) lấy k phần tử đầu, gửi thành công thì
    ack(k) xóa chúng khỏi đầu hàng đợi. Gửi lỗi thì giữ nguyên để lần sau thử
    (đảm bảo "ít nhất một lần" — at-least-once).
  - Mỗi phần tử mang timestamp gốc {"ts":..., "values":{...}} nên ThingsBoard
    vẽ đúng thời điểm, không bị dồn cục lúc gửi bù.
  - Khóa threading.Lock vì luồng MQTT và luồng chính cùng truy cập.
"""
from __future__ import annotations

import json
import threading
from pathlib import Path

import config


class OfflineQueue:
    def __init__(self, path: Path = config.QUEUE_FILE, max_items: int = config.QUEUE_MAX_ITEMS):
        self.path = path
        self.max_items = max_items
        self._lock = threading.Lock()
        self._items: list[dict] = self._load()

    def _load(self) -> list[dict]:
        if not self.path.exists():
            return []
        items = []
        # TODO[M7-B]: đọc từng dòng, json.loads, bỏ qua dòng hỏng (JSONDecodeError)
        return items

    def _save(self) -> None:
        # Ghi ra file tạm rồi đổi tên => không hỏng file nếu tắt giữa chừng (atomic replace)
        tmp = self.path.with_suffix(".tmp")
        with tmp.open("w", encoding="utf-8") as f:
            for it in self._items:
                f.write(json.dumps(it) + "\n")
        tmp.replace(self.path)

    def push(self, item: dict) -> None:
        with self._lock:
            # TODO[M7-B]: append item; nếu len > max_items thì cắt bỏ phần tử cũ nhất; self._save()
            pass

    def peek_batch(self, k: int = 50) -> list[dict]:
        with self._lock:
            return list(self._items[:k])

    def ack(self, k: int) -> None:
        with self._lock:
            # TODO[M7-B]: xóa k phần tử đầu, self._save()
            pass

    def __len__(self) -> int:
        with self._lock:
            return len(self._items)
