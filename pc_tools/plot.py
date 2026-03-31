import sys
import re
import glob
import os
import numpy as np
import matplotlib.pyplot as plt
from math import radians, cos, sin

SCAN_LINE_RE = re.compile(r'^\s*-?\d+(?:\.\d+)?\s*,\s*-?\d+(?:\.\d+)?\s*,\s*-?\d+(?:\.\d+)?\s*$')

def get_input_files():
    if len(sys.argv) >= 2:
        return sys.argv[1:]

    files = glob.glob("scan_*.txt")
    files = sorted(files, key=os.path.getmtime)

    if not files:
        print("No scan_*.txt files found.")
        print("Usage: python plot_from_txt.py <scan_file1.txt> [scan_file2.txt ...]")
        sys.exit(1)

    print("Auto-detected scan files:")
    for f in files:
        print(" ", f)

    return files

def load_raw_data(files):
    raw_data = []

    for input_file in files:
        print(f"\nReading: {input_file}")
        with open(input_file, "r") as f:
            for raw_line in f:
                line = raw_line.strip()

                if not line:
                    continue

                if not SCAN_LINE_RE.match(line):
                    print(f"Skipping non-scan line: {repr(line)}")
                    continue

                parts = [p.strip() for p in line.split(",")]

                try:
                    x = float(parts[0])
                    theta = float(parts[1])
                    r = float(parts[2])

                    if theta < -360 or theta > 360:
                        print(f"Skipping impossible angle: {theta}")
                        continue

                    if r < 20 or r > 1200:
                        print(f"Skipping impossible distance: {r}")
                        continue

                    raw_data.append((x, theta, r))

                except ValueError:
                    print(f"Skipping malformed numeric line: {repr(line)}")
                    continue

    return raw_data

input_files = get_input_files()
raw_data = load_raw_data(input_files)

print(f"\nAccepted scan rows: {len(raw_data)}")

if len(raw_data) == 0:
    print("No valid scan rows found.")
    sys.exit(1)

x_groups_found = sorted(set(int(round(row[0])) for row in raw_data))
print("Detected X groups:", x_groups_found)

coords = []
for x, theta, r in raw_data:
    theta_rad = radians(theta)
    y = r * np.cos(theta_rad)
    z = r * np.sin(theta_rad)
    coords.append((x, y, z, theta, r))

points = np.array([[c[0], c[1], c[2]] for c in coords], dtype=float)

center = np.mean(points, axis=0)
dist_from_center = np.linalg.norm(points - center, axis=1)
threshold = np.mean(dist_from_center) + 2.0 * np.std(dist_from_center)

filtered_coords = [coords[i] for i in range(len(coords)) if dist_from_center[i] < threshold]

print(f"Points before outlier filter: {len(coords)}")
print(f"Points after outlier filter:  {len(filtered_coords)}")

if len(filtered_coords) == 0:
    print("All points filtered out.")
    sys.exit(1)

groups = {}
for x, y, z, theta, r in filtered_coords:
    x_group = int(round(x))
    groups.setdefault(x_group, []).append((x, y, z, theta, r))

for x_group in groups:
    groups[x_group].sort(key=lambda p: p[3])

xyz_outfile = "merged_scan.xyz" if len(input_files) > 1 else input_files[0].replace(".txt", ".xyz")

with open(xyz_outfile, "w") as f:
    for x_group in sorted(groups.keys()):
        for x, y, z, theta, r in groups[x_group]:
            f.write(f"{x} {y} {z}\n")

print(f"Saved XYZ to {xyz_outfile}")

fig = plt.figure(figsize=(9, 7))
ax = fig.add_subplot(111, projection="3d")

for x_group in sorted(groups.keys()):
    sweep = groups[x_group]
    xs = [p[0] for p in sweep]
    ys = [p[1] for p in sweep]
    zs = [p[2] for p in sweep]

    ax.scatter(xs, ys, zs, s=25, label=f"x = {x_group} mm")
    ax.plot(xs, ys, zs)

ax.set_xlabel("X (mm)")
ax.set_ylabel("Y (mm)")
ax.set_zlabel("Z (mm)")
ax.set_title("3D TOF Scan")
ax.grid(False)
ax.set_box_aspect([1, 1, 1])
ax.legend()

plt.show()