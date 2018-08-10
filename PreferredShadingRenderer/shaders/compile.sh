#!/bin/sh

# shaders for rendering geometry information
# to an offscreen texture for later reference
glslangValidator -V geometry.vert -o geometry.vert.spv;
glslangValidator -V geometry.frag -o geometry.frag.spv;

# shaders for computing lighting from the attachments
glslangValidator -V lighting.vert -o lighting.vert.spv;
glslangValidator -V lighting.frag -o lighting.frag.spv;

# shaders for rasterizing the geometry and lighting
# to the screen in the final pass
glslangValidator -V raster.vert -o raster.vert.spv;
glslangValidator -V raster.frag -o raster.frag.spv;