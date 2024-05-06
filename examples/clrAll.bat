@echo off
echo @  Clean All Project's target...
echo.
for /f  "delims="  %%i in ('dir /a-d/b /s "clean.bat"') do (call "%%i")
echo @  Cleaned.
echo.
#pause