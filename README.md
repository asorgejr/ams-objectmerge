# AMS Object Merge Plugin
This is a DSO plugin for Houdini that merges multiple objects into one.
It is similar to the built-in object merge node, but it has a few extra features:
- It can assign transform path attributes to the merged objects. This is useful for exporting to USD, and packing in Alembic.
- It is context-aware of material paths at both the geometry and object level.

## Installation
Run CMake to generate the project files for your platform, then build the project.
Copy the resulting DSO file to your Houdini plugins directory.
