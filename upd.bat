@echo off
if "%1"=="" goto help

set dest=%ProgramFiles(x86)%\premise\sys
if not exist "%dest%" goto help

echo Killing sys process
taskkill /f /im sys.exe
taskkill /f /im psmonitor.exe
taskkill /f /im prkernel.exe

if "%2"== "reset" (
echo Deleting data.
del "%dest%\schema\slserver.*" /q /s /f > nul
del "%dest%\data\*.*" /q /s /f > nul
for /d %%1 in ("%dest%\data\*.*") do rmdir /s/q "%%1"
)

echo Copying new binaries
for %%F in (spBond spRadioRA2) do (
	copy /y "%1\%%F.dll" "%dest%\bin\devices"
	copy /y "%1\%%F.pdb" "%dest%\bin\devices"
	)

echo Starting service and launching client.
start /wait net start prkernel
start /D"%ProgramFiles(x86)%\premise\sys\bin" sys.exe

goto Exit

:Help
echo "Usage:  upd Release|Debug [reset]"
echo "        reset will cause all sys data to be deleted (slserver.xdo, data\*)
echo Example: upd Debug reset
:Exit
set dest=
