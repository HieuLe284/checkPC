#!/bin/bash
# =============================================================================
# Environment Check Script
# Kiểm tra toàn bộ môi trường phát triển đã được cài đặt đầy đủ chưa
# =============================================================================

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
BOLD='\033[1m'
NC='\033[0m' # No Color

PASS=0
FAIL=0
WARN=0

print_header() {
    echo ""
    echo -e "${BOLD}${BLUE}══════════════════════════════════════════════════════════${NC}"
    echo -e "${BOLD}${BLUE}  $1${NC}"
    echo -e "${BOLD}${BLUE}══════════════════════════════════════════════════════════${NC}"
}

print_section() {
    echo ""
    echo -e "${CYAN}▶ $1${NC}"
    echo -e "${CYAN}──────────────────────────────────────────────────────────${NC}"
}

ok() {
    echo -e "  ${GREEN}✔${NC}  $1"
    ((PASS++))
}

fail() {
    echo -e "  ${RED}✘${NC}  $1"
    ((FAIL++))
}

warn() {
    echo -e "  ${YELLOW}⚠${NC}  $1"
    ((WARN++))
}

# Kiểm tra lệnh có tồn tại không
check_cmd() {
    local name="$1"
    local cmd="$2"
    if command -v "$cmd" &>/dev/null; then
        local ver
        ver=$(eval "$cmd --version 2>/dev/null | head -1" 2>/dev/null || echo "installed")
        ok "$name: $ver"
    else
        fail "$name: không tìm thấy lệnh '$cmd'"
    fi
}

# Kiểm tra package apt đã cài chưa
check_pkg() {
    local pkg="$1"
    if dpkg -l "$pkg" 2>/dev/null | grep -q "^ii"; then
        local ver
        ver=$(dpkg -l "$pkg" 2>/dev/null | grep "^ii" | awk '{print $3}')
        ok "Package $pkg ($ver)"
    else
        fail "Package $pkg: chưa cài"
    fi
}

# Kiểm tra file/thư mục tồn tại
check_path() {
    local name="$1"
    local path="$2"
    if [ -e "$path" ]; then
        ok "$name: $path"
    else
        fail "$name: không tìm thấy $path"
    fi
}

# Kiểm tra shared library
check_lib() {
    local lib="$1"
    if ldconfig -p 2>/dev/null | grep -q "$lib"; then
        ok "Library $lib"
    else
        fail "Library $lib: không tìm thấy"
    fi
}

# =============================================================================
print_header "KIỂM TRA MÔI TRƯỜNG HỆ THỐNG"
echo -e "  Thời gian: $(date)"
echo -e "  Host: $(hostname)"
echo -e "  OS: $(lsb_release -d 2>/dev/null | cut -f2 || cat /etc/os-release | grep PRETTY_NAME | cut -d= -f2)"
echo -e "  Kernel: $(uname -r)"
echo -e "  User: $(whoami)"

# =============================================================================
print_header "3. BUILD TOOLS"

print_section "build-essential"
check_pkg "build-essential"
check_cmd "gcc" "gcc"
check_cmd "g++" "g++"
check_cmd "make" "make"

print_section "CMake"
check_cmd "cmake" "cmake"

print_section "ninja-build"
check_cmd "ninja" "ninja"

# =============================================================================
print_header "4. NVIDIA DRIVER"

print_section "NVIDIA Driver & nvidia-smi"
if command -v nvidia-smi &>/dev/null; then
    ok "nvidia-smi: $(nvidia-smi --query-gpu=driver_version --format=csv,noheader 2>/dev/null | head -1)"
    nvidia-smi --query-gpu=name,driver_version,memory.total --format=csv,noheader 2>/dev/null | while IFS=, read -r name drv mem; do
        echo -e "     GPU: ${name} | Driver: ${drv} | VRAM: ${mem}"
    done
else
    fail "nvidia-smi: không tìm thấy (NVIDIA Driver chưa cài hoặc không có GPU NVIDIA)"
fi

# =============================================================================
print_header "5. LIBRARIES"

print_section "Boost"
check_pkg "libboost-dev"
check_lib "libboost"

print_section "OpenGL / Mesa"
for pkg in libglu1-mesa-dev freeglut3-dev mesa-common-dev libglfw3-dev libgl1-mesa-dev libglew-dev libftgl-dev; do
    check_pkg "$pkg"
done

print_section "Qt5 Dependencies (xcb, x11, fonts, ...)"
QT_DEPS=(
    libfontconfig1-dev libfreetype-dev libx11-dev libx11-xcb-dev libxext-dev
    libxfixes-dev libxi-dev libxrender-dev libxcb1-dev libxcb-cursor-dev
    libxcb-glx0-dev libxcb-keysyms1-dev libxcb-image0-dev libxcb-shm0-dev
    libxcb-icccm4-dev libxcb-sync-dev libxcb-xfixes0-dev libxcb-shape0-dev
    libxcb-randr0-dev libxcb-render-util0-dev libxcb-util-dev
    libxcb-xinerama0-dev libxcb-xkb-dev libxkbcommon-dev libxkbcommon-x11-dev
)
for pkg in "${QT_DEPS[@]}"; do
    check_pkg "$pkg"
done

print_section "Qt5 Runtime libs"
QT_RT=(
    libfontconfig1 libxcb-glx0 libx11-xcb1 libxcb-icccm4 libxcb-image0
    libxcb-keysyms1 libxcb-randr0 libxcb-render-util0 libxcb-sync1
    libxcb-xfixes0 libxcb-xinerama0 libxcb-xkb1 libxkbcommon-x11-0
    libgl1 libegl1 libxcb-shape0 libxcb-cursor0 libglx-dev
)
for pkg in "${QT_RT[@]}"; do
    check_pkg "$pkg"
done

print_section "Qt5 Dev packages"
QT5_PKGS=(
    zlib1g-dev gcc g++ graphviz doxygen gettext git
    qtscript5-dev libqt5svg5-dev qttools5-dev-tools qttools5-dev
    libqt5opengl5-dev qtmultimedia5-dev libqt5multimedia5-plugins
    libqt5serialport5 libqt5serialport5-dev qtpositioning5-dev
    libgps-dev libqt5positioning5 libqt5positioning5-plugins
)
for pkg in "${QT5_PKGS[@]}"; do
    check_pkg "$pkg"
done

print_section "libclang-dev"
check_pkg "libclang-dev"

print_section "libgdal-dev"
check_pkg "libgdal-dev"
check_cmd "gdal-config" "gdal-config"

print_section "GLog / GLM / CUPS"
check_pkg "libgoogle-glog-dev"
check_pkg "libglm-dev"
check_pkg "libcups2-dev"

print_section "xcb-cursor"
check_pkg "libxcb-cursor0"
check_pkg "libxcb-cursor-dev"

print_section "RTL-SDR"
check_pkg "librtlsdr-dev"
check_lib "librtlsdr"

print_section "SoundTouch"
check_pkg "libsoundtouch-dev"
check_lib "libSoundTouch"

print_section "ALSA (asoundlib)"
check_pkg "libasound2-dev"
check_lib "libasound"

# =============================================================================
print_header "6. OTHER TOOLS"

print_section "Git"
check_cmd "git" "git"
check_pkg "git"

print_section "OpenSSH Server"
check_pkg "openssh-server"
if systemctl is-active --quiet ssh 2>/dev/null; then
    ok "SSH service: đang chạy"
else
    warn "SSH service: chưa chạy (có thể chưa start)"
fi

print_section "GitHub Desktop"
if command -v github-desktop &>/dev/null || dpkg -l github-desktop 2>/dev/null | grep -q "^ii"; then
    ok "GitHub Desktop: đã cài"
else
    warn "GitHub Desktop: chưa cài (tuỳ chọn)"
fi

print_section "Net-tools"
check_pkg "net-tools"
check_cmd "ifconfig" "ifconfig"

print_section "Wireshark"
check_cmd "wireshark" "wireshark"

print_section "FFmpeg"
check_cmd "ffmpeg" "ffmpeg"

print_section "Terminator"
check_cmd "terminator" "terminator"

# =============================================================================
print_header "7. FONTS"

print_section "fonts-roboto"
check_pkg "fonts-roboto"

# =============================================================================
print_header "8. ROS2 ROLLING"

print_section "Locale"
if locale 2>/dev/null | grep -q "en_US.UTF-8"; then
    ok "Locale: en_US.UTF-8"
else
    warn "Locale: en_US.UTF-8 chưa được set (kiểm tra: locale)"
fi

print_section "ROS2 Repository"
if [ -f /etc/apt/sources.list.d/ros2.list ]; then
    ok "ROS2 apt source: /etc/apt/sources.list.d/ros2.list"
else
    fail "ROS2 apt source: không tìm thấy"
fi

if [ -f /usr/share/keyrings/ros-archive-keyring.gpg ]; then
    ok "ROS2 GPG key: /usr/share/keyrings/ros-archive-keyring.gpg"
else
    fail "ROS2 GPG key: không tìm thấy"
fi

print_section "ROS2 Installation"
if [ -d /opt/ros/rolling ]; then
    ok "ROS2 Rolling: /opt/ros/rolling"
else
    fail "ROS2 Rolling: /opt/ros/rolling không tồn tại"
fi

check_pkg "ros-rolling-desktop"
check_pkg "ros-dev-tools"

print_section "ROS2 Commands"
if source /opt/ros/rolling/setup.bash 2>/dev/null && command -v ros2 &>/dev/null; then
    ok "ros2 CLI: $(ros2 --version 2>/dev/null | head -1)"
else
    fail "ros2: không thể chạy (chưa source setup.bash hoặc chưa cài)"
fi

print_section "colcon"
if command -v colcon &>/dev/null; then
    ok "colcon: $(colcon version-check 2>/dev/null | head -1 || echo 'installed')"
else
    check_pkg "python3-colcon-common-extensions"
    fail "colcon: lệnh không tìm thấy"
fi

# =============================================================================
print_header "9. Qt INSTALLATION"

print_section "Qt6 Binary (qmake6)"
if command -v qmake6 &>/dev/null; then
    ok "qmake6: $(qmake6 --version 2>/dev/null | tail -1)"
elif command -v qmake &>/dev/null; then
    ok "qmake: $(qmake --version 2>/dev/null | tail -1)"
else
    warn "qmake6 / qmake: không tìm thấy trong PATH"
fi

print_section "Qt installation directory"
QT_DIRS=(/home/*/Qt /home/*/Qt/6.7.3 /home/*/Qt/Tools/QtCreator)
FOUND_QT=0
for d in "${QT_DIRS[@]}"; do
    for expanded in $d; do
        if [ -d "$expanded" ]; then
            ok "Qt directory: $expanded"
            FOUND_QT=1
        fi
    done
done
if [ "$FOUND_QT" -eq 0 ]; then
    warn "Thư mục Qt (/home/*/Qt) không tìm thấy (có thể chưa cài hoặc đường dẫn khác)"
fi

print_section "Qt Creator Desktop Launcher"
LAUNCHER_FOUND=0
for f in /home/*/.local/share/applications/org.qt-project.qtcreator.desktop; do
    if [ -f "$f" ]; then
        ok "Qt Creator launcher: $f"
        LAUNCHER_FOUND=1
    fi
done
if [ "$LAUNCHER_FOUND" -eq 0 ]; then
    warn "Qt Creator .desktop file chưa tạo"
fi

print_section "Qt6 trong PATH"
if echo "$PATH" | grep -q "Qt.*gcc_64"; then
    ok "Qt gcc_64/bin trong PATH"
elif grep -q "Qt.*gcc_64" /home/*/.bashrc 2>/dev/null; then
    warn "Qt PATH được set trong .bashrc nhưng chưa export trong shell hiện tại"
else
    warn "Qt gcc_64/bin chưa được thêm vào PATH"
fi

# =============================================================================
print_header "10. SNAP7 LIBRARY"

print_section "libsnap7.so"
SNAP7=$(find /usr /usr/local -name "libsnap7.so*" 2>/dev/null | head -1)
if [ -n "$SNAP7" ]; then
    ok "snap7 library: $SNAP7"
else
    fail "libsnap7.so: không tìm thấy"
fi

if ldconfig -p 2>/dev/null | grep -q "libsnap7"; then
    ok "snap7 trong ldconfig cache"
else
    warn "snap7 chưa trong ldconfig cache (chạy: sudo ldconfig)"
fi

# =============================================================================
print_header "11. QWT LIBRARY"

print_section "Qwt"
QWT_DIR=/usr/local/qwt-6.3.0
if [ -d "$QWT_DIR" ]; then
    ok "Qwt installed: $QWT_DIR"
else
    fail "Qwt: $QWT_DIR không tồn tại"
fi

QWT_LIB=$(find /usr/local -name "libqwt*.so*" 2>/dev/null | head -1)
if [ -n "$QWT_LIB" ]; then
    ok "libqwt: $QWT_LIB"
else
    fail "libqwt*.so: không tìm thấy"
fi

if [ -f /etc/ld.so.conf.d/qwt.conf ]; then
    ok "Qwt ldconfig config: /etc/ld.so.conf.d/qwt.conf"
else
    fail "Qwt ldconfig config: /etc/ld.so.conf.d/qwt.conf không tồn tại"
fi

if ldconfig -p 2>/dev/null | grep -q "libqwt"; then
    ok "Qwt trong ldconfig cache"
else
    warn "Qwt chưa trong ldconfig cache (chạy: sudo ldconfig)"
fi

# =============================================================================
print_header "12. DISPLAY SERVER (WAYLAND)"

print_section "GDM3 Wayland config"
GDM_CONF=/etc/gdm3/custom.conf
if [ -f "$GDM_CONF" ]; then
    if grep -q "WaylandEnable=false" "$GDM_CONF"; then
        ok "Wayland đã bị disable: WaylandEnable=false trong $GDM_CONF"
    else
        warn "Wayland chưa bị disable (chưa có WaylandEnable=false trong $GDM_CONF)"
    fi
else
    warn "$GDM_CONF không tồn tại (chưa cài gdm3 hoặc đường dẫn khác)"
fi

print_section "Display Server hiện tại"
if [ -n "$WAYLAND_DISPLAY" ]; then
    warn "Đang chạy trên Wayland (WAYLAND_DISPLAY=$WAYLAND_DISPLAY) — nên dùng X11"
elif [ -n "$DISPLAY" ]; then
    ok "Đang chạy trên X11 (DISPLAY=$DISPLAY)"
else
    warn "Không xác định được display server (có thể đang chạy headless)"
fi

# =============================================================================
print_header "TÓM TẮT KẾT QUẢ"
echo ""
TOTAL=$((PASS + FAIL + WARN))
echo -e "  Tổng kiểm tra : ${BOLD}$TOTAL${NC}"
echo -e "  ${GREEN}✔ PASS${NC}         : ${BOLD}$PASS${NC}"
echo -e "  ${RED}✘ FAIL${NC}         : ${BOLD}$FAIL${NC}"
echo -e "  ${YELLOW}⚠ WARNING${NC}      : ${BOLD}$WARN${NC}"
echo ""

if [ "$FAIL" -eq 0 ] && [ "$WARN" -eq 0 ]; then
    echo -e "  ${GREEN}${BOLD}🎉 Môi trường đầy đủ! Sẵn sàng build.${NC}"
elif [ "$FAIL" -eq 0 ]; then
    echo -e "  ${YELLOW}${BOLD}⚠  Có $WARN cảnh báo nhỏ, kiểm tra lại nếu cần.${NC}"
else
    echo -e "  ${RED}${BOLD}✘  Có $FAIL thành phần bị thiếu. Cần cài thêm trước khi build.${NC}"
fi
echo ""
echo -e "${BOLD}${BLUE}══════════════════════════════════════════════════════════${NC}"

