@echo off
set CURR_PATH=%~dp0
cd /d %CURR_PATH%
if "%~1"=="" (
    echo Usage: %~nx0 "$L"
    goto :eof
)

REM 移除参数中的引号（如果存在）
set "ARG=%~1"
set "ARG=%ARG:"=%"

powershell.exe -NoProfile -ExecutionPolicy Unrestricted -Command "& { & '.\BxOTA.PS1' '%ARG%' }"
::pause