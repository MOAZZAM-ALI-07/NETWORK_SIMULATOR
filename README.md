# Network Simulator
A visual, interactive **Network Topology Simulator** built in **C++** using the **Raylib** graphics library. This project allows users to create custom network graphs and visualize famous graph algorithms in real-time with smooth animations.
---
## Features
- 🖱️ **Interactive Graph Builder** — Add nodes, add weighted edges, and delete elements using simple UI controls
- 📡 **Real-time Algorithm Visualization** — Watch algorithms animate step-by-step on your custom graph
- 📋 **Live Log Panel** — See every step of the algorithm explained in a log panel
- 🎨 **Beautiful Dark UI** — Clean, modern dark theme built with Raylib
---
## Algorithms Implemented
| Algorithm | Purpose |
|---|---|
| **Dijkstra's Algorithm** | Shortest path with weighted edges (milliseconds & hops) |
| **BFS (Breadth-First Search)** | Shortest path by number of hops |
| **DFS (Depth-First Search)** | Path exploration using recursion |
| **Prim's Algorithm** | Minimum Spanning Tree (MST) |
| **Kruskal's Algorithm** | Minimum Spanning Tree using Union-Find |
| **Cycle Detection** | Detects if a cycle exists in the graph |
---
## Tech Stack
- **Language:** C++
- **Graphics Library:** [Raylib](https://www.raylib.com/)
- **Data Structures Used:** Vectors, Queues, Stacks, Sets, Priority Queue, Union-Find
---
## Project Structure
NetworkSimulator/
├── main.cpp               # Main source file (all logic & rendering)
├── compile.bat            # Windows build script
├── include/
│   ├── raylib.h           # Raylib header
│   ├── raymath.h          # Math utilities
│   └── rlgl.h             # Low-level graphics
├── lib/
│   ├── libraylib.a        # Raylib static library
│   ├── libraylibdll.a     # Raylib DLL import library
│   └── raylib.dll         # Raylib dynamic library
└── NetworkSimulator.exe   # Compiled executable (Windows)
---
## 🚀 How to Run
### Option 1 — Run Directly
Just double-click NetworkSimulator.exe (Windows only)
### Option 2 — Compile from Source
Make sure you have **MinGW (g++)** installed, then run:
compile.bat
Or manually:
g++ main.cpp -o NetworkSimulator -Iinclude -Llib -lraylib -lopengl32 -lgdi32 -lwinmm
---
## How to Use
1. **Add Node** — Click "Add Node" button, then click anywhere on the canvas
2. **Add Edge** — Click "Add Edge", select source node, then destination node, enter weight
3. **Delete** — Click "Delete" mode and click any node or edge to remove it
4. **Run Algorithm** — Select source & destination, then click any algorithm button
5. **Watch** — The algorithm animates step by step with logs shown on the right panel
---
## 📸 Screenshots

![Screenshot 1](screenshots/WhatsApp%20Image%202026-08-20%20at%206.12.31%20PM.jpeg)
![Screenshot 2](screenshots/WhatsApp%20Image%202026-08-20%20at%206.17.43%20PM.jpeg)
![Screenshot 3](screenshots/WhatsApp%20Image%202026-08-20%20at%206.17.48%20PM.jpeg)
![Screenshot 4](screenshots/WhatsApp%20Image%202026-08-20%20at%206.17.54%20PM.jpeg)
![Screenshot 5](screenshots/WhatsApp%20Image%202026-08-20%20at%206.17.59%20PM.jpeg)
![Screenshot 6](screenshots/WhatsApp%20Image%202026-08-20%20at%206.18.03%20PM.jpeg)
![Screenshot 7](screenshots/WhatsApp%20Image%202026-08-20%20at%206.18.10%20PM.jpeg)
---
## Developed By
- MOAZZAM ALI
---
## 📄 License
This project was developed as an academic project. Feel free to use it for learning purposes.
