@echo off
set exePath="%~dp0"
cd %exePath%

HexMerge.exe ".\temp\bmc_boot.hex"  "..\..\projectGD32F103\output\temp\bmc_app.hex" 	  "%~dp0"  "bootApp"