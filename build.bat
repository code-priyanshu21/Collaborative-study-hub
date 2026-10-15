@echo off
setlocal enabledelayedexpansion

echo ===================================================
echo  StudySync - Build System (PBL Phase Two)
echo ===================================================

where g++ >nul 2>nul
if %ERRORLEVEL% neq 0 (
    echo [ERROR] GCC/G++ compiler not found in PATH.
    echo Please ensure MinGW-w64 is installed and added to PATH.
    exit /b 1
)

echo [INFO] Compiler:
g++ --version | findstr /R /C:"g++" /C:"GCC"

if not exist "src\main.cpp" (
    echo [INFO] Project directory initialized.
    echo [INFO] Ready for source implementation.
    exit /b 0
)

echo [INFO] Compiling StudySync...
if not exist "bin" mkdir bin

g++ -std=c++17 -Wall -Wextra -O2 -Iinclude src\main.cpp -o bin\StudySync.exe -lws2_32
if %ERRORLEVEL% neq 0 (
    echo [ERROR] Build failed!
    exit /b %ERRORLEVEL%
)

echo [SUCCESS] Build completed successfully: bin\StudySync.exe
exit /b 0
