@echo off
set CURR_PATH=%~dp0
cd /d %CURR_PATH%

powershell.exe -executionpolicy Unrestricted -file "..\..\tools\cmake_build.PS1" -Action rebuild -BuildDir "%CURR_PATH%build"
::pause