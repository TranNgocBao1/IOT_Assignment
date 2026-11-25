"""
storage.py — Lưu số liệu thô ra CSV (module M7, Bạn A; M8 đọc lại).

Thuật toán / kỹ thuật:
  - Ghi nối (append-only) mỗi mẫu 1 dòng, không sửa dữ liệu cũ => an toàn khi
    tắt đột ngột (mất tối đa 1 dòng).
  - Xoay file theo ngày (daily rotation): data/raw_YYYY-MM-DD.csv, file không
    phình vô hạn và dễ lọc theo ngày.
  - Đọc N dòng cuối (tail) cho giao diện: dùng pandas đọc file hôm nay rồi lấy
    tail(n); dữ liệu 1 ngày ~17 nghìn dòng nên đủ nhanh.
"""
from __future__ import annotations

import csv
from datetime import datetime
from pathlib import Path

import config

FIELDS = ["ts", "co2", "temperature", "humidity", "window_open", "fan_state", "fw_version"]


class CsvStorage:
    def __init__(self, data_dir: Path = config.DATA_DIR):
        self.data_dir = data_dir

    def _path_for(self, dt: datetime) -> Path:
        return self.data_dir / f"raw_{dt:%Y-%m-%d}.csv"

    def append(self, sample: dict, ts: float) -> None:
        """Ghi 1 mẫu. Tự tạo header nếu file mới."""
        path = self._path_for(datetime.fromtimestamp(ts))
        is_new = not path.exists()
        row = {"ts": datetime.fromtimestamp(ts).isoformat(timespec="seconds")}
        row.update({k: sample.get(k) for k in FIELDS if k != "ts"})
        # TODO[M7-A]:
        #   with path.open("a", newline="", encoding="utf-8") as f:
        #       w = csv.DictWriter(f, fieldnames=FIELDS)
        #       if is_new: w.writeheader()
        #       w.writerow(row)
        _ = (csv, is_new, row)

    def read_recent(self, n: int = 720):
        """Trả DataFrame n dòng gần nhất của hôm nay (720 dòng ~ 1 giờ với chu kỳ 5 s)."""
        import pandas as pd

        path = self._path_for(datetime.now())
        if not path.exists():
            return pd.DataFrame(columns=FIELDS)
        # TODO[M8-A]: df = pd.read_csv(path, parse_dates=["ts"]); return df.tail(n)
        return pd.DataFrame(columns=FIELDS)
