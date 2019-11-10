@echo OFF
setlocal EnableDelayedExpansion

set arg1=%1
set WIDTH=1024
set HEIGHT=680

if "%~1" == "" goto TESTFOLDER


: TESTFOLDER
FOR %%i IN (%arg1%) DO IF EXIST %%~si\NUL GOTO FOLDER


:FILE
setlocal DISABLEDELAYEDEXPANSION
set WFCFILE=%1
wfc2png.exe %WFCFILE% %WIDTH% %HEIGHT%
endlocal
GOTO QUIT


:FOLDER
setlocal disabledelayedexpansion
 for /f "delims=: tokens=1*" %%A in ('dir /b /s /a-d %arg1%^|findstr /n "^"') do (
	wfc2png.exe "%%B" %WIDTH% %HEIGHT%
 )
setlocal EnableDelayedExpansion


GOTO QUIT

:QUIT
pause
