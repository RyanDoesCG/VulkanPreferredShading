cls

@REM shaders for rendering geometry information
@REM to an offscreen texture for later reference
C:\VulkanSDK\1.0.61.1\Bin32\glslangValidator -V geometry.vert -o geometry.vert.spv
C:\VulkanSDK\1.0.61.1\Bin32\glslangValidator -V geometry.frag -o geometry.frag.spv

@REM shaders for computing lighting from the attachments
C:\VulkanSDK\1.0.61.1\Bin32\glslangValidator -V lighting.vert -o lighting.vert.spv
C:\VulkanSDK\1.0.61.1\Bin32\glslangValidator -V lighting.frag -o lighting.frag.spv

@REM shaders for rasterizing the geometry and lighting
@REM to the screen in the final pass
C:\VulkanSDK\1.0.61.1\Bin32\glslangValidator -V raster.vert -o raster.vert.spv
C:\VulkanSDK\1.0.61.1\Bin32\glslangValidator -V raster.frag -o raster.frag.spv

pause