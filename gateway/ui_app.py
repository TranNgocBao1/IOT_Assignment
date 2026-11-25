"""
ui_app.py — Giao diện gateway trên PC bằng Streamlit (module M8, T10).

Chạy:   streamlit run ui_app.py
(Chạy song song với gateway.py ở cửa sổ terminal khác.)

Nguồn dữ liệu:
  - data/gateway_status.json  (gateway.py ghi mỗi chu kỳ)  -> trạng thái hiện tại
  - data/raw_YYYY-MM-DD.csv   (storage.py)                 -> biểu đồ lịch sử
  - Nút quạt gọi thẳng Node M qua NodeMClient.set_fan()
Giao diện KHÔNG tự kết nối CoreIOT (đúng phạm vi T10: giao diện của gateway).

Kỹ thuật:
  - Làm mới định kỳ: st.fragment(run_every=...) chạy lại phần hiển thị mỗi 5 s
    (Streamlit >= 1.37). Bản cũ hơn: dùng time.sleep + st.rerun().
  - Tô màu theo mức chất lượng (good/fair/poor) giống trang web trên board.
"""
from __future__ import annotations

import json

import streamlit as st

import config
from node_client import NodeMClient
from storage import CsvStorage

st.set_page_config(page_title="Gateway - Chất lượng không khí", layout="wide")
st.title("Gateway: giám sát chất lượng không khí")

node = NodeMClient()
storage = CsvStorage()

LEVEL_COLOR = {"good": "🟢 Tốt", "fair": "🟡 Trung bình", "poor": "🔴 Kém"}


def read_status() -> dict:
    try:
        return json.loads(config.STATUS_FILE.read_text(encoding="utf-8"))
    except (FileNotFoundError, ValueError):
        return {}


@st.fragment(run_every=5)
def live_panel():
    status = read_status()
    sample = status.get("last_sample") or {}

    # --- Hàng trạng thái kết nối ---
    c1, c2, c3 = st.columns(3)
    c1.metric("Node M", "online" if status.get("node_online") else "offline")
    c2.metric("CoreIOT", "đã kết nối" if status.get("cloud_connected") else "mất kết nối")
    c3.metric("Hàng đợi gửi bù", status.get("queue_len", 0))

    # --- Số liệu hiện tại ---
    # TODO[M8-B]: 4 cột st.metric: CO2 (kèm LEVEL_COLOR theo sample["level"]),
    #             nhiệt độ, độ ẩm, cửa sổ (Mở/Đóng), quạt (Bật/Tắt)

    # --- Biểu đồ lịch sử ---
    df = storage.read_recent(720)
    if df.empty:
        st.info("Chưa có dữ liệu CSV hôm nay.")
    else:
        # TODO[M8-B]: st.line_chart(df.set_index("ts")[["co2"]]) và biểu đồ nhiệt độ/độ ẩm
        #             Vẽ thêm đường ngưỡng 1000 / 1500 nếu muốn (dùng st.altair_chart).
        pass

    st.caption(f"Cập nhật: {status.get('updated', '--')}")


live_panel()

# --- Điều khiển quạt ---
st.subheader("Điều khiển quạt")
b1, b2 = st.columns(2)
if b1.button("Bật quạt", use_container_width=True):
    # TODO[M8-A]: ok = node.set_fan(True); st.success / st.error theo ok
    pass
if b2.button("Tắt quạt", use_container_width=True):
    # TODO[M8-A]: tương tự với False
    pass

# TODO[M8-A] (tùy chọn): ô chọn khoảng thời gian (30 phút / 1 giờ / cả ngày) để lọc df.
