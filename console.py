import socket
import sys
import threading
import time

def receive(sock):
    try:
        while True:
            data = sock.recv(1024)
            if not data:
                print("\nConnection closed by remote host.")
                break
            print(data.decode(errors='replace'), end='', flush=True)
    except Exception as e:
        print(f"\nReceive error: {e}")

def connect_and_run(host, port):
    while True:
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        try:
            sock.connect((host, port))
            print(f"Connected to {host}:{port}. Type your messages below.\n")
            
            receiver_thread = threading.Thread(target=receive, args=(sock,), daemon=True)
            receiver_thread.start()

            try:
                while True:
                    line = sys.stdin.readline()
                    if not line:
                        break
                    sock.sendall(line.encode())
            except (KeyboardInterrupt, BrokenPipeError):
                print("\nDisconnected by user or broken pipe.")
                break
            finally:
                sock.close()

        except (ConnectionRefusedError, OSError) as e:
            print(f"Connection failed: {e}. Retrying in 3 seconds...")
            sock.close()
            time.sleep(3)
            continue

        print("Connection lost. Reconnecting in 3 seconds...")
        time.sleep(1)

def main():
    if len(sys.argv) != 3:
        print(f"Usage: {sys.argv[0]} <host> <port>")
        sys.exit(1)

    host = sys.argv[1]
    port = int(sys.argv[2])

    connect_and_run(host, port)

if __name__ == "__main__":
    main()