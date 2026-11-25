"""
config.py — Đọc cấu hình gateway từ file .env (module M7).

Mọi file khác import từ đây, không tự đọc biến môi trường, để có một nơi duy
nhất chứa cấu hình.
"""
import os
from pathlib import Path

from dotenv import load_dotenv

BASE_DIR = Path(__file__).resolve().parent
load_dotenv(BASE_DIR / ".env")

NODE_M_URL = os.getenv("NODE_M_URL", "http://127.0.0.1:5000").rstrip("/")
HTTP_TIMEOUT_S = float(os.getenv("HTTP_TIMEOUT_S", "3"))

COREIOT_HOST = os.getenv("COREIOT_HOST", "app.coreiot.io")
COREIOT_PORT = int(os.getenv("COREIOT_PORT", "1883"))
GATEWAY_TOKEN = os.getenv("GATEWAY_TOKEN", "")

POLL_INTERVAL_S = float(os.getenv("POLL_INTERVAL_S", "5"))
AGG_WINDOW_S = int(os.getenv("AGG_WINDOW_S", "60"))  # cửa sổ tổng hợp 1 phút

DATA_DIR = BASE_DIR / os.getenv("DATA_DIR", "data")
DATA_DIR.mkdir(parents=True, exist_ok=True)
QUEUE_FILE = DATA_DIR / "offline_queue.jsonl"
QUEUE_MAX_ITEMS = int(os.getenv("QUEUE_MAX_ITEMS", "5000"))
STATUS_FILE = DATA_DIR / "gateway_status.json"   # giao diện (M8) đọc file này
