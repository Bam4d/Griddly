CALL :compile_shaders_in_dir .\resources\shaders
CALL :compile_shaders_in_dir .\tests\resources\observer\block\shaders\global_lighting
CALL :compile_shaders_in_dir .\tests\resources\observer\isometric\shaders\lighting
CALL :compile_shaders_in_dir .\tests\resources\observer\sprite\shaders\health_bars

:compile_shaders_in_dir
  glslc %~1\triangle-textured.frag -o %~1\triangle-textured.frag.spv
  glslc %~1\triangle-textured.vert -o %~1\triangle-textured.vert.spv
EXIT /B 0
