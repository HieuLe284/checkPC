# HUONG DAN SU DUNG & CAC LENH BASH CHO CPU POWER LIMIT (INTEL RAPL)

Tài liệu tổng hợp toàn bộ các lệnh Bash cần dùng để kiểm tra, giới hạn công suất tiêu thụ điện (Power Limit), mở khóa P-State và gỡ bỏ Thermal Throttling cho CPU Intel trên Linux thông qua giao tiếp **Intel RAPL**.

---

## 1. Sử dụng Script cpu.sh (Khuyên dùng)

### Mở Menu tương tác trực quan (Đầy đủ chức năng):
```bash
sudo ./cpu.sh
```
*Sẽ hiển thị công suất thực tế đang dùng, thông số PL1/PL2 hiện tại và menu chọn nhanh các profile.*

### Xem trạng thái công suất & công suất tức thời hiện tại (Status / Info):
```bash
sudo ./cpu.sh status
```

### Theo dõi công suất CPU liên tục thời gian thực (cập nhật mỗi 1 giây):
```bash
watch -n 1 sudo ./cpu.sh status
```
*(Bấm `Ctrl + C` để thoát)*

---

## 2. Thiết lập giới hạn công suất nhanh bằng dòng lệnh (CLI)

### Chế độ Tiết kiệm điện / Mát máy (Eco Mode: PL1=125W, PL2=150W):
```bash
sudo ./cpu.sh set 125 150
```

### Chế độ Cân bằng thông dụng (Balanced Mode: PL1=180W, PL2=220W):
```bash
sudo ./cpu.sh set 180 220
```

### Chế độ Chuẩn máy trạm Xeon W5-3435X (Workstation TDP: PL1=270W, PL2=338W):
```bash
sudo ./cpu.sh set 270 338
```

### Chế độ Bung tối đa hiệu năng (Uncapped Mode: PL1=350W, PL2=400W):
```bash
sudo ./cpu.sh set 350 400
```

### Đặt mức công suất Watt tùy ý (ví dụ: PL1=100W):
```bash
sudo ./cpu.sh set 100
```

---

## 3. Cài đặt tự động duy trì mức Watt khi khởi động lại máy (Persistence)

Do Linux sysfs sẽ reset về mặc định BIOS khi reboot, bạn có thể tạo systemd service để máy tự động nạp lại mức công suất mong muốn:

### Lưu cấu hình tự chạy khi Boot máy (ví dụ: PL1=180W, PL2=220W):
```bash
sudo ./cpu.sh persist 180 220
```

### Kiểm tra trạng thái dịch vụ tự động:
```bash
systemctl status cpu-rapl-limit.service
```

### Gỡ bỏ dịch vụ khởi động cùng hệ thống:
```bash
sudo ./cpu.sh unpersist
```

---

## 4. Các lệnh Linux Sysfs thủ công cốt lõi (Tham chiếu nhanh)

Bộ 4 câu lệnh can thiệp trực tiếp vào nhân Linux không cần công cụ ngoài:

### 1. Đặt giới hạn công suất PL1 = 100W (100,000,000 µW):
```bash
echo 100000000 | sudo tee /sys/class/powercap/intel-rapl:0/constraint_0_power_limit_uw
```

### 2. Mở khóa 100% hiệu năng xung nhịp Intel P-State:
```bash
echo 100 | sudo tee /sys/devices/system/cpu/intel_pstate/max_perf_pct
```

### 3. Bật Intel Turbo Boost (no_turbo = 0):
```bash
echo 0 | sudo tee /sys/devices/system/cpu/intel_pstate/no_turbo
```

### 4. Gỡ bỏ trạng thái bóp xung nhiệt độ (Thermal Throttling Clamp) của toàn bộ core:
```bash
# Reset ve 0 cho tat ca cooling devices cua Processor
for cd in /sys/class/thermal/cooling_device*; do
    if [ -f "$cd/type" ] && [ "$(cat "$cd/type")" = "Processor" ]; then
        echo 0 | sudo tee "$cd/cur_state" > /dev/null
    fi
done
```

---

## 5. Giám sát xung nhịp & nhiệt độ CPU khi tải (Verification)

### Theo dõi xung nhịp (MHz) của tất cả các nhân CPU liên tục:
```bash
watch -n 1 "grep 'cpu MHz' /proc/cpuinfo"
```

### Theo dõi nhiệt độ các nhân CPU (cần gói lm-sensors):
```bash
watch -n 1 sensors
```

### Xem nhật ký thay đổi công suất CPU đã thực hiện:
```bash
cat build/cpu_rapl.log
```


### Kiểm tra trạng thái dịch vụ tự động:
```bash
systemctl status cpu-rapl-limit.service
```

### Gỡ bỏ dịch vụ khởi động cùng hệ thống:
```bash
sudo ./cpu.sh unpersist
```

---

## 4. Các lệnh Linux Sysfs thủ công cốt lõi (Tham chiếu nhanh)

Bộ 4 câu lệnh can thiệp trực tiếp vào nhân Linux không cần công cụ ngoài:

### 1. Đặt giới hạn công suất PL1 = 100W (100,000,000 µW):
```bash
echo 100000000 | sudo tee /sys/class/powercap/intel-rapl:0/constraint_0_power_limit_uw
```

### 2. Mở khóa 100% hiệu năng xung nhịp Intel P-State:
```bash
echo 100 | sudo tee /sys/devices/system/cpu/intel_pstate/max_perf_pct
```

### 3. Bật Intel Turbo Boost (no_turbo = 0):
```bash
echo 0 | sudo tee /sys/devices/system/cpu/intel_pstate/no_turbo
```

### 4. Gỡ bỏ trạng thái bóp xung nhiệt độ (Thermal Throttling Clamp) của toàn bộ core:
```bash
# Reset ve 0 cho tat ca cooling devices cua Processor
for cd in /sys/class/thermal/cooling_device*; do
    if [ -f "$cd/type" ] && [ "$(cat "$cd/type")" = "Processor" ]; then
        echo 0 | sudo tee "$cd/cur_state" > /dev/null
    fi
done
```

---

## 5. Giám sát xung nhịp & nhiệt độ CPU khi tải (Verification)

### Theo dõi xung nhịp (MHz) của tất cả các nhân CPU liên tục:
```bash
watch -n 1 "grep 'cpu MHz' /proc/cpuinfo"
```

### Theo dõi nhiệt độ các nhân CPU (cần gói lm-sensors):
```bash
watch -n 1 sensors
```

### Xem nhật ký thay đổi công suất CPU đã thực hiện:
```bash
cat build/cpu_rapl.log
```