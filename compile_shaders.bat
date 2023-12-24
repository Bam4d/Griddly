cd /D "%~dp0"

CALL :compile_shaders_in_dir .\resources\shaders\default\block
CALL :compile_shaders_in_dir .\resources\shaders\default\sprite
CALL :compile_shaders_in_dir .\resources\shaders\default\isometric
CALL :compile_shaders_in_dir .\tests\resources\observer\block\shaders\global_lighting
CALL :compile_shaders_in_dir .\tests\resources\observer\isometric\shaders\lighting
CALL :compile_shaders_in_dir .\tests\resources\observer\sprite\shaders\health_bars

CALL :compile_shaders_in_dir ".\python\examples\Custom Shaders\Global Lighting\shaders"
CALL :compile_shaders_in_dir ".\python\examples\Custom Shaders\Health Bars\shaders"
CALL :compile_shaders_in_dir ".\python\examples\Custom Shaders\Object Lighting\shaders"



EXIT /B 0

:compile_shaders_in_dir
echo "Compiling shaders in %~1"
%GLSLC_BIN% "%~1\triangle-textured.frag" -o "%~1\triangle-textured.frag.spv"
%GLSLC_BIN% "%~1\triangle-textured.vert" -o "%~1\triangle-textured.vert.spv"
