import re
import serial
import time
from datetime import datetime

COM_PORT = "/dev/ttyACM0"
BAUD_RATE = 115200
TIMEOUT = 0.2
IDLE_TIMEOUT = 5.0

timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
OUTFILE = f"scan_{timestamp}.txt"

# x,angle,distance
SCAN_LINE_RE = re.compile(r"^\s*(-?\d+),(-?\d+(?:\.\d+)?),(\d+)\s*$")

def is_scan_line(line: str) -> bool:
    return bool(SCAN_LINE_RE.match(line))

def main():
    print(f"Opening {COM_PORT} at {BAUD_RATE} baud...")
    print(f"Saving capture to: {OUTFILE}")

    try:
        with serial.Serial(COM_PORT, BAUD_RATE, timeout=TIMEOUT) as ser, open(OUTFILE, "w") as f:
            time.sleep(2.0)
            ser.reset_input_buffer()

            print("Waiting for first valid scan line...")
            capturing = False
            last_valid_time = None
            valid_count = 0

            while True:
                try:
                    raw = ser.readline()
                except serial.SerialException as e:
                    print(f"\n[ERROR] Serial exception: {e}")
                    break

                if not raw:
                    if capturing and last_valid_time is not None:
                        if time.time() - last_valid_time > IDLE_TIMEOUT:
                            print(f"\n=== Capture stopped: idle timeout ({IDLE_TIMEOUT}s) ===")
                            print(f"Saved to {OUTFILE}")
                            break
                    continue

                line = raw.decode("utf-8", errors="ignore").strip()
                if not line:
                    continue

                print(repr(line))

                if not is_scan_line(line):
                    continue

                if not capturing:
                    capturing = True
                    print("\n=== First valid scan line seen: capture started ===\n")

                f.write(line + "\n")
                f.flush()
                valid_count += 1
                last_valid_time = time.time()

            print(f"Valid scan lines captured: {valid_count}")

    except KeyboardInterrupt:
        print("\nCapture interrupted by user.")
        print(f"Partial data saved to {OUTFILE}")
    except serial.SerialException as e:
        print(f"\n[ERROR] Could not open serial port: {e}")

if __name__ == "__main__":
    main()