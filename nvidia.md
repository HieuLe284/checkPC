# HUONG DAN SU DUNG & CAC LENH BASH CHO NVIDIA (UBUNTU)

Tài liệu tổng hợp toàn bộ các lệnh Bash cần dùng để cài đặt, cấu hình, kiểm tra và giám sát GPU NVIDIA (tối ưu hóa cho hệ thống chạy **2x NVIDIA RTX 4000 Ada Generation - 20GB GDDR6 ECC**).

---

## 1. Cài đặt toàn bộ Driver & CUDA Stack (Full)

Cấp quyền thực thi và chạy script cài đặt trọn bộ:
```bash
# 1. Cap quyen thuc thi cho script
chmod +x nvidia.sh

# 2. Chay cai dat toan bo voi quyen sudo
sudo ./nvidia.sh
```

Sau khi cài đặt xong, nếu là lần đầu cài đặt driver hoặc vừa gỡ Nouveau, hãy khởi động lại máy để Kernel nạp đầy đủ driver:
```bash
sudo reboot
```

---

## 2. Kiểm tra & Giám sát GPU thời gian thực (Monitoring)

### Xem bảng điều khiển tổng quan GPU:
```bash
nvidia-smi
```
*Hiển thị: Tên GPU, Driver Version, CUDA Version tối đa, VRAM đã dùng/còn trống, Nhiệt độ và Công suất (W).*

### Theo dõi GPU liên tục thời gian thực (cập nhật mỗi 1 giây):
```bash
watch -n 1 nvidia-smi
```
*(Bấm `Ctrl + C` để thoát)*

### Xem chi tiết cảm biến, VRAM, điện áp và xung nhịp:
```bash
nvidia-smi -q -d MEMORY,TEMPERATURE,POWER,CLOCK
```

### Giám sát mức tải GPU và công suất tiêu thụ dạng bảng log:
```bash
nvidia-smi --query-gpu=index,name,utilization.gpu,utilization.memory,temperature.gpu,power.draw --format=csv -l 1
```

### Xem danh sách tiến trình đang ngốn bộ nhớ VRAM:
```bash
nvidia-smi pmon -s u
```

---

## 3. Kiểm tra trình biên dịch CUDA & Môi trường AI

### Kiểm tra trình biên dịch CUDA Compiler (`nvcc`):
```bash
nvcc --version
```
*Nếu báo không tìm thấy lệnh, hãy nạp lại biến môi trường:*
```bash
source /etc/profile.d/cuda.sh
```

### Kiểm tra đường dẫn CUDA:
```bash
echo "CUDA_HOME: $CUDA_HOME"
which nvcc
```

### Kiểm tra các gói cuDNN đã cài đặt:
```bash
dpkg -l | grep -i cudnn
```

### Kiểm tra thư viện TensorRT:
```bash
dpkg -l | grep -i tensorrt
```

### Test tính toán GPU thực tế với Python (PyTorch):
```bash
python3 -c "import torch; print('CUDA Available:', torch.cuda.is_available()); print('Device Name:', torch.cuda.get_device_name(0)); print('Device Count:', torch.cuda.device_count())"
```

---

## 4. Cấu hình tối ưu Multi-GPU cho 2x RTX 4000 Ada

### Bật Persistence Mode cho tất cả GPU (Giảm độ trễ nạp driver):
```bash
sudo nvidia-smi -pm 1
```

### Bật chế độ kiểm tra lỗi bộ nhớ GDDR6 ECC:
```bash
sudo nvidia-smi --ecc-config=1
```

### Xem ma trận kết nối Peer-to-Peer (P2P / PCIe Topology) giữa 2 GPU:
```bash
nvidia-smi topo -m
```

---

## 5. Kiểm tra NVIDIA Container Toolkit (Docker GPU Passthrough)

### Cấu hình runtime Docker (nếu có Docker):
```bash
sudo nvidia-ctk runtime configure --runtime=docker
sudo systemctl restart docker
```

### Test chạy container kiểm tra GPU qua Docker:
```bash
sudo docker run --rm --gpus all nvidia/cuda:12.6.0-base-ubuntu24.04 nvidia-smi
```

---

## 6. Xử lý sự cố & Xem nhật ký (Troubleshooting)

### Xem file log của quá trình cài đặt:
```bash
cat build/nvidia_install.log
```

### Kiểm tra module Kernel NVIDIA đã được nạp hay chưa:
```bash
lsmod | grep -i nvidia
```

### Xem thông điệp Kernel liên quan đến NVIDIA:
```bash
sudo dmesg | grep -i nvidia
```

---

## 7. Sử dụng trên Windows (nvidia.bat)

### Mở Menu tương tác quản lý GPU trên Windows:
```cmd
nvidia.bat
```
*(Nếu chưa chạy bằng quyền Administrator, script sẽ tự động yêu cầu cấp quyền UAC)*

### Xem nhanh thông tin và trạng thái GPU (CLI):
```cmd
nvidia.bat status
```

### Giám sát GPU thời gian thực liên tục (1s):
```cmd
nvidia.bat monitor
```

### Kích hoạt Persistence Mode & ECC Memory cho 2x RTX 4000 Ada:
```cmd
nvidia.bat opt
```

### Kiểm tra trình biên dịch CUDA Compiler (nvcc) trên Windows:
```cmd
nvidia.bat cuda
```

### Tự động chạy Persistence Mode mỗi khi khởi động Windows (Task Scheduler):
```cmd
:: Tao tac vu tu dong khoi dong cung Windows:
nvidia.bat persist

:: Go bo tac vu khoi dong:
nvidia.bat unpersist
```


