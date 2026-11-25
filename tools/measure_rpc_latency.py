"""
measure_rpc_latency.py — Đo độ trễ điều khiển quạt cho báo cáo (module M9, Bạn B).

Cách đo: gọi /api/fan của Node M (hoặc mock) N lần xen kẽ bật/tắt, mỗi lần
cách nhau > FAN_MIN_SWITCH (10 s), đo thời gian khứ hồi (round-trip time).
In min / trung bình / trung vị / p95.

Đo độ trễ đường CoreIOT (bấm nút dashboard -> relay chạy) thì làm thủ công
bằng video quay chậm hoặc so timestamp log serial — ghi cách đo vào báo cáo.

Chạy:
    python measure_rpc_latency.py --url http://192.168.1.50 --n 10
"""
from __future__ import annotations

import argparse
import statistics
import time

import requests


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--url", required=True)
    ap.add_argument("--n", type=int, default=10)
    ap.add_argument("--gap", type=float, default=11)
    a = ap.parse_args()

    rtts = []
    for i in range(a.n):
        on = i % 2 == 0
        t0 = time.perf_counter()
        r = requests.get(f"{a.url}/api/fan", params={"on": int(on)}, timeout=5)
        rtt = (time.perf_counter() - t0) * 1000
        print(f"{i + 1:2d}. on={int(on)}  HTTP {r.status_code}  {rtt:.0f} ms")
        if r.ok:
            rtts.append(rtt)
        time.sleep(a.gap)

    if not rtts:
        raise SystemExit("khong co lan do nao thanh cong")
    # TODO[M9-B]: tính và in thêm p95: sorted(rtts)[int(0.95 * (len(rtts) - 1))]
    print(f"\nmin {min(rtts):.0f} ms | tb {statistics.mean(rtts):.0f} ms | "
          f"trung vi {statistics.median(rtts):.0f} ms | n={len(rtts)}")


if __name__ == "__main__":
    main()
