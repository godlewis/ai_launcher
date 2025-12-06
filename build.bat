@echo off
echo Setting up Visual Studio environment...
call "D:\Programs\Microsoft Visual Studio\2022\BuildTools\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64
echo.
echo Starting compilation of ai_launcher...
cl /utf-8 /EHsc /Fe:ai_launcher.exe ai_launcher.cpp user32.lib gdi32.lib advapi32.lib shell32.lib ole32.lib
if %ERRORLEVEL% EQU 0 (
    echo ai_launcher compiled successfully!
) else (
    echo ai_launcher compilation failed!
)
echo.
echo Starting compilation of registry_manager...
cl /utf-8 /EHsc /Fe:registry_manager.exe registry_manager.cpp user32.lib gdi32.lib comctl32.lib shlwapi.lib advapi32.lib shell32.lib comdlg32.lib
if %ERRORLEVEL% EQU 0 (
    echo registry_manager compiled successfully!
) else (
    echo registry_manager compilation failed!
)
echo.
echo Compilation complete
pause