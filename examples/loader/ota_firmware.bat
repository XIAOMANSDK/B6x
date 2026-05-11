@Echo off & setlocal EnableDelayedExpansion

SET AppFile=bleUart.bin
SET OutFile=OTA.bin

SET OTA_ADDR=0x18020100
SET LDR_TYPE_COPY=0x55AA5AA5

SET InfoFile=.\info.bin
SET SREC_CAT=..\..\tools\srec_cat.exe

if not exist %InfoFile% (
	fsutil file createNew %InfoFile% 256
)

for %%i in (%AppFile%) do (
	SET app_bin_size=%%~zi
)
::echo %app_bin_size%

%SREC_CAT% -generate 0x00 0x04 -constant-l-e %LDR_TYPE_COPY% 4 %InfoFile% -binary -exclude 0x00 0x04 -o %InfoFile% -binary 
%SREC_CAT% -generate 0x04 0x08 -constant-l-e %app_bin_size%  4 %InfoFile% -binary -exclude 0x04 0x08 -o %InfoFile% -binary 
%SREC_CAT% -generate 0x08 0x0C -constant-l-e %OTA_ADDR%      4 %InfoFile% -binary -exclude 0x08 0x0C -o %InfoFile% -binary 

%SREC_CAT% %InfoFile% -binary %AppFile% -binary -offset 0x100 -o %OutFile% -binary

::pause
