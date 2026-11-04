#!/usr/bin/env bash
# ==============================================================================
# SCRIPT THIET LAP GIOI HAN CONG SUAT (POWER LIMIT) CPU INTEL QUA RAPL (LINUX)
# ------------------------------------------------------------------------------
# Chuc nang:
#   - Kiem tra va giam sat cong suat tieu thu dien hien tai cua CPU (Watts)
#   - Thiet lap gioi han cong suat dai han (PL1 - Long Term / TDP Sustain)
#   - Thiet lap gioi han cong suat ngan han (PL2 - Short Term / Turbo Burst)
#   - Tu dong ap dung cho tat ca CPU Socket / Package (Ho tro ca 1 Socket & Multi-Socket)
#   - Tich hop che do Menu tuong tac hoac truyen tham so dong lenh
#   - Tuy chon cai dat systemd service de duy tri gioi han sau khi khoi dong lai
# ==============================================================================

set -eo pipefail

# ------------------------------------------------------------------------------
# 1. KIEM TRA QUYEN ROOT / SUDO TRUOC TIEN
# ------------------------------------------------------------------------------
if [ "$EUID" -ne 0 ]; then
    if command -v sudo >/dev/null 2>&1; then
        echo ">> [INFO] Script yeu cau quyen sudo de truy cap Intel RAPL / sysfs. Dang chuyen sang sudo..."
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
CYAN='\033[0;36m'
BOLD='\033[1m'
NC='\033[0m' # No Color

# Thu muc lam viec & file log (Tuan thu Rule 8: moi file tao ra deu nam trong build/)
mkdir -p build
LOG_FILE="build/cpu_rapl.log"

# Ham ghi log
log_msg() {
    echo -e "$1"
    echo -e "$(date '+%Y-%m-%d %H:%M:%S') $1" | sed -r "s/\x1B\[([0-9]{1,3}(;[0-9]{1,2})?)?[mGK]//g" >> "$LOG_FILE"
}

# ------------------------------------------------------------------------------
# 2. KIEM TRA VA NAP MODULE INTEL RAPL TRONG LINUX KERNEL
# ------------------------------------------------------------------------------
RAPL_BASE="/sys/class/powercap/intel-rapl"

if [ ! -d "$RAPL_BASE" ]; then
    log_msg "${BLUE}[*] Dang nạp kernel module intel_rapl_common va intel_rapl_msr...${NC}"
    modprobe intel_rapl_common 2>/dev/null || true
    modprobe intel_rapl_msr 2>/dev/null || true
    sleep 0.5
fi

if [ ! -d "$RAPL_BASE" ]; then
    log_msg "${RED}[ERROR] He thong khong ho tro hoac chua bat giao tiep Intel RAPL (/sys/class/powercap/intel-rapl)!${NC}"
    log_msg "Vui long kiem tra BIOS co ho tro Intel RAPL / Power Management hay khong."
    exit 1
fi

# Tim cac thu muc package (Socket)
find_packages() {
    local pkgs=()
    for d in /sys/class/powercap/intel-rapl:*; do
        if [ -d "$d" ] && [ -f "$d/name" ]; then
            local name
            name=$(cat "$d/name")
            if [[ "$name" =~ package-[0-9]+ ]]; then
                pkgs+=("$d")
            fi
        fi
    done
    echo "${pkgs[@]}"
}

# ------------------------------------------------------------------------------
# 3. DO CONG SUAT TIEU THU DIEN TUC THOI (INSTANTANEOUS POWER MEASUREMENT)
# ------------------------------------------------------------------------------
get_live_power_watts() {
    local pkg_dir="$1"
    if [ ! -f "$pkg_dir/energy_uj" ]; then
        echo "N/A"
        return
    fi

    local e1 e2
    e1=$(cat "$pkg_dir/energy_uj" 2>/dev/null || echo 0)
    sleep 0.5
    e2=$(cat "$pkg_dir/energy_uj" 2>/dev/null || echo 0)

    if [ "$e2" -ge "$e1" ] && [ "$e1" -gt 0 ]; then
        local delta_uj=$((e2 - e1))
        # 0.5s = 500,000 us -> Watt = delta_uj / 500000
        local watts
        watts=$(awk "BEGIN {printf \"%.2f\", $delta_uj / 500000}")
        echo "${watts} W"
    else
        echo "0.00 W"
    fi
}

# ------------------------------------------------------------------------------
# 4. HIEN THI TRANG THAI GIOI HAN CONG SUAT HIEN TAI (STATUS / INFO)
# ------------------------------------------------------------------------------
show_status() {
    echo -e "${BLUE}================================================================================${NC}"
    echo -e "${BOLD}${CYAN}         TRANG THAI CONG SUAT CPU INTEL QUA INTEL RAPL (POWERCAP)        ${NC}"
    echo -e "${BLUE}================================================================================${NC}"

    local cpu_model
    cpu_model=$(lscpu 2>/dev/null | grep -E "^Model name:" | sed 's/Model name:[ \t]*//' || echo "Intel Processor")
    echo -e "CPU Model : ${BOLD}$cpu_model${NC}"
    echo -e "Thoi gian : $(date '+%Y-%m-%d %H:%M:%S')"
    echo ""

    local pkgs
    read -r -a pkgs <<< "$(find_packages)"

    if [ ${#pkgs[@]} -eq 0 ]; then
        echo -e "${RED}[!] Khong tim thay socket package CPU nao trong $RAPL_BASE!${NC}"
        return
    fi

    for pkg in "${pkgs[@]}"; do
        local pkg_name enabled live_power
        pkg_name=$(cat "$pkg/name")
        enabled=$(cat "$pkg/enabled" 2>/dev/null || echo "0")
        echo -e "${BOLD}--- [Socket / $pkg_name] ---${NC}"
        echo -e "  Thu muc sysfs    : $pkg"
        echo -e "  RAPL Status      : $( [ "$enabled" -eq 1 ] && echo -e "${GREEN}KICH HOAT (Enabled)${NC}" || echo -e "${RED}VO HIEU (Disabled)${NC}" )"

        echo -ne "  Dang do cong suat tieu thu thuc te (vui long doi 0.5s)... "
        live_power=$(get_live_power_watts "$pkg")
        echo -e "${BOLD}${GREEN}$live_power${NC}"

        # Doc cac rang buoc constraint
        for idx in 0 1; do
            if [ -f "$pkg/constraint_${idx}_name" ]; then
                local c_name c_uw c_w c_time_us c_time_s
                c_name=$(cat "$pkg/constraint_${idx}_name")
                c_uw=$(cat "$pkg/constraint_${idx}_power_limit_uw" 2>/dev/null || echo 0)
                c_w=$((c_uw / 1000000))
                c_time_us=$(cat "$pkg/constraint_${idx}_time_window_us" 2>/dev/null || echo 0)
                c_time_s=$(awk "BEGIN {printf \"%.2f\", $c_time_us / 1000000}")

                if [ "$c_name" == "long_term" ]; then
                    echo -e "  [PL1] Long-Term  : ${BOLD}${YELLOW}${c_w} W${NC} (${c_uw} uW) | Time Window (Tau): ${c_time_s}s"
                elif [ "$c_name" == "short_term" ]; then
                    echo -e "  [PL2] Short-Term : ${BOLD}${YELLOW}${c_w} W${NC} (${c_uw} uW) | Time Window (Tau): ${c_time_s}s"
                else
                    echo -e "  Constraint $idx ($c_name): ${BOLD}${c_w} W${NC} (${c_uw} uW) | Time Window: ${c_time_s}s"
                fi
            fi
        done

        # Doc thong tin max power neu co
        if [ -f "$pkg/constraint_0_max_power_uw" ]; then
            local max_uw max_w
            max_uw=$(cat "$pkg/constraint_0_max_power_uw")
            max_w=$((max_uw / 1000000))
            echo -e "  Gioi han toi da  : ${max_w} W (${max_uw} uW)"
        fi
        echo ""
    done

    # Thong tin Intel P-State & Turbo Boost
    echo -e "${BOLD}--- [Intel P-State & Turbo Boost] ---${NC}"
    if [ -d "/sys/devices/system/cpu/intel_pstate" ]; then
        local max_perf no_turbo
        max_perf=$(cat /sys/devices/system/cpu/intel_pstate/max_perf_pct 2>/dev/null || echo "N/A")
        no_turbo=$(cat /sys/devices/system/cpu/intel_pstate/no_turbo 2>/dev/null || echo "N/A")
        echo -e "  Max Performance Pct: ${BOLD}${max_perf}%${NC}"
        echo -e "  Turbo Boost Status : $( [ "$no_turbo" == "0" ] && echo -e "${GREEN}BAT (Enabled - no_turbo=0)${NC}" || echo -e "${RED}TAT (Disabled - no_turbo=1)${NC}" )"
    fi

    # Thong tin Thermal Cooling Devices
    local unclamp_count=0
    local total_cpu_cooling=0
    for cd in /sys/class/thermal/cooling_device*; do
        if [ -f "$cd/type" ]; then
            local t
            t=$(cat "$cd/type")
            if [ "$t" == "Processor" ] || [ "$t" == "intel_powerclamp" ]; then
                total_cpu_cooling=$((total_cpu_cooling + 1))
                local cs
                cs=$(cat "$cd/cur_state" 2>/dev/null || echo 0)
                if [ "$cs" -gt 0 ]; then
                    unclamp_count=$((unclamp_count + 1))
                fi
            fi
        fi
    done
    echo -e "  Thermal Throttling : $( [ "$unclamp_count" -eq 0 ] && echo -e "${GREEN}KHONG CO THROTTLING (Toan bo $total_cpu_cooling cooling devices cur_state=0)${NC}" || echo -e "${YELLOW}CO $unclamp_count/$total_cpu_cooling THIET BI DANG BI THROTTLE${NC}" )"
    echo ""
    echo -e "${BLUE}================================================================================${NC}"
}

# ------------------------------------------------------------------------------
# 5. THIET LAP GIOI HAN CONG SUAT (SET POWER LIMIT)
# ------------------------------------------------------------------------------
set_power_limit() {
    local pl1_w="$1"
    local pl2_w="$2"

    if [ -z "$pl1_w" ] || ! [[ "$pl1_w" =~ ^[0-9]+$ ]]; then
        echo -e "${RED}[ERROR] Gia tri PL1 phai la so nguyen duong (Watts)! Vi du: 150${NC}"
        return 1
    fi

    # Neu khong truyen PL2, mac dinh PL2 = PL1 * 1.25 (hoac bang PL1)
    if [ -z "$pl2_w" ]; then
        pl2_w=$((pl1_w * 125 / 100))
    fi

    if ! [[ "$pl2_w" =~ ^[0-9]+$ ]]; then
        echo -e "${RED}[ERROR] Gia tri PL2 phai la so nguyen duong (Watts)! Vi du: 200${NC}"
        return 1
    fi

    local pl1_uw=$((pl1_w * 1000000))
    local pl2_uw=$((pl2_w * 1000000))

    log_msg "${BLUE}[*] Tien hanh thiet lap Power Limit:${NC}"
    log_msg "    - PL1 (Long-Term TDP) : ${BOLD}${GREEN}${pl1_w} W${NC} (${pl1_uw} uW)"
    log_msg "    - PL2 (Short-Term/Turbo): ${BOLD}${GREEN}${pl2_w} W${NC} (${pl2_uw} uW)"

    local pkgs
    read -r -a pkgs <<< "$(find_packages)"

    if [ ${#pkgs[@]} -eq 0 ]; then
        log_msg "${RED}[ERROR] Khong tim thay package CPU nao de thiet lap!${NC}"
        return 1
    fi

    for pkg in "${pkgs[@]}"; do
        local pkg_name
        pkg_name=$(cat "$pkg/name")
        log_msg "[*] Ap dung cho $pkg_name..."

        # Kich hoat RAPL tren socket
        echo 1 > "$pkg/enabled" || true

        for idx in 0 1; do
            if [ -f "$pkg/constraint_${idx}_name" ]; then
                local c_name
                c_name=$(cat "$pkg/constraint_${idx}_name")
                if [ "$c_name" == "long_term" ]; then
                    if [ -w "$pkg/constraint_${idx}_power_limit_uw" ]; then
                        echo "$pl1_uw" > "$pkg/constraint_${idx}_power_limit_uw"
                    fi
                elif [ "$c_name" == "short_term" ]; then
                    if [ -w "$pkg/constraint_${idx}_power_limit_uw" ]; then
                        echo "$pl2_uw" > "$pkg/constraint_${idx}_power_limit_uw"
                    fi
                fi
            fi
        done
    done

    # 1. Thiet lap Intel P-State max performance 100%
    if [ -f "/sys/devices/system/cpu/intel_pstate/max_perf_pct" ]; then
        echo 100 > /sys/devices/system/cpu/intel_pstate/max_perf_pct
        log_msg "    [P-State] Da dat max_perf_pct = 100%"
    fi

    # 2. Bat Intel Turbo Boost (no_turbo = 0)
    if [ -f "/sys/devices/system/cpu/intel_pstate/no_turbo" ]; then
        echo 0 > /sys/devices/system/cpu/intel_pstate/no_turbo
        log_msg "    [Turbo]   Da bat Turbo Boost (no_turbo = 0)"
    fi

    # 3. Go bo toan bo Thermal Throttling tren tat ca cooling devices cua CPU
    local unclamp_count=0
    for cd in /sys/class/thermal/cooling_device*; do
        if [ -f "$cd/type" ] && [ -w "$cd/cur_state" ]; then
            local t
            t=$(cat "$cd/type")
            if [ "$t" == "Processor" ] || [ "$t" == "intel_powerclamp" ]; then
                echo 0 > "$cd/cur_state" 2>/dev/null || true
                unclamp_count=$((unclamp_count + 1))
            fi
        fi
    done
    log_msg "    [Thermal] Da reset cur_state = 0 tren $unclamp_count cooling devices (unclamp CPU throttle)"

    echo ""
    log_msg "${GREEN}[OK] Thiet lap thanh cong Power Limit, P-State va Thermal Clamp cho tat ca CPU!${NC}"
    echo ""
    show_status
}

# ------------------------------------------------------------------------------
# 6. CAI DAT SYSTEMD SERVICE DE DUY TRI SAU REBOOT (PERSISTENCE)
# ------------------------------------------------------------------------------
install_persistence_service() {
    local pl1_w="$1"
    local pl2_w="$2"

    if [ -z "$pl1_w" ] || ! [[ "$pl1_w" =~ ^[0-9]+$ ]]; then
        echo -e "${RED}[ERROR] Can chi dinh gia tri PL1 (Watts) de cai dat service! Vi du: ./cpu.sh persist 180 220${NC}"
        return 1
    fi

    if [ -z "$pl2_w" ]; then
        pl2_w=$((pl1_w * 125 / 100))
    fi

    local service_path="/etc/systemd/system/cpu-rapl-limit.service"
    log_msg "[*] Dang tao systemd service tai $service_path..."

    cat <<EOF > "$service_path"
[Unit]
Description=Intel CPU RAPL Power Limit Setter
After=multi-user.target

[Service]
Type=oneshot
ExecStart=$SCRIPT_DIR/cpu.sh set $pl1_w $pl2_w
RemainAfterExit=yes

[Install]
WantedBy=multi-user.target
EOF

    systemctl daemon-reload
    systemctl enable cpu-rapl-limit.service
    log_msg "${GREEN}[OK] Da kich hoat dich vu cpu-rapl-limit.service!${NC}"
    log_msg "    Moi khi khoi dong lai may, Linux se tu dong dat PL1=${pl1_w}W va PL2=${pl2_w}W."
}

remove_persistence_service() {
    local service_path="/etc/systemd/system/cpu-rapl-limit.service"
    if [ -f "$service_path" ]; then
        systemctl disable cpu-rapl-limit.service 2>/dev/null || true
        rm -f "$service_path"
        systemctl daemon-reload
        log_msg "${GREEN}[OK] Da go bo dich vu cpu-rapl-limit.service.${NC}"
    else
        log_msg "[i] Dich vu cpu-rapl-limit.service chua duoc cai dat."
    fi
}

# ------------------------------------------------------------------------------
# 7. MENU TUONG TAC KHI CHAY KHONG THAM SO
# ------------------------------------------------------------------------------
interactive_menu() {
    clear || true
    show_status

    echo -e "${BOLD}${CYAN}MENU THIET LAP NHANH CONG SUAT CPU (INTEL RAPL):${NC}"
    echo "  1) Che do Tiet kiem dien / Mat may (Eco Mode: PL1=125W, PL2=150W)"
    echo "  2) Che do Can bang thong dung      (Balanced: PL1=180W, PL2=220W)"
    echo "  3) Che do May tram Xeon W5-3435X   (Workstation TDP: PL1=270W, PL2=338W)"
    echo "  4) Che do Hieu nang cuc dai (Max)  (Uncapped: PL1=350W, PL2=400W)"
    echo "  5) Nhap gia tri Watts tuy chinh theo y muon (Custom PL1 & PL2)"
    echo "  6) Cai dat service tu dong duy tri muc Watt khi Reboot"
    echo "  7) Go bo service khoi dong cung he thong"
    echo "  8) Lam moi / Xem lai thong so hien tai"
    echo "  0) Thoat"
    echo ""
    read -rp "Vui long chon [0-8]: " choice

    case "$choice" in
        1)
            set_power_limit 125 150
            ;;
        2)
            set_power_limit 180 220
            ;;
        3)
            set_power_limit 270 338
            ;;
        4)
            set_power_limit 350 400
            ;;
        5)
            echo ""
            read -rp "Nhap cong suat dai han PL1 (Watts, vd: 200): " custom_pl1
            read -rp "Nhap cong suat ngan han PL2 (Watts, vd: 250 - Enter de mac dinh): " custom_pl2
            set_power_limit "$custom_pl1" "$custom_pl2"
            ;;
        6)
            echo ""
            read -rp "Nhap PL1 muon luu khi khoi dong (Watts): " s_pl1
            read -rp "Nhap PL2 muon luu khi khoi dong (Watts): " s_pl2
            install_persistence_service "$s_pl1" "$s_pl2"
            ;;
        7)
            remove_persistence_service
            ;;
        8)
            show_status
            ;;
        0)
            echo "Tam biet!"
            exit 0
            ;;
        *)
            echo -e "${RED}[!] Lua chon khong hop le.${NC}"
            ;;
    esac
}

# ------------------------------------------------------------------------------
# 8. PHAN TICH THAM SO DONG LENH (CLI PARSER)
# ------------------------------------------------------------------------------
case "$1" in
    status|info|show|"")
        if [ -z "$1" ] && [ -t 0 ]; then
            interactive_menu
        else
            show_status
        fi
        ;;
    set)
        shift
        set_power_limit "$1" "$2"
        ;;
    persist)
        shift
        install_persistence_service "$1" "$2"
        ;;
    unpersist)
        remove_persistence_service
        ;;
    help|--help|-h)
        echo "Cach su dung script $0:"
        echo "  $0                          : Mo Menu tuong tac de xem va chon profile Watt"
        echo "  $0 status                   : Hien thi thong so gioi han cong suat & muc tieu thu tuc thoi"
        echo "  $0 set <PL1_W> [PL2_W]      : Dat PL1 va PL2 (Watts). Vi du: $0 set 180 220"
        echo "  $0 persist <PL1_W> [PL2_W]  : Tao systemd service de tu dong dat lai moi khi reboot"
        echo "  $0 unpersist                : Go bo systemd service khoi dong cung he thong"
        ;;
    *)
        if [[ "$1" =~ ^[0-9]+$ ]]; then
            set_power_limit "$1" "$2"
        else
            echo -e "${RED}[!] Tham so khong hop le: $1${NC}"
            echo "Chay '$0 help' de xem huong dan."
            exit 1
        fi
        ;;
esac

