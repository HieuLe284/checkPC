@echo off
setlocal enabledelayedexpansion

:: Chuyen ve dung thu muc hien tai chua batch file
cd /d "%~dp0"

:: Tao thu muc build theo Rule 7 (Tat ca file tao ra deu nam trong build)
if not exist build mkdir build

echo ================================================================================
echo  [BUILD] Dang bien dich chuong trinh C++ tren Windows vao thu muc build...
echo ================================================================================

:: Kiem tra compiler san co: MinGW g++ hoac MSVC cl.exe
where g++ >nul 2>&1
if %errorlevel% equ 0 (
    echo [*] Phat hien trinh bien dich MinGW g++...
    g++ -std=c++17 -O2 main.cpp -o build\sysinfo.exe
    if %errorlevel% neq 0 (
        echo [!] ERROR: Bien dich that bai bang g++!
        pause
        exit /b %errorlevel%
    )
) else (
    where cl >nul 2>&1
    if %errorlevel% equ 0 (
        echo [*] Phat hien trinh bien dich MSVC cl.exe...
        cl /EHsc /std:c++17 /O2 main.cpp /Fe:build\sysinfo.exe /Fo:build\
        if %errorlevel% neq 0 (
            echo [!] ERROR: Bien dich that bai bang cl.exe!
            pause
            exit /b %errorlevel%
        )
    ) else (
        echo [!] ERROR: Khong tim thay trinh bien dich g++ hoac cl.exe tren may!
        echo Vui long mo Developer Command Prompt cho Visual Studio hoac cai MinGW.
        pause
        exit /b 1
    )
)

echo [OK] Bien dich hoan tat -> build\sysinfo.exe
echo ================================================================================
echo  [RUN] Bat dau chay chuong trinh kiem tra he thong...
echo ================================================================================

build\sysinfo.exe

echo.
echo ================================================================================
echo Ket qua da duoc luu vao cac tap tin trong thu muc build:
echo   - build\hardware_report.txt (Bao cao toan dien thong so phan cung he thong)
echo   - build\workstation.txt     (Bao cao doi chieu thong so May tram ASUS WTS)
echo ================================================================================
pause

