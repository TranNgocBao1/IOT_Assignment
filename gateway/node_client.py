"""
node_client.py — Giao tiếp HTTP với Node M (module M7, phần Bạn A).

Gọi 2 API của Node M:
    GET /api/data           -> dict số liệu
    GET /api/fan?on=1|0     -> bật/tắt quạt

Thuật toán:
  - Thử lại có giới hạn (bounded retry) với thời gian chờ tăng dần
    (0.5 s, 1 s, ...) khi lỗi mạng tạm thời.
  - Timeout cho mọi request để gateway không bị treo khi Node M tắt.
"""
from __future__ import annotations

import logging
import time

import requests

import config

log = logging.getLogger("node")


class NodeMClient:
    def __init__(self, base_url: str = config.NODE_M_URL, timeout: float = config.HTTP_TIMEOUT_S):
        self.base_url = base_url
        self.timeout = timeout
        self.session = requests.Session()  # giữ kết nối TCP, nhanh hơn tạo mới mỗi lần

    def _get(self, path: str, params: dict | None = None, retries: int = 2) -> dict | None:
        """GET có thử lại. Trả dict JSON hoặc None nếu thất bại."""
        delay = 0.5
        for attempt in range(retries + 1):
            try:
                # TODO[M7-A]:
                #   r = self.session.get(self.base_url + path, params=params, timeout=self.timeout)
                #   r.raise_for_status()
                #   return r.json()
                raise NotImplementedError
            except NotImplementedError:
                raise
            except (requests.RequestException, ValueError) as e:
                log.warning("GET %s loi (lan %d): %s", path, attempt + 1, e)
                time.sleep(delay)
                delay *= 2
        return None

    def get_data(self) -> dict | None:
        """Đọc /api/data. Trả None nếu Node M không phản hồi."""
        data = self._get("/api/data")
        # TODO[M7-A]: kiểm tra có đủ key co2, temperature, humidity, fan_state;
        #             thiếu thì log cảnh báo và trả None.
        return data

    def set_fan(self, on: bool) -> bool:
        """Bật/tắt quạt qua Node M. Trả True nếu Node M xác nhận ok."""
        # TODO[M7-A]: gọi self._get("/api/fan", {"on": 1 if on else 0, "src": "gateway"}, retries=1)
        #             trả về bool(resp and resp.get("ok"))
        #   Lưu ý: Node M trả HTTP 409 khi đổi quá nhanh -> raise_for_status() ném lỗi,
        #   cân nhắc xử lý riêng mã 409 để không thử lại vô ích.
        return False
