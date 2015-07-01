An interactive volume raycaster I wrote in the summer of 2010 as part of a medical research effort.
Designed to visualize **streaming** ultrasound data - each frame is read anew from an ultrasound probe (see caveats).
This program also supports compositing multiple datasets into a single visualization.

Build Requirements
==================

The codebase written is in C++.
It was last successfully built on Windows using the 32-bit MinGW gcc compiler.
The build process is managed by `CMake v2.8`.

Library Dependencies
--------------------

(lib)png, glew32, glut32, glu32, OpenGL32

Hardware Dependencies
---------------------

In 2010, it was possible to attain 15 FPS on an Nvidia Quadro Plex.
With modern hardware the program is likely usable on consumer graphics cards.
With better integration with modern OpenGL, performance could also be improved.

Caveats
=======

This program only reads data from a series of PNG files.
No support for actual ultrasound hardware was implemented.

Known Issues
============

* Slices having incorrect x scale. Seems that the scale is dependent on the buffer/texture size, which shouldn't be the case.
