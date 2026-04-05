import numpy as np
import open3d as o3d
from collections import defaultdict


MAX_NEIGHBORS = 4         # max nearest neighbors checked per point
SLICE_BIN_SIZE = 8.0      # group noisy positions into nearby slices

points = []
positions_to_indices = defaultdict(list)

with open("table.txt", "r") as f:
    for line in f:
        line = line.strip()
        if not line:
            continue

        parts = line.split(",")
        if len(parts) != 3:
            print(f"Skipping bad line: {line!r}")
            continue

        try:
            position = float(parts[0])
            x = float(parts[1])
            y = float(parts[2])
        except ValueError:
            print(f"Skipping non-numeric line: {line!r}")
            continue

        idx = len(points)
        points.append([position, x, y])

        # bin nearby positions together so noisy slices still group nicely
        pos_key = round(position / SLICE_BIN_SIZE) * SLICE_BIN_SIZE
        positions_to_indices[pos_key].append(idx)

if len(points) == 0:
    raise RuntimeError("No valid points found in examplepoints.txt")


points = np.array(points, dtype=np.float32)

# auto-scale based on data spread
bbox = np.max(points, axis=0) - np.min(points, axis=0)
scale = np.linalg.norm(bbox)
MAX_DIST = 0.02 * scale   # 2% of scene size


pcd = o3d.geometry.PointCloud()
pcd.points = o3d.utility.Vector3dVector(points)

# optional coloring by slice
colors = np.zeros((len(points), 3), dtype=np.float32)
sorted_positions = sorted(positions_to_indices.keys())

for s, pos in enumerate(sorted_positions):
    t = s / max(1, len(sorted_positions) - 1)
    for idx in positions_to_indices[pos]:
        colors[idx] = [t, 1.0 - t, 1.0]

pcd.colors = o3d.utility.Vector3dVector(colors)

# connect only nearby points within each slice
lines = set()

for pos in sorted_positions:
    inds = positions_to_indices[pos]
    if len(inds) < 2:
        continue

    slice_points = points[inds]

    # make a temporary point cloud for just this slice
    slice_pcd = o3d.geometry.PointCloud()
    slice_pcd.points = o3d.utility.Vector3dVector(slice_points)
    slice_tree = o3d.geometry.KDTreeFlann(slice_pcd)

    for local_i in range(len(slice_points)):
        [k, idxs, dists2] = slice_tree.search_knn_vector_3d(
            slice_points[local_i], MAX_NEIGHBORS + 1
        )

        for local_j, d2 in zip(idxs[1:], dists2[1:]):  # skip self
            dist = np.sqrt(d2)
            if dist <= MAX_DIST:
                global_i = inds[local_i]
                global_j = inds[local_j]
                a, b = sorted((global_i, global_j))
                lines.add((a, b))

lines = list(lines)

line_set = o3d.geometry.LineSet()
line_set.points = o3d.utility.Vector3dVector(points)
line_set.lines = o3d.utility.Vector2iVector(lines)

axis = o3d.geometry.TriangleMesh.create_coordinate_frame(size=50.0)

o3d.visualization.draw_geometries(
    [pcd, line_set, axis],
    window_name="Hallway Scan"
)