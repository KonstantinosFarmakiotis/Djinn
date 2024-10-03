# Djinn

This is the work I have made as part of my computer graphics project for the corresponding semester course. It is a simulation of a djinn that comes out of a lamp.

The basic steps that needed to be implemented were the creation of a simple scene through the loading and creation of objects, the lighting of the objects as well as their shading, the production of particles and the management of their movement, and the handling of a mesh object.

The "djinn" file contains the .exe and a few keyboard commands to control the simulation are listed below.

Keyboard Commands Manual
- WASD            control the camera movement on the x and y axis
- QE              control the camera movement on the z axis
- ARROW UP/DOWN   control the zoom in and out effect of the camera
- T (toggle)      enables/disables polygon view of the scene
- Y (toggle)      enables/disables the particles of the scene      
- IJKL            control the lighting source movement on the x and y axis
- UO              control the lighting source movement on the z axis
- -+              control the djinn movement on the z axis
- C               coin particles start to fall from the ceiling
- P (toggle)      pauses/starts the simulation
- ESC             ends the simulation

Requirements:
- visual studio 17(and upwards) installation with a C++17 compiler

To build:
- Update your drivers!
- Install CMake
- Download the source code
- Open Cmake and input the path to the 
'Djinn' directory on the
source code box. It is the same directory
as the readme.md file that this text is 
generated from.
- Create a 'build' folder on the same level
and input its path to the 'build the binaries'
textbox
Your inputs should look something like:
first textbox: C:/Users/[username]/Djinn
second textbox: C:/Users/[username]/Djinn/build
- Press Configure and select your installed version of Visual Studio
as the generator for this project
- Leave the rest of the options empty and select 'Use default
native compilers'
- Press Generate 
- Press Open Project
- Run from Visual studio
- Have fun!