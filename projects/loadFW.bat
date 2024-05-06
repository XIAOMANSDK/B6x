@echo off & setlocal EnableDelayedExpansion

SET JLink_exe="C:\Program Files (x86)\SEGGER\JLink\JLink.exe"

if not exist %JLink_exe% (
    echo error: %JLink_exe% not exist, please modify path!
    goto ERROR
)

::SET KPATH=%~dp0
::SET FW_Dir=%KPATH%..\ble\lib\
SET Fw_Hex=..\ble\lib\ble6_fw.hex
SET FW_Cmd=fw_cmd.txt

echo ************Load B6x File************
echo.
echo Please input firmware file(*.hex), default "%Fw_Hex%"
SET /p Fw_File=^>^>^> 

if "%Fw_File%"=="" (
    SET Fw_File=%Fw_Hex%
)
echo.

if not exist "!Fw_File!" (
    echo error: "!Fw_File!" not exist, please choose file!
    goto ERROR
)

REM Add -commandfile to temp file
(
echo device B6x
echo si 1
echo speed 4000
echo r 
echo h
echo exec SetCompareMode=0
echo exec SetVerifyDownload=0
echo exec EnableEraseAllFlashBanks
echo loadfile "!Fw_File!"
echo qc
) > %FW_Cmd%

ECHO JLink.exe               [%JLink_exe%]
ECHO Load Program file       [%FW_Dir%%Fw_Hex%]

::%JLink_exe% -log jlink.log -autoconnect 1 -device B6x -if SWD -speed 4000 -commandfile "%FW_Cmd%" 
%JLink_exe% "%FW_Cmd%"
DEL /q %FW_Cmd%

IF ERRORLEVEL 1 (
    ECHO J-Flash Program : Error!
    goto ERROR
)

ECHO J-Flash Program : OK!
goto END

:ERROR
pause

:END
