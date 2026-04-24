@echo off
SET KPATH=%~dp0
echo __________________________________________________
echo ^>^>^> %~0
echo %KPATH%

del /Q /S "%KPATH%build_log.txt" 2>nul
rd /Q /S "%KPATH%build" 2>nul
rd /Q /S "%KPATH%output" 2>nul

echo.
choice /T 1 /C ync /CS /D y /n
::TIMEOUT /T 1
