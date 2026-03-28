import serial
from math import sin, cos, radians
import open3d as o3d

data = []
line = ""
file_path = "coordinates.xyz"
com_port = "/dev/ttyACM0"
baud_rate = 115200

with serial.Serial(com_port, baud_rate, timeout=1) as ser:
    while line != "ENDDATA":
        line = ser.readline().decode("utf-8").strip()
        print(line)
        if line == "ENDDATA":
            break
        data.append(line.split(','))
    

coords = []

for x, theta, r in data:
    x = float(x)
    theta_rad = radians(float(theta))
    r = float(r)

    coords.append((x, r * cos(theta_rad), r * sin((theta_rad)))) 

with open(file_path, "w") as f:
    for coord in coords:
        f.write(str(coord[0]) + " " + str(coord[1]) + " " + str(coord[2]) + "\n")


points = np.array(coords, dtype=float)
pcd = o3d.geometry.PointCloud()
pcd.points = o3d.utility.Vector3dVector(points)

o3d.visualization.draw_geometries([pcd])