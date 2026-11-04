#!/usr/bin/env bash
set -e

# Chuyen den thu muc chua script (dam bao dung thu muc hien tai)
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

# Tu dong yeu cau quyen sudo neu chua phai la root de doc duoc toan bo SMBIOS/dmidecode
if [ "$EUID" -ne 0 ]; then
    if command -v sudo >/dev/null 2>&1; then
        echo ">> [INFO] Can quyen 'sudo' de doc toan bo Serial, UUID, va chi tiet tung thanh RAM (SMBIOS)."
        exec sudo "$0" "$@"
    fi
fi

# Tao thu muc build theo Rule 7 (Tat ca file tao ra nam trong build/)
mkdir -p build

echo "================================================================================"
echo " [BUILD] Dang bien dich chuong trinh C++ tren Ubuntu/Linux vao thu muc build/..."
echo "================================================================================"

# Bien dich main.cpp ra file thuc thi build/sysinfo
g++ -std=c++17 -O2 -Wall -Wextra main.cpp -o build/sysinfo -lpthread

echo "[BUILD] Bien dich thanh cong -> build/sysinfo"
echo "================================================================================"
echo " [RUN] Bat dau chay chuong trinh kiem tra he thong..."
echo "================================================================================"

./build/sysinfo

echo ""
echo "================================================================================"
echo " Ket qua da duoc luu vao cac tap tin trong thu muc build/:"
echo "   - build/hardware_report.txt  (Bao cao toan dien thong so phan cung he thong)"
echo "   - build/workstation.txt      (Bao cao doi chieu thong so May tram ASUS WTS)"
echo "================================================================================"
