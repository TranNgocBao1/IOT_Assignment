"""
aggregator.py — Tổng hợp số liệu theo cửa sổ thời gian 1 phút (module M7, Bạn A).

Thuật toán: cửa sổ cuộn không chồng lấn (tumbling window).
  - Mỗi mẫu có timestamp; cửa sổ được xác định bằng
        window_start = floor(ts / W) * W          (W = 60 s)
  - Mẫu cùng window_start thì cộng dồn: sum, count, max.
  - Khi mẫu đầu tiên của cửa sổ MỚI tới -> "đóng" cửa sổ cũ, tính
        avg = sum / count
    và trả kết quả để gửi lên CoreIOT; rồi bắt đầu cửa sổ mới.
  - Chạy trực tuyến (online), O(1) bộ nhớ: không lưu toàn bộ mẫu.
  - Bỏ qua giá trị None (cảm biến chưa sẵn sàng).

Kết quả gửi lên CoreIOT (khớp docs/interfaces.md):
    co2_avg_1m, co2_max_1m, temperature_avg_1m   (+ tùy chọn humidity_avg_1m, samples)
"""
from __future__ import annotations

from dataclasses import dataclass, field

import config


@dataclass
class _Acc:
    """Bộ cộng dồn cho 1 đại lượng."""
    total: float = 0.0
    count: int = 0
    maximum: float = float("-inf")

    def add(self, x: float | None) -> None:
        if x is None:
            return
        # TODO[M7-A]: cộng total, tăng count, cập nhật maximum
        pass

    def avg(self) -> float | None:
        # TODO[M7-A]: trả total / count, count == 0 thì None
        return None


@dataclass
class MinuteAggregator:
    window_s: int = config.AGG_WINDOW_S
    _start: int | None = None
    _co2: _Acc = field(default_factory=_Acc)
    _temp: _Acc = field(default_factory=_Acc)
    _hum: _Acc = field(default_factory=_Acc)

    def _window_of(self, ts: float) -> int:
        return int(ts // self.window_s) * self.window_s

    def _close(self) -> dict | None:
        """Đóng cửa sổ hiện tại, trả dict kết quả (hoặc None nếu rỗng)."""
        if self._start is None or self._co2.count == 0:
            return None
        # TODO[M7-A]: trả dict:
        #   {"ts": (self._start + self.window_s) * 1000,   # ms, ThingsBoard dùng mili-giây
        #    "values": {"co2_avg_1m": round(avg,1), "co2_max_1m": max,
        #               "temperature_avg_1m": ..., "humidity_avg_1m": ...,
        #               "samples": count}}
        return None

    def add(self, sample: dict, ts: float) -> dict | None:
        """Thêm 1 mẫu. Trả kết quả của cửa sổ VỪA ĐÓNG nếu có, ngược lại None."""
        w = self._window_of(ts)
        result = None
        if self._start is not None and w != self._start:
            result = self._close()
            self._co2, self._temp, self._hum = _Acc(), _Acc(), _Acc()
        self._start = w
        self._co2.add(sample.get("co2"))
        self._temp.add(sample.get("temperature"))
        self._hum.add(sample.get("humidity"))
        return result


if __name__ == "__main__":
    # Tự kiểm tra nhanh: python aggregator.py
    agg = MinuteAggregator(window_s=60)
    for i in range(130):  # 130 mẫu, mỗi mẫu cách 1 s => phải đóng 2 cửa sổ
        out = agg.add({"co2": 800 + i, "temperature": 28.0, "humidity": 60.0}, ts=1_700_000_000 + i)
        if out:
            print(out)
    # TODO[M7-A]: kỳ vọng in ra 2 dòng, co2_max_1m tăng dần. Thêm assert để tự kiểm tra.
