FOR %%i IN (.\*.frag) DO %VK_SDK_PATH%\Bin\glslc.exe %%i -c -o ../Build/shaders/%%~ni.spv
FOR %%i IN (.\*.vert) DO %VK_SDK_PATH%\Bin\glslc.exe %%i -c -o ../Build/shaders/%%~ni.spv
pause