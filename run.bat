@echo off
setlocal

echo ===================================================
echo  StudySync - Application Launcher
echo ===================================================

if not exist "bin\StudySync.exe" (
    echo [INFO] Executable not found. Running build.bat first...
    call build.bat
    if not exist "bin\StudySync.exe" (
        echo [INFO] Ready for application source files.
        exit /b 0
    )
)

echo [INFO] Launching StudySync...
start "" "http://localhost:8080"
bin\StudySync.exe
