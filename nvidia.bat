@echo off
setlocal enabledelayedexpansion

:: ==============================================================================
:: SCRIPT QUAN LY, KIEM TRA & TOI UU GPU NVIDIA CHO WINDOWS
:: ------------------------------------------------------------------------------
:: Cau hinh may tram muc tieu:
::   MAY TRAM (WTS) ASUS PRO ET700I W7
::   GPU: 2x NVIDIA RTX 4000 Ada Generation - 20GB GDDR6 ECC
::   OS : Windows 10 / 11 / Windows Server (64-bit)
:: ------------------------------------------------------------------------------
:: Chuc nang:
::   1. Kiem tra phan cung GPU qua WMI va NVIDIA System Management Interface (nvidia-smi)
::   2. Theo doi thoi gian thuc nhiet do, VRAM, dien nang tieu thu (Watts)
::   3. Kiem tra trinh bien dich CUDA Toolkit (nvcc) va bien moi truong tren Windows
::   4. Kich hoat che do Persistence Mode & GDDR6 ECC Memory cho 2x RTX 4000 Ada
::   5. Tao tac vu Scheduled Task de tu dong duy tri Persistence Mode khi khoi dong Windows
::   6. Ghi nhat ky vao build\nvidia_windows.log (Tuan thu Rule 8)
:: ==============================================================================

:: Chuyen ve dung thu muc chua batch file
cd /d "%~dp0"

:: Tao thu muc build de luu log
if not exist build mkdir build
set "LOG_FILE=build\nvidia_windows.log"

:: Kiem tra quyen Administrator (Neu chua co, tu dong mo hop thoai UAC RunAs)
net session >nul 2>&1
if %errorlevel% neq 0 (
    echo [INFO] Dang yeu cau quyen Administrator de quan ly driver va GPU...
    powershell -NoProfile -ExecutionPolicy Bypass -Command "Start-Process cmd -ArgumentList '/c \"\"%~dpnx0\" %*\"' -Verb RunAs"
    exit /b
)

:: Tim kiem file thuc thi nvidia-smi tren Windows
where nvidia-smi >nul 2>&1
if %errorlevel% equ 0 (
    set "NVSMI=nvidia-smi"
) else if exist "%ProgramFiles%\NVIDIA Corporation\NVSMI\nvidia-smi.exe" (
    set "NVSMI=%ProgramFiles%\NVIDIA Corporation\NVSMI\nvidia-smi.exe"
) else if exist "%SystemRoot%\System32\nvidia-smi.exe" (
    set "NVSMI=%SystemRoot%\System32\nvidia-smi.exe"
) else (
    set "NVSMI="
)

:: Xu ly tham so dong lenh
if "%~1"=="" goto :menu
if /i "%~1"=="status" goto :status
if /i "%~1"=="monitor" goto :monitor
if /i "%~1"=="opt" goto :optimize
if /i "%~1"=="cuda" goto :cuda_check
if /i "%~1"=="persist" goto :create_task
if /i "%~1"=="unpersist" goto :remove_task
if /i "%~1"=="help" goto :help

:: ------------------------------------------------------------------------------
:: MENU CHINH (INTERACTIVE MENU)
:: ------------------------------------------------------------------------------
:menu
cls
echo ================================================================================
echo        CONG CU KIEM TRA VA QUAN LY GPU NVIDIA CHO WINDOWS
echo       Cau hinh tieu chuan: 2x NVIDIA RTX 4000 Ada Generation 20GB ECC
echo ================================================================================
echo.
echo  1. Xem tong quan trang thai GPU (nvidia-smi ^& Thong tin he thong)
echo  2. Theo doi GPU lien tuc thoi gian thuc (Realtime Monitoring 1s)
echo  3. Kiem tra chi tiet Cam bien: VRAM, Nhiet do, Cong suat dien (Watts)
echo  4. Kiem tra trinh bien dich CUDA Compiler (nvcc) ^& Thu vien AI
echo  5. Kich hoat Persistence Mode ^& ECC Memory cho 2x RTX 4000 Ada
echo  6. Cai dat tu dong chay Persistence Mode khi khoi dong Windows (Task Scheduler)
echo  7. Go bo tac vu tu dong khoi dong cung Windows
echo  8. Xem nhat ky hoat dong (build\nvidia_windows.log)
echo  0. Thoat
echo ================================================================================
set /p "choice=Vui long chon [0-8]: "

if "%choice%"=="1" goto :status
if "%choice%"=="2" goto :monitor
if "%choice%"=="3" goto :sensor_details
if "%choice%"=="4" goto :cuda_check
if "%choice%"=="5" goto :optimize
if "%choice%"=="6" goto :create_task
if "%choice%"=="7" goto :remove_task
if "%choice%"=="8" goto :view_log
if "%choice%"=="0" exit /b 0
goto :menu

:: ------------------------------------------------------------------------------
:: 1. XEM TRANG THAI GPU
:: ------------------------------------------------------------------------------
:status
cls
echo ================================================================================
echo  THONG TIN CARD DO HOA NVIDIA TREN WINDOWS
echo ================================================================================
echo Thoi gian kiem tra: %date% %time%
echo.

echo [*] Danh sach Card do hoa nhan dien qua WMI / Windows Display:
powershell -NoProfile -Command "Get-CimInstance Win32_VideoController | Select-Object Name,DriverVersion,@{Name='AdapterRAM_GB';Expression={[math]::Round($_.AdapterRAM/1GB,2)}},Status | Format-Table -AutoSize"
echo.

if defined NVSMI (
    echo [*] Thong tin chi tiet tu NVIDIA System Management Interface:
    "%NVSMI%"
    echo.
    echo [*] Thong tin tom tat cac GPU phat hien:
    "%NVSMI%" --query-gpu=index,name,driver_version,memory.total,memory.free,memory.used,temperature.gpu,power.draw --format=csv
) else (
    echo [!] CANH BAO: Khong tim thay cong cu nvidia-smi.exe!
    echo Vui long cai dat NVIDIA Official Driver de su dung toan bo tinh nang quan ly.
)

echo.
echo ================================================================================
echo Nhat ky da duoc ghi vao: %LOG_FILE%
echo [%date% %time%] Da kiem tra trang thai GPU tren Windows >> "%LOG_FILE%"
pause
goto :menu

:: ------------------------------------------------------------------------------
:: 2. THEO DOI LIEN TUC THOI GIAN THUC (REALTIME MONITORING)
:: ------------------------------------------------------------------------------
:monitor
if not defined NVSMI (
    echo [!] Khong tim thay nvidia-smi.exe de giam sat!
    pause
    goto :menu
)
cls
echo ================================================================================
echo  DANG THEO DOI GPU THOI GIAN THUC (Cap nhat moi 1 giay)
echo  Nhan phim Ctrl + C de thoat ra Menu
echo ================================================================================
timeout /t 1 >nul
:monitor_loop
cls
echo ================================================================================
echo  THEO DOI GPU THOI GIAN THUC (Cap nhat moi 1s) - Nhan Ctrl + C de thoat
echo ================================================================================
"%NVSMI%"
timeout /t 1 >nul
goto :monitor_loop

:: ------------------------------------------------------------------------------
:: 3. XEM CHI TIET CAM BIEN (SENSORS / VRAM / POWER)
:: ------------------------------------------------------------------------------
:sensor_details
cls
if not defined NVSMI (
    echo [!] Khong tim thay nvidia-smi.exe!
    pause
    goto :menu
)
echo ================================================================================
echo  CHI TIET BO NHO VRAM, NHIET DO VA CONG SUAT DIEN
echo ================================================================================
"%NVSMI%" -q -d MEMORY,TEMPERATURE,POWER,CLOCK
echo.
pause
goto :menu

:: ------------------------------------------------------------------------------
:: 4. KIEM TRA CUDA COMPILER & MOI TRUONG
:: ------------------------------------------------------------------------------
:cuda_check
cls
echo ================================================================================
echo  KIEM TRA BO CONG CU CUDA TOOLKIT TREN WINDOWS
echo ================================================================================
echo.
echo [*] Kiem tra bien moi truong CUDA_PATH:
if defined CUDA_PATH (
    echo     CUDA_PATH = %CUDA_PATH%
) else (
    echo     [!] Bien CUDA_PATH chua duoc thiet lap.
)

echo.
echo [*] Kiem tra trinh bien dich nvcc.exe:
where nvcc >nul 2>&1
if %errorlevel% equ 0 (
    nvcc --version
) else (
    if exist "%CUDA_PATH%\bin\nvcc.exe" (
        "%CUDA_PATH%\bin\nvcc.exe" --version
    ) else (
        echo [!] Khong tim thay trinh bien dich nvcc tren may.
        echo Vui long cai dat NVIDIA CUDA Toolkit cho Windows:
        echo https://developer.nvidia.com/cuda-downloads
    )
)

echo.
pause
goto :menu

:: ------------------------------------------------------------------------------
:: 5. TOI UU PERSISTENCE MODE & ECC CHO 2x RTX 4000 ADA
:: ------------------------------------------------------------------------------
:optimize
cls
echo ================================================================================
echo  TOI UU HOA MULTI-GPU & ECC MEMORY CHO 2x RTX 4000 ADA
echo ================================================================================
if not defined NVSMI (
    echo [!] Khong tim thay nvidia-smi.exe!
    pause
    goto :menu
)

echo [*] Dang bat Persistence Mode tren toan bo GPU...
"%NVSMI%" -pm 1
if %errorlevel% equ 0 (
    echo [OK] Da bat thanh cong Persistence Mode.
) else (
    echo [!] Khong the bat Persistence Mode qua nvidia-smi.
)

echo.
echo [*] Kiem tra va kich hoat GDDR6 ECC Memory (Dac quyen RTX 4000 Ada)...
"%NVSMI%" -e 1 >nul 2>&1
if %errorlevel% equ 0 (
    echo [OK] Che do ECC Memory da duoc bat tren cac GPU ho tro.
) else (
    echo [i] Trang thai ECC: Khong ho tro hoac da o che do mac dinh.
)

echo.
echo [*] Kiem tra ma tran ket noi giua 2 GPU (PCIe / P2P Topology):
"%NVSMI%" topo -m

echo.
echo [%date% %time%] Da ap dung toi uu Persistence Mode va ECC >> "%LOG_FILE%"
pause
goto :menu

:: ------------------------------------------------------------------------------
:: 6. DANG KY TASK SCHEDULER DE TU DONG BAT KHI KHOI DONG WINDOWS
:: ------------------------------------------------------------------------------
:create_task
cls
echo ================================================================================
echo  TAO TAC VU TU DONG KHOI DONG (TASK SCHEDULER)
echo ================================================================================
if not defined NVSMI (
    echo [!] Khong tim thay nvidia-smi.exe de tao tac vu!
    pause
    goto :menu
)

echo [*] Dang tao Scheduled Task 'NVIDIA_GPU_Persistence_Startup'...
schtasks /create /tn "NVIDIA_GPU_Persistence_Startup" /tr "\"%NVSMI%\" -pm 1" /sc onstart /ru "SYSTEM" /f >nul 2>&1
if %errorlevel% equ 0 (
    echo [OK] Da tao thanh cong tac vu 'NVIDIA_GPU_Persistence_Startup'!
    echo Moi khi khoi dong Windows, he thong se tu dong bat Persistence Mode cho 2x GPU.
    echo [%date% %time%] Da tao Scheduled Task NVIDIA_GPU_Persistence_Startup >> "%LOG_FILE%"
) else (
    echo [!] Loi khi tao Scheduled Task. Vui long kiem tra quyen Administrator.
)

echo.
pause
goto :menu

:: ------------------------------------------------------------------------------
:: 7. GO BO TAC VU KHOI DONG
:: ------------------------------------------------------------------------------
:remove_task
cls
echo [*] Dang go bo Scheduled Task 'NVIDIA_GPU_Persistence_Startup'...
schtasks /delete /tn "NVIDIA_GPU_Persistence_Startup" /f >nul 2>&1
if %errorlevel% equ 0 (
    echo [OK] Da go bo thanh cong Scheduled Task!
    echo [%date% %time%] Da go bo Scheduled Task NVIDIA_GPU_Persistence_Startup >> "%LOG_FILE%"
) else (
    echo [i] Tac vu 'NVIDIA_GPU_Persistence_Startup' chua ton tai hoac da bi xoa.
)
echo.
pause
goto :menu

:: ------------------------------------------------------------------------------
:: 8. XEM NHAT KY HOAT DONG
:: ------------------------------------------------------------------------------
:view_log
cls
echo ================================================================================
echo  NHAT KY HOAT DONG (%LOG_FILE%)
echo ================================================================================
if exist "%LOG_FILE%" (
    type "%LOG_FILE%"
) else (
    echo Nhat ky trong.
)
echo.
pause
goto :menu

:: ------------------------------------------------------------------------------
:: TRO GIUP (HELP)
:: ------------------------------------------------------------------------------
:help
echo Cach su dung %~nx0:
echo   %~nx0             : Mo Menu tuong tac quan ly GPU
echo   %~nx0 status      : Xem thong tin va trang thai GPU
echo   %~nx0 monitor     : Giam sat GPU thoi gian thuc
echo   %~nx0 opt         : Bat Persistence Mode va ECC Memory
echo   %~nx0 cuda        : Kiem tra CUDA Compiler va bien moi truong
echo   %~nx0 persist     : Cai dat tu dong bat Persistence khi khoi dong
echo   %~nx0 unpersist   : Go bo tac vu khoi dong tu dong
exit /b 0

