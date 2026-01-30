# 3D Terminal Rendering Tool With support for viewing ply files

A Windows-only 3D renderer that runs entirely in the terminal, designed for high performance and low-level control.
The renderer supports real-time camera movement, PLY model loading, culling optimizations, and a GLFW-style input polling system — all while maintaining 100+ FPS on complex models.

This project focuses on performance, simplicity, and demonstrating how far a terminal-based renderer can be pushed using SIMD-accelerated math.


## Features
- Keyboard & mouse camera controls
- GLFW-style input polling system
- PLY file support
	- Triangle primitives only
	- Vertex colors defined as uchar RGB
- Frustum culling
- Back-face culling
	- can be enabled/disabled via CMake options
	- meshes should be of Counter Clockwise winding
- Terminal resize support
- Highly optimized
	- SIMD used extensively for matrix and vector math
	- Extensive profiling done to cut hot paths as much as possible
- Capable of 100+ FPS with models exceeding:
	- 100,000 vertices
	- 80,000+ faces
- Anti Aliasing Support
	- mode 1: 2x2 through quater block characters
	- mode 2: 2x4 through braille characters

## Demos
1. Orbiting Cube Demo
	A simple demo showcasing camera motion and input handling and polling.

	Controls:

	A – increase camera rotation to the left
	
	D – increase camera rotation to the right

	Q - Enter wireframe mode

	Ctrl+C - quit

2. Ply viewer Demo: Loads and renders ply files to the terminal. \
	Demo highlights the renderer's ability to load and render ply files as well as show casing the camera and input system
	
	Controls:
	WASD - move the camera in cardinal directions realtive to the camera forward direction

    Q - enter wireframe mode

    Mouse - Click and drag in the direction you would to move the camera in

    T - Toggle what ply is being rendered to the screen

	Ctrl+C - quit


## Videos

### Cube Demo
[![video](https://i.ytimg.com/vi/CrHs00-3Ots/maxresdefault.jpg?sqp=-oaymwEmCIAKENAF8quKqQMa8AEB-AH-CYACuAWKAgwIABABGE8gTShlMA8=&amp;rs=AOn4CLDUjur1TF3HrGf_M9v8u8zev0_7vA)](https://www.youtube.com/watch?v=CrHs00-3Ots)

### Ply Viewer
[![video](https://i.ytimg.com/vi/4LW14TQwvUc/maxresdefault.jpg?sqp=-oaymwEmCIAKENAF8quKqQMa8AEB-AH-CYACuAWKAgwIABABGGUgUChDMA8=&amp;rs=AOn4CLDYfET3UnjRekcrSzce83-aSWW3Kw)](https://www.youtube.com/watch?v=4LW14TQwvUc)

## How To Build
run the following in the source directory

Some options that can be passed to cmake as a -D flags
* DEMOS
	* Builds demo programs (floating cube and ply viewer)	
* BACK_FACE
	* Turns back face culling on
```sh
mkdir build
cmake -S . -B ./build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```
you can also pull this project into CLion or Visual Studios and build the targets that way.

## HELPFUL LINKS (Used during the creation of this renderer)
[getting direction](https://stackoverflow.com/questions/15697273/how-can-i-get-view-direction-from-the-opengl-modelview-matrix) \
[scratch Pixel](https://www.scratchapixel.com/index.html) \
[lighthouse 3d: Frustum culling](https://www.lighthouse3d.com/tutorials/view-frustum-culling/) \
[indigo quilezles: frustum culling](https://iquilezles.org/articles/frustumcorrect/) \
[Extracting view frustum: transpose method](https://fgiesen.wordpress.com/2012/08/31/frustum-planes-from-the-projection-matrix/) \
[Extracting view Frustum paper](http://www8.cs.umu.se/kurser/5DV051/HT12/lab/plane_extraction.pdf) \
[Ply Parser](https://w3.impa.br/~diego/software/rply/)
