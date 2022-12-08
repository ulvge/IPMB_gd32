@echo off
set exePath="%~dp0"
cd %exePath%

HexMerge.exe "..\..\projectGD32F103Boot\output\temp\bmc_boot.hex"  ".\temp\bmc_app.hex" 	  "%~dp0"  "bootApp"