set SHADER_OUTPUT_DIR=..\..\..\..\..\..\..\resources\shaders

if not exist "%SHADER_OUTPUT_DIR%" (mkdir %SHADER_OUTPUT_DIR%)

glslc triangle-textured.frag -o %SHADER_OUTPUT_DIR%\triangle-textured.frag.spv
glslc triangle-textured.vert -o %SHADER_OUTPUT_DIR%\triangle-textured.vert.spv