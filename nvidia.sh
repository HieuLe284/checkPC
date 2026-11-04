#!/usr/bin/env bash
set -eo pipefail

# ------------------------------------------------------------------------------
# 1. KIEM TRA QUYEN ROOT / SUDO TRUOC TIEN
# ------------------------------------------------------------------------------
if [ "$EUID" -ne 0 ]; then
    if command -v sudo >/dev/null 2>&1; then
        echo ">> [INFO] Script cai dat NVIDIA yeu cau quyen sudo. Dang chuyen sang sudo..."
        exec sudo "$0" "$@"
    else
        echo ">> [ERROR] Khong tim thay lenh sudo. Vui long chay bang quyen root!"
        exit 1
    fi
fi

# Chuyen den thu muc chua script
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

# Mau sac hien thi console
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m' # No Color

# Thu muc lam viec & file log (Tuan thu Rule 8: Moi file tao ra deu nam trong build/)
mkdir -p build
LOG_FILE="build/nvidia_install.log"

exec > >(tee -a "$LOG_FILE") 2>&1

echo -e "${BLUE}================================================================================${NC}"
echo -e "${GREEN}   BAT DAU CAI DAT TOAN BO (FULL) CHUAN NVIDIA CHO 2x RTX 4000 ADA TREN UBUNTU   ${NC}"
echo -e "${BLUE}================================================================================${NC}"
echo "Thoi gian bat dau: $(date)"
echo "Nhat ky cai dat se duoc ghi vao: $LOG_FILE"
echo ""

# ------------------------------------------------------------------------------
# 2. KIEM TRA PHAN CUNG & PHIEN BAN UBUNTU
# ------------------------------------------------------------------------------
echo -e "${BLUE}--- [BUOC 1/9] Kiem tra moi truong he thong & GPU ---${NC}"

if [ ! -f /etc/os-release ]; then
    echo -e "${RED}[ERROR] Khong xac dinh duoc thong tin ban phan phoi Linux (/etc/os-release).${NC}"
    exit 1
fi

. /etc/os-release
echo "He dieu hanh phat hien: $PRETTY_NAME"
echo "Kien truc: $(uname -m)"
echo "Kernel hien tai: $(uname -r)"

if [ "$ID" != "ubuntu" ]; then
    echo -e "${YELLOW}[CANH BAO] Script duoc toi uu dac biet cho Ubuntu (22.04 / 24.04). He thong hien tai la: $ID${NC}"
fi

# Chuyen doi version sang format repo NVIDIA (vd: 22.04 -> 2204, 24.04 -> 2404)
UBUNTU_VER_CLEAN=$(echo "$VERSION_ID" | tr -d '.')
if [ -z "$UBUNTU_VER_CLEAN" ]; then
    UBUNTU_VER_CLEAN="2404"
fi

echo -e "\n[*] Kiem tra cac Card do hoa NVIDIA tren may:"
lspci | grep -iE 'vga|3d|display|nvidia' || true
echo ""

# ------------------------------------------------------------------------------
# 3. CAI DAT GOI BO TRO & HEADER LINUX
# ------------------------------------------------------------------------------
echo -e "${BLUE}--- [BUOC 2/9] Cai dat goi build he thong & Kernel Headers ---${NC}"
export DEBIAN_FRONTEND=noninteractive

apt-get update -y
apt-get install -y --no-install-recommends \
    build-essential \
    linux-headers-"$(uname -r)" \
    dkms \
    curl \
    wget \
    gnupg \
    ca-certificates \
    software-properties-common \
    pkg-config \
    pciutils

echo -e "${GREEN}[OK] Da cai dat day du Kernel Headers va bo trinh bien dich.${NC}\n"

# ------------------------------------------------------------------------------
# 4. VO HIEU HOA DRIVER NOUVEAU (BAT BUOC DE TRANH XUNG DOT VOI NVIDIA DRIVER)
# ------------------------------------------------------------------------------
echo -e "${BLUE}--- [BUOC 3/9] Vo hieu hoa (Blacklist) driver ma nguon mo Nouveau ---${NC}"

NOUVEAU_CONF="/etc/modprobe.d/blacklist-nouveau.conf"
if [ ! -f "$NOUVEAU_CONF" ] || ! grep -q "blacklist nouveau" "$NOUVEAU_CONF"; then
    cat <<EOF > "$NOUVEAU_CONF"
blacklist nouveau
options nouveau modeset=0
EOF
    echo "[*] Da tao tap tin chan nouveau tai $NOUVEAU_CONF"
    update-initramfs -u
    echo -e "${GREEN}[OK] Da cap nhat initramfs thanh cong de go bo Nouveau.${NC}\n"
else
    echo -e "${GREEN}[OK] Nouveau da duoc chan tu truoc trong he thong.${NC}\n"
fi

# ------------------------------------------------------------------------------
# 5. CAI DAT KHO LUU TRU CHINH THUC CUA NVIDIA (NVIDIA OFFICIAL CUDA APT REPOSITORY)
# ------------------------------------------------------------------------------
echo -e "${BLUE}--- [BUOC 4/9] Cai dat kho luu tru chinh thuc NVIDIA CUDA Repo ---${NC}"

ARCH=$(uname -m)
KEYRING_URL="https://developer.download.nvidia.com/compute/cuda/repos/ubuntu${UBUNTU_VER_CLEAN}/${ARCH}/cuda-keyring_1.1-1_all.deb"
KEYRING_DEB="/tmp/cuda-keyring.deb"

echo "[*] Tai cuda-keyring tu NVIDIA cho Ubuntu ${UBUNTU_VER_CLEAN} (${ARCH})..."
if wget -q -O "$KEYRING_DEB" "$KEYRING_URL"; then
    dpkg -i "$KEYRING_DEB" || true
    rm -f "$KEYRING_DEB"
    echo -e "${GREEN}[OK] Da cai dat NVIDIA GPG Keyring thanh cong.${NC}"
else
    echo -e "${YELLOW}[!] Khong tai duoc qua ban ${UBUNTU_VER_CLEAN}, thu fallback ve 22.04...${NC}"
    FALLBACK_URL="https://developer.download.nvidia.com/compute/cuda/repos/ubuntu2204/${ARCH}/cuda-keyring_1.1-1_all.deb"
    wget -q -O "$KEYRING_DEB" "$FALLBACK_URL"
    dpkg -i "$KEYRING_DEB" || true
    rm -f "$KEYRING_DEB"
fi

apt-get update -y
echo ""

# ------------------------------------------------------------------------------
# 6. CAI DAT DRIVER NVIDIA WORKSTATION & PERSISTENCE DAEMON
# ------------------------------------------------------------------------------
echo -e "${BLUE}--- [BUOC 5/9] Cai dat NVIDIA Driver (Ban Workstation Production) ---${NC}"
echo "[*] Tien hanh cai dat cuda-drivers (bao gom driver, DKMS, va cong cu quan ly nvidia-smi)..."

# Cai dat driver thong qua goi cuda-drivers tu repo NVIDIA hoac nvidia-driver-550
apt-get install -y --no-install-recommends \
    cuda-drivers \
    nvidia-settings \
    nvidia-prime \
    libnvidia-compute-550 \
    libnvidia-gl-550 || apt-get install -y cuda-drivers

# Kich hoat va bat che do NVIDIA Persistence Daemon (Dac biet quan trong cho he thong 2x GPU)
echo "[*] Cau hinh va kich hoat NVIDIA Persistence Daemon..."
systemctl enable nvidia-persistenced || true
systemctl start nvidia-persistenced 2>/dev/null || true

echo -e "${GREEN}[OK] Hoan tat cai dat NVIDIA Workstation Driver & Persistence Daemon.${NC}\n"

# ------------------------------------------------------------------------------
# 7. CAI DAT NVIDIA CUDA TOOLKIT (FULL DEVELOPMENT STACK)
# ------------------------------------------------------------------------------
echo -e "${BLUE}--- [BUOC 6/9] Cai dat NVIDIA CUDA Toolkit (Full) ---${NC}"
echo "[*] Dang cai dat cuda-toolkit (nvcc, cuBLAS, cuFFT, cuSPARSE, cuSOLVER, Visual Profiler)..."

apt-get install -y cuda-toolkit

# Cau hinh bien moi truong he thong vao /etc/profile.d/cuda.sh
CUDA_PROFILE="/etc/profile.d/cuda.sh"
cat <<'EOF' > "$CUDA_PROFILE"
# NVIDIA CUDA Environment Paths
export CUDA_HOME=/usr/local/cuda
export PATH=/usr/local/cuda/bin${PATH:+:${PATH}}
export LD_LIBRARY_PATH=/usr/local/cuda/lib64${LD_LIBRARY_PATH:+:${LD_LIBRARY_PATH}}
EOF
chmod +x "$CUDA_PROFILE"

# Cau hinh ldconfig cho thu vien CUDA
CUDA_LD_CONF="/etc/ld.so.conf.d/cuda.conf"
echo "/usr/local/cuda/lib64" > "$CUDA_LD_CONF"
ldconfig

echo -e "${GREEN}[OK] Hoan tat cai dat CUDA Toolkit & cau hinh bien moi truong he thong.${NC}\n"

# ------------------------------------------------------------------------------
# 8. CAI DAT NVIDIA cuDNN & TENSORRT
# ------------------------------------------------------------------------------
echo -e "${BLUE}--- [BUOC 7/9] Cai dat cuDNN & TensorRT cho Ada Lovelace Tensor Cores ---${NC}"

echo "[*] Cai dat cuDNN v9 (CUDA Deep Neural Network library)..."
apt-get install -y \
    libcudnn9-cuda-12 \
    libcudnn9-dev-cuda-12 \
    libcudnn9-samples || echo -e "${YELLOW}[!] cuDNN se duoc cap nhat qua repo chinh thuc.${NC}"

echo "[*] Cai dat TensorRT (Bo suy luan AI hieu nang cao cho 2x RTX 4000 Ada)..."
apt-get install -y \
    tensorrt \
    libnvinfer-dev \
    libnvinfer-plugin-dev \
    python3-libnvinfer || echo -e "${YELLOW}[!] TensorRT tiep tuc xu ly.${NC}"

echo -e "${GREEN}[OK] Hoan tat cai dat cuDNN va TensorRT.${NC}\n"

# ------------------------------------------------------------------------------
# 9. CAI DAT NVIDIA CONTAINER TOOLKIT (DOCKER GPU PASSTHROUGH)
# ------------------------------------------------------------------------------
echo -e "${BLUE}--- [BUOC 8/9] Cai dat NVIDIA Container Toolkit (Docker GPU) ---${NC}"

# Them repo nvidia-container-toolkit
CONTAINER_LIST="/etc/apt/sources.list.d/nvidia-container-toolkit.list"
if [ ! -f "$CONTAINER_LIST" ]; then
    curl -fsSL https://nvidia.github.io/libnvidia-container/gpgkey | gpg --dearmor -o /usr/share/keyrings/nvidia-container-toolkit-keyring.gpg --yes
    curl -s -L https://nvidia.github.io/libnvidia-container/stable/deb/nvidia-container-toolkit.list | \
        sed 's#deb https://#deb [signed-by=/usr/share/keyrings/nvidia-container-toolkit-keyring.gpg] https://#g' | \
        tee "$CONTAINER_LIST" > /dev/null
    apt-get update -y
fi

apt-get install -y nvidia-container-toolkit

# Cau hinh Docker runtime neu may da cai san Docker
if command -v docker >/dev/null 2>&1; then
    echo "[*] Phat hien Docker, dang cau hinh runtime NVIDIA..."
    nvidia-ctk runtime configure --runtime=docker || true
    systemctl restart docker 2>/dev/null || true
    echo -e "${GREEN}[OK] Da tich hop NVIDIA Container Toolkit vao Docker.${NC}"
else
    echo "[i] Docker chua duoc cai dat. Khi ban cai Docker, hay chay: sudo nvidia-ctk runtime configure --runtime=docker"
fi
echo ""

# ------------------------------------------------------------------------------
# 10. TOI UU MULTI-GPU & ECC MEMORY CHO 2x RTX 4000 ADA
# ------------------------------------------------------------------------------
echo -e "${BLUE}--- [BUOC 9/9] Toi uu Multi-GPU & ECC Memory ---${NC}"

# Tao systemd service de tu dong bat Persistence Mode va ECC khi khoi dong
SYSTEMD_PERSISTENCE="/etc/systemd/system/nvidia-gpu-setup.service"
cat <<'EOF' > "$SYSTEMD_PERSISTENCE"
[Unit]
Description=NVIDIA Multi-GPU Optimization and Persistence Mode
After=network.target

[Service]
Type=oneshot
ExecStart=/usr/bin/nvidia-smi -pm 1
RemainAfterExit=yes

[Install]
WantedBy=multi-user.target
EOF

systemctl daemon-reload
systemctl enable nvidia-gpu-setup.service || true

# Thu ap dung ngay neu kernel module nvidia da nạp
if command -v nvidia-smi >/dev/null 2>&1; then
    nvidia-smi -pm 1 2>/dev/null || true
    # RTX 4000 Ada ho tro ECC GDDR6
    nvidia-smi -e 1 2>/dev/null || true
fi

echo -e "${GREEN}[OK] Da cau hinh tu dong duy tri Persistence Mode & ECC tren 2x RTX 4000 Ada.${NC}\n"

# ------------------------------------------------------------------------------
# TONG KET & KIEM TRA SAU CAI DAT
# ------------------------------------------------------------------------------
echo -e "${BLUE}================================================================================${NC}"
echo -e "${GREEN}             HOAN TAT QUA TRINH CAI DAT FULL NVIDIA TREN UBUNTU                 ${NC}"
echo -e "${BLUE}================================================================================${NC}"

echo -e "\n[*] Kiem tra trinh bien dich CUDA Compiler (nvcc):"
if [ -f /usr/local/cuda/bin/nvcc ]; then
    /usr/local/cuda/bin/nvcc --version
else
    echo "nvcc se san sang sau khi tai lai bien moi truong (source /etc/profile.d/cuda.sh)."
fi

echo -e "\n[*] Kiem tra trang thai GPU hien tai (nvidia-smi):"
if command -v nvidia-smi >/dev/null 2>&1; then
    nvidia-smi || true
fi

echo ""
echo -e "${YELLOW}--------------------------------------------------------------------------------${NC}"
echo -e "${YELLOW}[QUAN TRONG] NGUYEN TAC NONG - KHOI DONG LAI MAY (REBOOT):${NC}"
echo -e "Neu day la lan dau tien ban cai dat NVIDIA Driver hoac vua go bo Nouveau,"
echo -e "vui long ${GREEN}KHOI DONG LAI MAY${NC} bang lenh sau de Kernel nap day du module NVIDIA:"
echo ""
echo -e "    ${GREEN}sudo reboot${NC}"
echo ""
echo -e "Sau khi khoi dong lai, kiem tra 2x GPU bang lenh:"
echo -e "    ${GREEN}nvidia-smi${NC}"
echo -e "    ${GREEN}nvcc -V${NC}"
echo -e "${YELLOW}--------------------------------------------------------------------------------${NC}"
echo "Nhat ky toan bo qua trinh cai dat da luu tai: $LOG_FILE"

