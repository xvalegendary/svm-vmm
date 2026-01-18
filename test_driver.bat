@echo off
echo ========================================
echo SVM Hypervisor Debug Test
echo ========================================
echo.
echo This will load the driver with kdmapper and show debug output.
echo Make sure DebugView is running to see kernel debug messages!
echo.
echo Press any key to continue...
pause >nul

echo.
echo [*] Loading driver with kdmapper...
cd /d "%~dp0bin\x64\Release"
kdmapper.exe SeCodeIntegrityQueryInformation.sys

echo.
echo ========================================
echo Check DebugView for detailed output!
echo ========================================
echo.
echo Look for messages starting with "SVM-HV:"
echo These will show exactly where initialization fails.
echo.
pause