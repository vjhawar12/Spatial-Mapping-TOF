import serial

baud_rate = 115200
port = "COM3"


def filter_string(string):
    string = string.strip()
    line = string.split(",")
    try:
        line = [float(i) for i in line]
        position, angle, x, y = line
    except Exception as e:
        return None
    
    return position, x, y

lines = []

buffer = ""

with serial.Serial(port, baud_rate, timeout=5) as ser:
    line = ""

    print("Waiting for STARTDATA ... ")
    while line != "STARTDATA":
        line = ser.readline().decode('utf-8').rstrip().lstrip()

    while line != "ENDDATA":
        line = ser.readline().decode('utf-8').rstrip().lstrip()
        
        try:
            position, x, y = filter_string(line)
        except Exception:
            print("Skipping line")
            continue            

        print(line)
        lines.append([position, x, y])
        buffer += f"{position},{x},{y}\n"


if len(lines) == 0:
    raise RuntimeError("No valid points received.") 


with open('points.txt', 'w') as f:
    f.write(buffer)

print("\nWritten to points.txt\n")

