import socket
import hashlib
import struct
import sys

# ESP32_IP = "192.168.4.1"  # Replace with your ESP32 IP
ESP32_IP = "192.168.178.128"
ESP32_PORT = 8050
FIRMWARE_FILE = "build/hakkou.bin"
MAGIC_HEADER = b"HakkouOTA!!!"

def send_firmware(ip: str, port: int, firmware_path: str):
    try:
        with open(firmware_path, "rb") as f:
            firmware_data = f.read()
    except FileNotFoundError:
        print(f"[!] Firmware file not found: {firmware_path}")
        sys.exit(1)

    firmware_size = len(firmware_data)
    firmware_hash = hashlib.sha256(firmware_data).digest()
    magic = b'OTAv'

    print(MAGIC_HEADER, len(MAGIC_HEADER), f"<{len(MAGIC_HEADER)}sI32s")
    header = struct.pack(f"<{len(MAGIC_HEADER)}sI32s", MAGIC_HEADER, firmware_size, firmware_hash)

    print(f"[+] Connecting to {ip}:{port}")
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
        sock.connect((ip, port))
        print(f"[+] Connected. Sending OTA header...")
        sock.sendall(header)
        print(f"[+] Header sent. Sending firmware ({firmware_size} bytes)...")
        sock.sendall(firmware_data)
        print(f"[✓] Firmware sent successfully.")
        print(f"    SHA256: {firmware_hash.hex()}")

if __name__ == "__main__":
    send_firmware(ESP32_IP, ESP32_PORT, FIRMWARE_FILE)