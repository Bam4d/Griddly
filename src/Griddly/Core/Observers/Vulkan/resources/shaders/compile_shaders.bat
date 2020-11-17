set SHADER_OUTPUT_DIR=..\..\..\..\..\..\..\resources\shaders

echo %SHADER_OUTPUT_DIR%
echo %SHADER_OUTPUT_DIR%\triangle.frag.spv


if not exist "%SHADER_OUTPUT_DIR%" (mkdir %SHADER_OUTPUT_DIR%)

glslc triangle.frag -o %SHADER_OUTPUT_DIR%\triangle.frag.spv
glslc triangle.vert -o %SHADER_OUTPUT_DIR%\triangle.vert.spv

glslc triangle-textured.frag -o %SHADER_OUTPUT_DIR%\triangle-textured.frag.spv
glslc triangle-textured.vert -o %SHADER_OUTPUT_DIR%\triangle-textured.vert.spv