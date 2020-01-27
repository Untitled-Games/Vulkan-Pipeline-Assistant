FOR %%i IN (.\*.frag) DO %VK_SDK_PATH%\Bin\glslc.exe %%i -c -o ../%%~ni.spv
FOR %%i IN (.\*.vert) DO %VK_SDK_PATH%\Bin\glslc.exe %%i -c -o ../%%~ni.spv
pause