"""
ota_checksum.py — In kích thước và SHA-256 của file firmware .bin (module M6, Bạn B).

Dùng để đối chiếu với checksum CoreIOT tự tính khi upload gói OTA, và để ghi
vào báo cáo.

Chạy:
    python ota_checksum.py ../firmware/node_m/.pio/build/yolo_uno/firmware.bin

Thuật toán: băm SHA-256 theo khối 64 KB (đọc từng phần, không nạp cả file vào RAM)
— giống cách Node M băm tăng dần từng chunk khi nhận OTA.
"""
import hashlib
import sys
from pathlib import Path


def sha256_file(path: Path, block: int = 65536) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(block), b""):
            h.update(chunk)
    return h.hexdigest()


def main():
    if len(sys.argv) != 2:
        raise SystemExit("cach dung: python ota_checksum.py <firmware.bin>")
    p = Path(sys.argv[1])
    size = p.stat().st_size
    print(f"file     : {p}")
    print(f"size     : {size} byte ({size / 1024:.1f} KB)")
    print(f"chunks   : {-(-size // 4096)} (chunk 4096 byte)")
    print(f"SHA256   : {sha256_file(p)}")


if __name__ == "__main__":
    main()
