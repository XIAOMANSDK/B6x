@Echo off & setlocal EnableDelayedExpansion

SET AppFile=.\output\loader.bin
SET OutFile=.\output\loader_info.bin

SET MAG_INFO=0xAA55A001
SET FLASH_ADDR=0x18000010
SET SRAM_ADDR=0x20003600
SET MAG_OFFSET=0x10
SET InfoFile=.\info.bin
SET zeroFile=.\zero.bin
SET SREC_CAT=..\..\..\tools\srec_cat.exe

if not exist %InfoFile% (
	fsutil file createNew %InfoFile% 16
)

for %%i in (%AppFile%) do (
	SET app_bin_size=%%~zi
)

set /a bin_sz=%app_bin_size%+%MAG_OFFSET% 
::不足256字节的补全00
set /a last_sz=256-%bin_sz%

if not exist %zeroFile% (
	fsutil file createNew %zeroFile% %last_sz%
)

%SREC_CAT% -generate 0x00 0x04 -constant-l-e %MAG_INFO%      4 %InfoFile% -binary -exclude 0x00 0x04 -o %InfoFile% -binary 
%SREC_CAT% -generate 0x04 0x08 -constant-l-e %app_bin_size%  4 %InfoFile% -binary -exclude 0x04 0x08 -o %InfoFile% -binary 
%SREC_CAT% -generate 0x08 0x0C -constant-l-e %FLASH_ADDR%    4 %InfoFile% -binary -exclude 0x08 0x0C -o %InfoFile% -binary 
%SREC_CAT% -generate 0x0C 0x10 -constant-l-e %SRAM_ADDR%     4 %InfoFile% -binary -exclude 0x0C 0x10 -o %InfoFile% -binary 

%SREC_CAT% %InfoFile% -binary %AppFile% -binary -offset %MAG_OFFSET% -o %OutFile% -binary
%SREC_CAT% %OutFile% -binary %zeroFile% -binary -offset %bin_sz% -o %OutFile% -binary
del %InfoFile%
del %zeroFile%

echo %app_bin_size%, %bin_sz%, %last_sz%
::pause
