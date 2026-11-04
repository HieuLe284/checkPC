@echo off
setlocal enabledelayedexpansion

:: ==============================================================================
:: SCRIPT THIET LAP & QUAN LY NGUON CPU INTEL (POWER MANAGEMENT) CHO WINDOWS
:: ------------------------------------------------------------------------------
:: Cau hinh may tram muc tieu:
::   MAY TRAM (WTS) ASUS PRO ET700I W7
::   CPU: 1x Intel Xeon W5-3435X 16C/32T 45M (hoac Intel Core / Ultra)
::   OS : Windows 10 / 11 / Windows Server (64-bit)
:: ------------------------------------------------------------------------------
:: Chuc nang:
::   1. Xem thong tin CPU, xung nhip (MHz), muc tai (%) va Power Scheme hien tai
::   2. Thiet lap cac che do nguon CPU (Eco, Balanced, High, Ultimate Performance)
::   3. Bat / Tat che do Intel Turbo Boost qua Windows PowerCFG
::   4. Khong che gioi han xung nhip toi da (Max Frequency Cap MHz) de giam nhiet va dien
::   5. Tich hop ho tro Intel Extreme Tuning Utility (XTU CLI) de set PL1/PL2 neu co
::   6. Ghi nhat ky vao build\cpu_windows.log (Tuan thu Rule 8)
:: ==============================================================================

:: Chuyen ve dung thu muc chua batch file
cd /d "%~dp0"

:: Tao thu muc build de luu log
if not exist build mkdir build
set "LOG_FILE=build\cpu_windows.log"

:: Kiem tra quyen Administrator
net session >nul 2>&1
if %errorlevel% neq 0 (
    echo [INFO] Dang yeu cau quyen Administrator de quan ly Power Plan CPU...
    powershell -NoProfile -ExecutionPolicy Bypass -Command "Start-Process cmd -ArgumentList '/c \"\"%~dpnx0\" %*\"' -Verb RunAs"
    exit /b
)

:: Dinh danh cac GUID Power Plan chuan cua Windows
set "GUID_BALANCED=381b4222-f694-41f0-9685-ff5bb260df2e"
set "GUID_HIGH_PERF=8c5e7fda-e8bf-4a96-9a85-a6e23a8c635c"
set "GUID_SAVER=a1841308-3541-4fab-bc81-f71556f20b4a"
set "GUID_ULTIMATE=e9a42b02-d5df-448d-aa00-03f14749eb61"

:: Tim kiem file thuc thi Intel XTU CLI neu may da cai san
set "XTU_CLI="
if exist "%ProgramFiles%\Intel\Intel(R) Extreme Tuning Utility\Client\XtuCLI.exe" (
    set "XTU_CLI=%ProgramFiles%\Intel\Intel(R) Extreme Tuning Utility\Client\XtuCLI.exe"
)

:: Xu ly tham so dong lenh
if "%~1"=="" goto :menu
if /i "%~1"=="status" goto :status
if /i "%~1"=="monitor" goto :monitor
if /i "%~1"=="eco" goto :set_eco
if /i "%~1"=="balanced" goto :set_balanced
if /i "%~1"=="high" goto :set_high
if /i "%~1"=="ultimate" goto :set_ultimate
if /i "%~1"=="turbo_on" goto :turbo_on
if /i "%~1"=="turbo_off" goto :turbo_off
if /i "%~1"=="freq" goto :set_freq_cli
if /i "%~1"=="help" goto :help

:: ------------------------------------------------------------------------------
:: MENU CHINH (INTERACTIVE MENU)
:: ------------------------------------------------------------------------------
:menu
cls
echo ================================================================================
echo         CONG CU QUAN LY NGUON VA HIEU NANG CPU INTEL CHO WINDOWS
echo       Tuong thich: Intel Xeon W5-3435X / Intel Core Ultra / Core i7-i9
echo ================================================================================
echo.
echo  1. Xem trang thai CPU, Power Plan va Xung nhip hien tai
echo  2. Che do Tiet kiem dien / Mat may (Eco / Power Saver - Mat me, giam tai)
echo  3. Che do Can bang tieu chuan      (Balanced Mode - Mac dinh on dinh)
echo  4. Che do Hieu nang cao            (High Performance - 100%% Xung nhip)
echo  5. Che do May tram toi thuong      (Ultimate Performance - Chuyen dung Workstation)
echo  6. Bat Intel Turbo Boost (Aggressive / Max Turbo)
echo  7. Tat Intel Turbo Boost (Giu xung nhip Base, khong vuot nguong, ha nhiet do)
echo  8. Khong che xung nhip toi da theo MHz (Processor Frequency Cap)
echo  9. Thiet lap PL1/PL2 qua Intel XTU CLI (Neu co cai dat Intel XTU)
echo 10. Giam sat xung nhip & Tai CPU thoi gian thuc (Realtime Monitoring)
echo 11. Xem nhat ky thiet lap (build\cpu_windows.log)
echo  0. Thoat
echo ================================================================================
set /p "choice=Vui long chon [0-11]: "

if "%choice%"=="1" goto :status
if "%choice%"=="2" goto :set_eco
if "%choice%"=="3" goto :set_balanced
if "%choice%"=="4" goto :set_high
if "%choice%"=="5" goto :set_ultimate
if "%choice%"=="6" goto :turbo_on
if "%choice%"=="7" goto :turbo_off
if "%choice%"=="8" goto :set_freq_input
if "%choice%"=="9" goto :xtu_rapl
if "%choice%"=="10" goto :monitor
if "%choice%"=="11" goto :view_log
if "%choice%"=="0" exit /b 0
goto :menu

:: ------------------------------------------------------------------------------
:: 1. XEM TRANG THAI CPU & POWER SCHEME
:: ------------------------------------------------------------------------------
:status
cls
echo ================================================================================
echo  THONG SO CPU VA POWER SCHEME HIEN TAI TREN WINDOWS
echo ================================================================================
echo Thoi gian kiem tra: %date% %time%
echo.

echo [*] Thong tin Bo vi xu ly (Win32_Processor):
powershell -NoProfile -Command "Get-CimInstance Win32_Processor | Select-Object Name,NumberOfCores,NumberOfLogicalProcessors,CurrentClockSpeed,MaxClockSpeed,LoadPercentage | Format-List"

echo [*] Che do Power Plan hien tai dang kich hoat:
powercfg /getactivescheme
echo.

echo [*] Trang thai cau hinh quan ly nguon CPU (Sub-Processor):
powershell -NoProfile -Command "$s = (powercfg /getactivescheme).Split()[3]; Write-Host '  - Power Plan GUID:' $s; $max = (powercfg /query $s SUB_PROCESSOR PROCTHROTTLEMAX | Select-String 'Current AC Power Setting Index:'); Write-Host '  - Max Processor State:' $max; $min = (powercfg /query $s SUB_PROCESSOR PROCTHROTTLEMIN | Select-String 'Current AC Power Setting Index:'); Write-Host '  - Min Processor State:' $min"

if defined XTU_CLI (
    echo.
    echo [*] Phat hien Intel XTU CLI tai: %XTU_CLI%
    "%XTU_CLI%" -t -l | findstr /i "power boost tdp"
)

echo.
echo ================================================================================
echo [%date% %time%] Da kiem tra trang thai CPU tren Windows >> "%LOG_FILE%"
pause
goto :menu

:: ------------------------------------------------------------------------------
:: 2. CHE DO TIET KIEM DIEN (ECO / POWER SAVER)
:: ------------------------------------------------------------------------------
:set_eco
cls
echo [*] Dang kich hoat Che do Tiet kiem dien (Power Saver)...
powercfg /setactive %GUID_SAVER% 2>nul
if %errorlevel% neq 0 (
    powercfg /duplicatescheme %GUID_SAVER% >nul 2>&1
    powercfg /setactive %GUID_SAVER%
)
:: Dat max xung 80%, tat turbo de mat may
powercfg /setacvalueindex SCHEME_CURRENT SUB_PROCESSOR PROCTHROTTLEMAX 80
powercfg /setacvalueindex SCHEME_CURRENT SUB_PROCESSOR PERFBOOSTMODE 0
powercfg /setactive SCHEME_CURRENT
echo [OK] Da kich hoat thanh cong Che do Eco (Power Saver, Max 80%%, Tat Turbo).
echo [%date% %time%] Da kich hoat Che do Eco >> "%LOG_FILE%"
pause
goto :menu

:: ------------------------------------------------------------------------------
:: 3. CHE DO CAN BANG (BALANCED)
:: ------------------------------------------------------------------------------
:set_balanced
cls
echo [*] Dang kich hoat Che do Can bang (Balanced Mode)...
powercfg /setactive %GUID_BALANCED% 2>nul
if %errorlevel% neq 0 (
    powercfg /duplicatescheme %GUID_BALANCED% >nul 2>&1
    powercfg /setactive %GUID_BALANCED%
)
powercfg /setacvalueindex SCHEME_CURRENT SUB_PROCESSOR PROCTHROTTLEMIN 5
powercfg /setacvalueindex SCHEME_CURRENT SUB_PROCESSOR PROCTHROTTLEMAX 100
powercfg /setacvalueindex SCHEME_CURRENT SUB_PROCESSOR PERFBOOSTMODE 2
powercfg /setactive SCHEME_CURRENT
echo [OK] Da kich hoat thanh cong Che do Can bang (Balanced Mode, 100%% Max, Turbo Bat).
echo [%date% %time%] Da kich hoat Che do Can bang >> "%LOG_FILE%"
pause
goto :menu

:: ------------------------------------------------------------------------------
:: 4. CHE DO HIEU NANG CAO (HIGH PERFORMANCE)
:: ------------------------------------------------------------------------------
:set_high
cls
echo [*] Dang kich hoat Che do Hieu nang cao (High Performance)...
powercfg /setactive %GUID_HIGH_PERF% 2>nul
if %errorlevel% neq 0 (
    powercfg /duplicatescheme %GUID_HIGH_PERF% >nul 2>&1
    powercfg /setactive %GUID_HIGH_PERF%
)
powercfg /setacvalueindex SCHEME_CURRENT SUB_PROCESSOR PROCTHROTTLEMIN 100
powercfg /setacvalueindex SCHEME_CURRENT SUB_PROCESSOR PROCTHROTTLEMAX 100
powercfg /setacvalueindex SCHEME_CURRENT SUB_PROCESSOR PERFBOOSTMODE 2
powercfg /setactive SCHEME_CURRENT
echo [OK] Da kich hoat thanh cong Che do Hieu nang cao (High Performance).
echo [%date% %time%] Da kich hoat Che do High Performance >> "%LOG_FILE%"
pause
goto :menu

:: ------------------------------------------------------------------------------
:: 5. CHE DO MAY TRAM TOI THUONG (ULTIMATE PERFORMANCE)
:: ------------------------------------------------------------------------------
:set_ultimate
cls
echo [*] Dang kich hoat Che do May tram toi thuong (Ultimate Performance)...
powercfg /setactive %GUID_ULTIMATE% 2>nul
if %errorlevel% neq 0 (
    echo [*] Mo khoa scheme Ultimate Performance tren Windows...
    powercfg -duplicatescheme %GUID_ULTIMATE% >nul 2>&1
    powercfg /setactive %GUID_ULTIMATE%
)
powercfg /setacvalueindex SCHEME_CURRENT SUB_PROCESSOR PROCTHROTTLEMIN 100
powercfg /setacvalueindex SCHEME_CURRENT SUB_PROCESSOR PROCTHROTTLEMAX 100
powercfg /setacvalueindex SCHEME_CURRENT SUB_PROCESSOR PERFBOOSTMODE 2
powercfg /setacvalueindex SCHEME_CURRENT SUB_PROCESSOR PROCFREQMAX 0
powercfg /setactive SCHEME_CURRENT
echo [OK] Da kich hoat thanh cong Che do Ultimate Performance (Toi uu cho Xeon W5-3435X).
echo [%date% %time%] Da kich hoat Che do Ultimate Performance >> "%LOG_FILE%"
pause
goto :menu

:: ------------------------------------------------------------------------------
:: 6. BAT INTEL TURBO BOOST
:: ------------------------------------------------------------------------------
:turbo_on
cls
echo [*] Dang bat che do Intel Turbo Boost (Aggressive Mode)...
powercfg /setacvalueindex SCHEME_CURRENT SUB_PROCESSOR PERFBOOSTMODE 2
powercfg /setacvalueindex SCHEME_CURRENT SUB_PROCESSOR PROCTHROTTLEMAX 100
powercfg /setactive SCHEME_CURRENT
echo [OK] Intel Turbo Boost da duoc BAT (Tuong duong no_turbo = 0 tren Linux).
echo [%date% %time%] Da BAT Intel Turbo Boost >> "%LOG_FILE%"
pause
goto :menu

:: ------------------------------------------------------------------------------
:: 7. TAT INTEL TURBO BOOST
:: ------------------------------------------------------------------------------
:turbo_off
cls
echo [*] Dang tat che do Intel Turbo Boost (Giu xung nhip Base, ha nhiet do)...
powercfg /setacvalueindex SCHEME_CURRENT SUB_PROCESSOR PERFBOOSTMODE 0
powercfg /setactive SCHEME_CURRENT
echo [OK] Intel Turbo Boost da duoc TAT (Tuong duong no_turbo = 1 tren Linux).
echo [%date% %time%] Da TAT Intel Turbo Boost >> "%LOG_FILE%"
pause
goto :menu

:: ------------------------------------------------------------------------------
:: 8. KHONG CHE XUNG NHIP TOI DA (FREQUENCY CAP MHZ)
:: ------------------------------------------------------------------------------
:set_freq_input
cls
echo ================================================================================
echo  KHONG CHE GIOI HAN XUNG NHIP TOI DA (MHz)
echo ================================================================================
echo Huong dan:
echo   - Nhap so MHz mong muon (vi du: 3000 de gioi han o 3.0 GHz, 3500 cho 3.5 GHz)
echo   - Nhap 0 de GO BO gioi han (cho phep CPU chay toi da xung Boost)
echo.
set /p "freq_val=Nhap gia tri MHz (0 = khong gioi han): "

if not defined freq_val (
    echo [!] Gia tri khong hop le.
    pause
    goto :menu
)

powercfg /setacvalueindex SCHEME_CURRENT SUB_PROCESSOR PROCFREQMAX %freq_val%
powercfg /setactive SCHEME_CURRENT
echo [OK] Da thiet lap PROCFREQMAX = %freq_val% MHz tren Power Scheme hien tai.
echo [%date% %time%] Da thiet lap PROCFREQMAX = %freq_val% MHz >> "%LOG_FILE%"
pause
goto :menu

:set_freq_cli
set "freq_val=%~2"
if not defined freq_val set "freq_val=0"
powercfg /setacvalueindex SCHEME_CURRENT SUB_PROCESSOR PROCFREQMAX %freq_val%
powercfg /setactive SCHEME_CURRENT
echo [OK] Da thiet lap PROCFREQMAX = %freq_val% MHz.
exit /b 0

:: ------------------------------------------------------------------------------
:: 9. THIET LAP PL1 / PL2 QUA INTEL XTU CLI
:: ------------------------------------------------------------------------------
:xtu_rapl
cls
echo ================================================================================
echo  THIET LAP PL1 / PL2 QUA INTEL XTU CLI
echo ================================================================================
if not defined XTU_CLI (
    echo [!] Khong tim thay Intel XTU tren may tinh cua ban!
    echo De thiet lap chinh xac PL1 va PL2 (Watts) tren Windows nhu Linux RAPL,
    echo ban co the cai dat Intel Extreme Tuning Utility (Intel XTU):
    echo https://www.intel.com/content/www/us/en/download/17881/intel-extreme-tuning-utility-intel-xtu.html
    echo.
    echo *Luu y: Ban van co the dung cac muc 2, 3, 4, 5, 8 de khong che xung va dien rat hieu qua!
    pause
    goto :menu
)

echo [*] Dang liet ke cac thong so PL1 / PL2 ho tro tren CPU:
"%XTU_CLI%" -t -l | findstr /i "power boost tdp"
echo.
set /p "pl1_val=Nhap PL1 Long-Term Power Limit (Watts, vd: 180): "
set /p "pl2_val=Nhap PL2 Short-Term Power Limit (Watts, vd: 220): "

if defined pl1_val (
    echo [*] Dang dat PL1 = %pl1_val% W...
    "%XTU_CLI%" -t -id 48 -v %pl1_val%
)
if defined pl2_val (
    echo [*] Dang dat PL2 = %pl2_val% W...
    "%XTU_CLI%" -t -id 49 -v %pl2_val%
)
echo [OK] Da gui lenh thiet lap PL1/PL2 qua Intel XTU CLI.
echo [%date% %time%] Da thiet lap XTU PL1=%pl1_val%W, PL2=%pl2_val%W >> "%LOG_FILE%"
pause
goto :menu

:: ------------------------------------------------------------------------------
:: 10. GIAM SAT XUNG NHIP & TAI CPU THOI GIAN THUC
:: ------------------------------------------------------------------------------
:monitor
cls
echo ================================================================================
echo  GIAM SAT XUNG NHIP VA MUC TAI CPU THOI GIAN THUC (Nhan Ctrl + C de thoat)
echo ================================================================================
powershell -NoProfile -Command "while($true) { $p = Get-CimInstance Win32_Processor; Write-Host ('[' + (Get-Date -Format 'HH:mm:ss') + '] CPU: ' + $p.Name + ' | Xung nhip: ' + $p.CurrentClockSpeed + ' MHz | Tai: ' + $p.LoadPercentage + '%'); Start-Sleep -Seconds 1 }"
goto :menu

:: ------------------------------------------------------------------------------
:: 11. XEM NHAT KY HOAT DONG
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
echo   %~nx0             : Mo Menu tuong tac quan ly CPU
echo   %~nx0 status      : Xem thong tin CPU va Power Scheme hien tai
echo   %~nx0 monitor     : Giam sat xung nhip va muc tai CPU thoi gian thuc
echo   %~nx0 eco         : Kich hoat che do Tiet kiem dien (Eco)
echo   %~nx0 balanced    : Kich hoat che do Can bang (Balanced)
echo   %~nx0 high        : Kich hoat che do Hieu nang cao (High Performance)
echo   %~nx0 ultimate    : Kich hoat che do May tram toi thuong (Ultimate Performance)
echo   %~nx0 turbo_on    : Bat Intel Turbo Boost
echo   %~nx0 turbo_off   : Tat Intel Turbo Boost
echo   %~nx0 freq [MHz]  : Khong che xung nhip toi da theo MHz (0 = bo gioi han)
exit /b 0

