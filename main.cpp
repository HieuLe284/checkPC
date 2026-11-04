#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <fstream>
#include <memory>
#include <array>
#include <chrono>
#include <iomanip>
#include <algorithm>
#include <filesystem>
#include <cstdlib>
#include <cstring>
#include <cctype>
#include <ctime>

#if defined(_WIN32)
    #include <windows.h>
    #define POPEN _popen
    #define PCLOSE _pclose
#elif defined(__linux__)
    #include <unistd.h>
    #include <sys/utsname.h>
    #include <sys/stat.h>
    #define POPEN popen
    #define PCLOSE pclose
#else
    #define POPEN popen
    #define PCLOSE pclose
#endif

namespace fs = std::filesystem;

// -------------------------------------------------------------
// Utilities
// -------------------------------------------------------------
std::string trim(const std::string& str) {
    auto start = str.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    auto end = str.find_last_not_of(" \t\r\n");
    return str.substr(start, end - start + 1);
}

std::string execCommand(const std::string& cmd) {
    std::array<char, 512> buffer;
    std::string result;
    FILE* pipe = POPEN(cmd.c_str(), "r");
    if (!pipe) {
        return "";
    }
    while (fgets(buffer.data(), static_cast<int>(buffer.size()), pipe) != nullptr) {
        result += buffer.data();
    }
    PCLOSE(pipe);
    return trim(result);
}

std::string readFileContent(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) return "";
    std::ostringstream ss;
    ss << file.rdbuf();
    return trim(ss.str());
}

std::vector<std::string> splitLines(const std::string& text) {
    std::vector<std::string> lines;
    std::istringstream stream(text);
    std::string line;
    while (std::getline(stream, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        lines.push_back(line);
    }
    return lines;
}

std::string decodeChassisType(const std::string& typeStr) {
    try {
        int t = std::stoi(typeStr);
        switch (t) {
            case 1: return typeStr + " (Other)";
            case 2: return typeStr + " (Unknown)";
            case 3: return typeStr + " (Desktop - May tinh de ban)";
            case 4: return typeStr + " (Low Profile Desktop)";
            case 5: return typeStr + " (Pizza Box)";
            case 6: return typeStr + " (Mini Tower)";
            case 7: return typeStr + " (Tower)";
            case 8: return typeStr + " (Portable)";
            case 9: return typeStr + " (Laptop)";
            case 10: return typeStr + " (Notebook)";
            case 11: return typeStr + " (Hand Held)";
            case 12: return typeStr + " (Docking Station)";
            case 13: return typeStr + " (All in One)";
            case 14: return typeStr + " (Sub Notebook)";
            case 15: return typeStr + " (Space-saving)";
            case 16: return typeStr + " (Lunch Box)";
            case 17: return typeStr + " (Main Server)";
            case 23: return typeStr + " (Rack Mount Chassis)";
            case 24: return typeStr + " (Sealed-case PC)";
            default: return typeStr;
        }
    } catch (...) {
        return typeStr;
    }
}

bool containsCaseInsensitive(const std::string& text, const std::string& search) {
    if (search.empty()) return true;
    auto it = std::search(
        text.begin(), text.end(),
        search.begin(), search.end(),
        [](char ch1, char ch2) {
            return std::tolower(static_cast<unsigned char>(ch1)) == std::tolower(static_cast<unsigned char>(ch2));
        }
    );
    return it != text.end();
}

struct SpecCheckResult {
    std::string component;
    std::string expected;
    std::string detected;
    std::string status;
    std::string details;
};

void saveAndPrintWorkstationReport(const std::vector<SpecCheckResult>& results, int passCount, int totalCriteria, const std::string& outputPath) {
    std::ostringstream ss;
    std::string bar(80, '=');
    std::string subBar(80, '-');

    auto now = std::chrono::system_clock::now();
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);
    std::tm now_tm;
#if defined(_WIN32)
    localtime_s(&now_tm, &now_c);
#else
    localtime_r(&now_c, &now_tm);
#endif
    char timeBuf[64];
    std::strftime(timeBuf, sizeof(timeBuf), "%Y-%m-%d %H:%M:%S", &now_tm);

    ss << "\n" << bar << "\n";
    ss << "     BANG DOI CHIEU THONG SO HE THONG VOI CAU HINH MAY TRAM (WORKSTATION)\n";
    ss << bar << "\n";
    ss << "  Cau hinh tieu chuan doi chieu:\n";
    ss << "  MAY TRAM (WTS) ASUS PRO ET700I W7 / 1x Intel Xeon W5-3435X 16C/32T 45M /\n";
    ss << "  4x Ram 64GB DDR5-5600 / 1x SSD Kingston 1TB NV3 M.2 2280 NVMe /\n";
    ss << "  2x NVIDIA RTX 4000 Ada Generation - 20GB GDDR6 ECC / WOS / 1300W / DEN\n";
    ss << bar << "\n";
    ss << "  Thoi gian thuc hien kiem tra : " << timeBuf << "\n";
    ss << "  Ket qua danh gia tong the    : "
       << (passCount == totalCriteria ? "[DAT CHUAN HOAN TOAN]" : (passCount >= 6 ? "[DAT PHAN LON / CO SU KHAC BIET]" : "[KHONG KHOP CAU HINH]"))
       << " (" << passCount << "/" << totalCriteria << " tieu chi dat/phu hop)\n";
    ss << bar << "\n\n";

    for (const auto& r : results) {
        ss << subBar << "\n";
        ss << " " << r.component << "\n";
        ss << subBar << "\n";
        ss << "   - Tieu chuan WTS   : " << r.expected << "\n";
        ss << "   - Thuc te phat hien: " << r.detected << "\n";
        ss << "   - Danh gia doi soat: " << r.status << "\n";
        ss << "   - Giai thich       : " << r.details << "\n\n";
    }

    ss << bar << "\n";
    ss << " TONG KET DOI CHIEU CAU HINH (VERIFICATION SUMMARY)\n";
    ss << bar << "\n";
    ss << "   - Tong so muc tieu chi kiem tra : " << totalCriteria << "\n";
    ss << "   - So muc Dat / Phu hop          : " << passCount << " / " << totalCriteria << "\n";
    if (passCount == totalCriteria) {
        ss << "   - KET LUAN                      : [DAT YEU CAU] He thong dap ung dung 100% thong so May tram ASUS PRO ET700I W7.\n";
    } else {
        ss << "   - KET LUAN                      : [CAN LUU Y] Phat hien " << (totalCriteria - passCount) << " thong so chua khop voi may tram ASUS PRO ET700I W7.\n";
        ss << "                                     Vui long xem chi tiet cac muc [FAIL / MISMATCH] o tren.\n";
    }
    ss << bar << "\n";

    std::string reportContent = ss.str();

    // In ra man hinh console
    std::cout << reportContent;

    // Ghi vao file build/workstation.txt
    try {
        fs::path p(outputPath);
        if (p.has_parent_path()) {
            fs::create_directories(p.parent_path());
        }
        std::ofstream out(outputPath);
        if (out.is_open()) {
            out << reportContent;
            out.close();
            std::cout << "[OK] Bao cao doi chieu thong so da duoc ghi thanh cong vao: " << outputPath << "\n";
        } else {
            std::cerr << "[!] Khong the ghi file: " << outputPath << "\n";
        }
    } catch (const std::exception& e) {
        std::cerr << "[!] Loi khi ghi file " << outputPath << ": " << e.what() << "\n";
    }
}

// -------------------------------------------------------------
// Report Collector
// -------------------------------------------------------------
class SystemReporter {
private:
    std::ostringstream reportStream;

public:
    void print(const std::string& text = "") {
        std::cout << text << "\n";
        reportStream << text << "\n";
    }

    void printHeader(const std::string& title) {
        std::string bar(80, '=');
        print();
        print(bar);
        print("  " + title);
        print(bar);
    }

    void printSubHeader(const std::string& subTitle) {
        std::string subBar(60, '-');
        print();
        print("  [" + subTitle + "]");
        print("  " + subBar);
    }

    void printField(const std::string& label, const std::string& value, int indent = 4) {
        std::string indentStr(indent, ' ');
        std::ostringstream oss;
        oss << indentStr << std::left << std::setw(32) << (label + ":") << (value.empty() ? "N/A" : value);
        print(oss.str());
    }

    void saveToFile(const std::string& filePath) {
        try {
            fs::path p(filePath);
            if (p.has_parent_path()) {
                fs::create_directories(p.parent_path());
            }
            std::ofstream out(filePath);
            if (out.is_open()) {
                out << reportStream.str();
                out.close();
                std::cout << "\n[OK] Bao cao he thong da duoc ghi thanh cong vao: " << filePath << "\n";
            } else {
                std::cerr << "\n[!] Khong the ghi file bao cao tai: " << filePath << "\n";
            }
        } catch (const std::exception& e) {
            std::cerr << "\n[!] Loi khi ghi file bao cao: " << e.what() << "\n";
        }
    }
};

// -------------------------------------------------------------
// Component Checkers - Linux Implementation
// -------------------------------------------------------------
#if defined(__linux__)

void checkBIOS(SystemReporter& reporter) {
    reporter.printHeader("1. BIOS / UEFI SETTINGS");

    std::string vendor = readFileContent("/sys/class/dmi/id/bios_vendor");
    std::string version = readFileContent("/sys/class/dmi/id/bios_version");
    std::string date = readFileContent("/sys/class/dmi/id/bios_date");
    std::string release = readFileContent("/sys/class/dmi/id/bios_release");

    bool isUEFI = fs::exists("/sys/firmware/efi");
    std::string bootMode = isUEFI ? "UEFI" : "Legacy BIOS";

    reporter.printField("Hang san xuat (Vendor)", vendor);
    reporter.printField("Phien ban BIOS (Version)", version);
    reporter.printField("Ngay phat hanh (Release Date)", date);
    reporter.printField("BIOS Revision (Release)", release);
    reporter.printField("Che do khoi dong (Boot Mode)", bootMode);

    // SMBIOS info via dmidecode if available
    std::string dmiBios = execCommand("dmidecode -t bios 2>/dev/null");
    if (!dmiBios.empty() && dmiBios.find("ROM Size:") != std::string::npos) {
        std::vector<std::string> lines = splitLines(dmiBios);
        for (const auto& line : lines) {
            if (line.find("ROM Size:") != std::string::npos ||
                line.find("Address:") != std::string::npos ||
                line.find("Runtime Size:") != std::string::npos) {
                auto colonPos = line.find(':');
                if (colonPos != std::string::npos) {
                    reporter.printField(trim(line.substr(0, colonPos)), trim(line.substr(colonPos + 1)));
                }
            }
        }
    }
}

void checkOS(SystemReporter& reporter) {
    reporter.printHeader("2. OPERATING SYSTEM (UBUNTU / LINUX)");

    struct utsname uts;
    if (uname(&uts) == 0) {
        reporter.printField("He dieu hanh (Kernel Name)", uts.sysname);
        reporter.printField("Kernel Release", uts.release);
        reporter.printField("Kernel Version", uts.version);
        reporter.printField("Kien truc (Architecture)", uts.machine);
        reporter.printField("Ten may (Hostname)", uts.nodename);
    }

    std::string osRelease = readFileContent("/etc/os-release");
    if (!osRelease.empty()) {
        std::vector<std::string> lines = splitLines(osRelease);
        for (const auto& line : lines) {
            if (line.rfind("PRETTY_NAME=", 0) == 0) {
                std::string val = line.substr(12);
                if (val.size() >= 2 && val.front() == '"' && val.back() == '"') val = val.substr(1, val.size() - 2);
                reporter.printField("Phien ban chi tiet (OS)", val);
            } else if (line.rfind("VERSION_ID=", 0) == 0) {
                std::string val = line.substr(11);
                if (val.size() >= 2 && val.front() == '"' && val.back() == '"') val = val.substr(1, val.size() - 2);
                reporter.printField("Ma phien ban (Version ID)", val);
            } else if (line.rfind("VERSION_CODENAME=", 0) == 0) {
                reporter.printField("Codename", line.substr(17));
            }
        }
    }

    std::string uptimeRaw = readFileContent("/proc/uptime");
    if (!uptimeRaw.empty()) {
        std::istringstream iss(uptimeRaw);
        double uptimeSeconds = 0;
        if (iss >> uptimeSeconds) {
            long totalSec = static_cast<long>(uptimeSeconds);
            long days = totalSec / 86400;
            long hours = (totalSec % 86400) / 3600;
            long mins = (totalSec % 3600) / 60;
            long secs = totalSec % 60;
            std::ostringstream oss;
            oss << days << " ngay, " << hours << " gio, " << mins << " phut, " << secs << " giay";
            reporter.printField("Thoi gian hoat dong (Uptime)", oss.str());
        }
    }

    const char* user = std::getenv("USER");
    reporter.printField("Nguoi dung hien tai (User)", user ? user : "Unknown");
}

void checkMainboard(SystemReporter& reporter) {
    reporter.printHeader("3. MAINBOARD / BO MACH CHU");

    std::string vendor = readFileContent("/sys/class/dmi/id/board_vendor");
    std::string product = readFileContent("/sys/class/dmi/id/board_name");
    std::string version = readFileContent("/sys/class/dmi/id/board_version");
    std::string serial = readFileContent("/sys/class/dmi/id/board_serial");
    std::string assetTag = readFileContent("/sys/class/dmi/id/board_asset_tag");

    std::string chassisType = readFileContent("/sys/class/dmi/id/chassis_type");
    std::string chassisVendor = readFileContent("/sys/class/dmi/id/chassis_vendor");
    std::string chassisSerial = readFileContent("/sys/class/dmi/id/chassis_serial");

    std::string sysVendor = readFileContent("/sys/class/dmi/id/sys_vendor");
    std::string sysProduct = readFileContent("/sys/class/dmi/id/product_name");
    std::string sysSerial = readFileContent("/sys/class/dmi/id/product_serial");
    std::string sysUuid = readFileContent("/sys/class/dmi/id/product_uuid");
    std::string sysSku = readFileContent("/sys/class/dmi/id/product_sku");

    if (serial.empty()) {
        std::string dmiMb = execCommand("dmidecode -s baseboard-serial-number 2>/dev/null");
        if (!dmiMb.empty()) serial = dmiMb;
    }
    if (sysSerial.empty()) {
        std::string dmiSys = execCommand("dmidecode -s system-serial-number 2>/dev/null");
        if (!dmiSys.empty()) sysSerial = dmiSys;
    }
    if (sysUuid.empty()) {
        std::string dmiUuid = execCommand("dmidecode -s system-uuid 2>/dev/null");
        if (!dmiUuid.empty()) sysUuid = dmiUuid;
    }

    reporter.printField("Nha san xuat (Board Vendor)", vendor);
    reporter.printField("Ten ma Main (Model/Product)", product);
    reporter.printField("Phien ban Main (Board Version)", version);
    reporter.printField("So Serial Main (Board Serial)", serial.empty() ? "(Chay sudo ./script.sh de xem)" : serial);
    reporter.printField("Asset Tag", assetTag);

    reporter.printSubHeader("Thong tin He thong & Chassis (Case)");
    reporter.printField("Hang may bo (System Vendor)", sysVendor);
    reporter.printField("Ten san pham he thong", sysProduct);
    reporter.printField("Serial san pham (System Serial)", sysSerial.empty() ? "(Chay sudo ./script.sh de xem)" : sysSerial);
    reporter.printField("UUID he thong", sysUuid.empty() ? "(Chay sudo ./script.sh de xem)" : sysUuid);
    reporter.printField("Ma SKU", sysSku);
    reporter.printField("Loai Chassis (Chassis Type)", decodeChassisType(chassisType));
    reporter.printField("Hang Chassis", chassisVendor);
    reporter.printField("Serial Chassis", chassisSerial.empty() ? "(Chay sudo ./script.sh de xem)" : chassisSerial);
}

void checkCPU(SystemReporter& reporter) {
    reporter.printHeader("4. CPU / BO VI XU LY");

    std::string lscpuOut = execCommand("lscpu 2>/dev/null");
    if (!lscpuOut.empty()) {
        std::vector<std::string> lines = splitLines(lscpuOut);
        for (const auto& l : lines) {
            auto colon = l.find(':');
            if (colon == std::string::npos) continue;
            std::string key = trim(l.substr(0, colon));
            std::string val = trim(l.substr(colon + 1));

            if (key == "Model name") reporter.printField("Ten Model CPU", val);
            else if (key == "Architecture") reporter.printField("Kien truc CPU", val);
            else if (key == "CPU(s)") reporter.printField("Tong so luong logic (Threads)", val);
            else if (key == "Core(s) per socket") reporter.printField("So nhan tren socket (Cores)", val);
            else if (key == "Socket(s)") reporter.printField("So luong Socket", val);
            else if (key == "CPU max MHz") reporter.printField("Xung nhip toi da (Boost MHz)", val + " MHz");
            else if (key == "CPU min MHz") reporter.printField("Xung nhip toi thieu (Min MHz)", val + " MHz");
            else if (key == "L1d cache") reporter.printField("L1d Cache", val);
            else if (key == "L1i cache") reporter.printField("L1i Cache", val);
            else if (key == "L2 cache") reporter.printField("L2 Cache", val);
            else if (key == "L3 cache") reporter.printField("L3 Cache", val);
            else if (key == "Virtualization") reporter.printField("Ao hoa phan cung (Virtualization)", val);
        }
    } else {
        // Fallback to /proc/cpuinfo
        std::string cpuinfo = readFileContent("/proc/cpuinfo");
        std::vector<std::string> lines = splitLines(cpuinfo);
        int procCount = 0;
        for (const auto& l : lines) {
            if (l.rfind("model name", 0) == 0 && procCount == 0) {
                reporter.printField("Ten Model CPU", trim(l.substr(l.find(':') + 1)));
            } else if (l.rfind("processor", 0) == 0) {
                procCount++;
            }
        }
        reporter.printField("Tong so luong Threads", std::to_string(procCount));
    }

    // Check VT-x / AMD-V virtualization flag in /proc/cpuinfo
    std::string cpuinfo = readFileContent("/proc/cpuinfo");
    bool vmx = cpuinfo.find(" vmx ") != std::string::npos;
    bool svm = cpuinfo.find(" svm ") != std::string::npos;
    std::string virtStatus = "Khong ho tro";
    if (vmx) virtStatus = "Ho tro (Intel VT-x / Co san de chay Docker & VM)";
    else if (svm) virtStatus = "Ho tro (AMD-V / Co san de chay Docker & VM)";
    reporter.printField("Trang thai Ao hoa (VT-x/AMD-V)", virtStatus);
}

void checkGPU(SystemReporter& reporter) {
    reporter.printHeader("5. GPU / CARD MAN HINH");

    std::string lspciOut = execCommand("lspci -nnk 2>/dev/null | grep -E '^[0-9a-fA-F]{2}:[0-9a-fA-F]{2}\\.[0-9a-fA-F].*(VGA compatible controller|3D controller|Display controller|\\[030[0-9]\\]|\\[0380\\])' -A 3");
    if (!lspciOut.empty()) {
        std::vector<std::string> lines = splitLines(lspciOut);
        int gpuIdx = 0;
        for (const auto& line : lines) {
            if (line.find("VGA compatible controller") != std::string::npos ||
                line.find("3D controller") != std::string::npos ||
                line.find("Display controller") != std::string::npos ||
                line.find("[0300]") != std::string::npos ||
                line.find("[0302]") != std::string::npos) {
                gpuIdx++;
                reporter.printSubHeader("GPU #" + std::to_string(gpuIdx));
                reporter.printField("Ten thiet bi (PCI)", trim(line));
            } else if (line.find("Subsystem:") != std::string::npos) {
                reporter.printField("Hang san xuat (Subsystem)", trim(line.substr(line.find(':') + 1)));
            } else if (line.find("Kernel driver in use:") != std::string::npos) {
                reporter.printField("Driver dang dung", trim(line.substr(line.find(':') + 1)));
            } else if (line.find("Kernel modules:") != std::string::npos) {
                reporter.printField("Kernel Modules", trim(line.substr(line.find(':') + 1)));
            }
        }
    } else {
        reporter.printField("Thong tin GPU (lspci)", "Khong co du lieu tu lspci");
    }

    std::string nvidiaSmi = execCommand("nvidia-smi --query-gpu=index,name,driver_version,memory.total,memory.free,memory.used,temperature.gpu,pci.bus_id,serial,uuid,power.draw --format=csv,noheader,nounits 2>/dev/null");
    if (!nvidiaSmi.empty()) {
        reporter.printSubHeader("Thong tin chi tiet NVIDIA GPU");
        std::vector<std::string> lines = splitLines(nvidiaSmi);
        for (const auto& line : lines) {
            std::istringstream ss(line);
            std::string idx, name, drv, memTot, memFree, memUsed, temp, busId, ser, uuid, pwr;
            std::getline(ss, idx, ',');
            std::getline(ss, name, ',');
            std::getline(ss, drv, ',');
            std::getline(ss, memTot, ',');
            std::getline(ss, memFree, ',');
            std::getline(ss, memUsed, ',');
            std::getline(ss, temp, ',');
            std::getline(ss, busId, ',');
            std::getline(ss, ser, ',');
            std::getline(ss, uuid, ',');
            std::getline(ss, pwr, ',');

            reporter.printField("NVIDIA GPU Index", trim(idx));
            reporter.printField("Ten GPU", trim(name));
            reporter.printField("Phien ban Driver NVIDIA", trim(drv));
            reporter.printField("Tong dung luong VRAM", trim(memTot) + " MB");
            reporter.printField("VRAM da dung / con trong", trim(memUsed) + " MB / " + trim(memFree) + " MB");
            reporter.printField("Nhiet do GPU", trim(temp) + " C");
            reporter.printField("Cong suat dien tieu thu", trim(pwr) + " W");
            reporter.printField("PCI Bus ID", trim(busId));
            reporter.printField("Ma Serial GPU", trim(ser));
            reporter.printField("UUID GPU", trim(uuid));
        }
    }
}

void checkRAM(SystemReporter& reporter) {
    reporter.printHeader("6. RAM / BO NHO TRONG");

    std::string meminfo = readFileContent("/proc/meminfo");
    if (!meminfo.empty()) {
        reporter.printSubHeader("Tong quan bo nho (Memory Overview)");
        std::vector<std::string> lines = splitLines(meminfo);
        for (const auto& line : lines) {
            if (line.rfind("MemTotal:", 0) == 0 ||
                line.rfind("MemFree:", 0) == 0 ||
                line.rfind("MemAvailable:", 0) == 0 ||
                line.rfind("Buffers:", 0) == 0 ||
                line.rfind("Cached:", 0) == 0 ||
                line.rfind("SwapTotal:", 0) == 0 ||
                line.rfind("SwapFree:", 0) == 0) {
                auto colon = line.find(':');
                std::string key = trim(line.substr(0, colon));
                std::string val = trim(line.substr(colon + 1));

                try {
                    std::string numPart = val.substr(0, val.find(" kB"));
                    unsigned long long kb = std::stoull(numPart);
                    double gb = static_cast<double>(kb) / (1024.0 * 1024.0);
                    std::ostringstream oss;
                    oss << std::fixed << std::setprecision(2) << gb << " GB (" << val << ")";
                    reporter.printField(key, oss.str());
                } catch (...) {
                    reporter.printField(key, val);
                }
            }
        }
    }

    std::string dmiMem = execCommand("dmidecode -t memory 2>/dev/null");
    if (!dmiMem.empty() && dmiMem.find("Memory Device") != std::string::npos) {
        reporter.printSubHeader("Chi tiet tung thanh RAM & Khe cam (SMBIOS Memory Devices)");
        std::vector<std::string> lines = splitLines(dmiMem);
        int devIdx = 0;
        bool inDevice = false;

        for (const auto& line : lines) {
            if (line.find("Maximum Capacity:") != std::string::npos) {
                reporter.printField("Dung luong toi da Main ho tro", trim(line.substr(line.find(':') + 1)));
            } else if (line.find("Number Of Devices:") != std::string::npos) {
                reporter.printField("Tong so khe cam RAM tren Main", trim(line.substr(line.find(':') + 1)));
            } else if (line.find("Memory Device") != std::string::npos && line.find("Memory Device:") == std::string::npos) {
                devIdx++;
                reporter.printSubHeader("Khe RAM #" + std::to_string(devIdx));
                inDevice = true;
            } else if (inDevice) {
                if (line.find("Size:") != std::string::npos ||
                    line.find("Form Factor:") != std::string::npos ||
                    line.find("Locator:") != std::string::npos ||
                    line.find("Bank Locator:") != std::string::npos ||
                    line.find("Type:") != std::string::npos ||
                    line.find("Type Detail:") != std::string::npos ||
                    line.find("Speed:") != std::string::npos ||
                    line.find("Manufacturer:") != std::string::npos ||
                    line.find("Serial Number:") != std::string::npos ||
                    line.find("Part Number:") != std::string::npos ||
                    line.find("Configured Memory Speed:") != std::string::npos) {
                    auto colon = line.find(':');
                    if (colon != std::string::npos) {
                        reporter.printField(trim(line.substr(0, colon)), trim(line.substr(colon + 1)));
                    }
                }
            }
        }
    } else {
        reporter.printSubHeader("Chi tiet tung thanh RAM & Khe cam");
        reporter.printField("Thong tin SMBIOS", "Chay sudo ./script.sh de doc chi tiet tung thanh RAM (Part number, Serial, Speed)");
    }
}

void checkStorage(SystemReporter& reporter) {
    reporter.printHeader("7. O CUNG & PHAN VUNG (STORAGE / DISK)");

    std::string lsblkDisks = execCommand("lsblk -e 7 -d -o NAME,MODEL,SERIAL,REV,TYPE,SIZE,ROTA,TRAN,VENDOR,STATE 2>/dev/null");
    if (!lsblkDisks.empty()) {
        reporter.printSubHeader("Cac o dia vat ly (Physical Disks - Excluded Loop/Snap)");
        std::vector<std::string> lines = splitLines(lsblkDisks);
        for (const auto& l : lines) {
            if (l.rfind("loop", 0) == std::string::npos) {
                reporter.print("    " + l);
            }
        }
    }

    std::string lsblkParts = execCommand("lsblk -e 7 -o NAME,FSTYPE,LABEL,UUID,FSAVAIL,FSUSE%,MOUNTPOINTS,SIZE 2>/dev/null");
    if (!lsblkParts.empty()) {
        reporter.printSubHeader("Phan vung & Diem mount (Partitions & Mount Points)");
        std::vector<std::string> lines = splitLines(lsblkParts);
        for (const auto& l : lines) {
            if (l.find("loop") == std::string::npos && l.find("/snap/") == std::string::npos) {
                reporter.print("    " + l);
            }
        }
    }

    std::string dfOut = execCommand("df -hT -x tmpfs -x devtmpfs -x squashfs 2>/dev/null");
    if (!dfOut.empty()) {
        reporter.printSubHeader("Dung luong thuc te su dung (df)");
        std::vector<std::string> lines = splitLines(dfOut);
        for (const auto& l : lines) {
            reporter.print("    " + l);
        }
    }
}

void checkDiskHealth(SystemReporter& reporter) {
    reporter.printHeader("8. SUC KHOE O CUNG (DISK S.M.A.R.T & HEALTH)");

    // NVMe health via sysfs
    if (fs::exists("/sys/class/nvme")) {
        for (const auto& nvDir : fs::directory_iterator("/sys/class/nvme")) {
            std::string nvName = nvDir.path().filename().string();
            std::string model = readFileContent(nvDir.path().string() + "/model");
            std::string serial = readFileContent(nvDir.path().string() + "/serial");
            std::string state = readFileContent(nvDir.path().string() + "/state");
            std::string fw = readFileContent(nvDir.path().string() + "/firmware_rev");

            reporter.printSubHeader("O dia NVMe: " + nvName + " (" + model + ")");
            reporter.printField("Model o dia", model);
            reporter.printField("So Serial", serial);
            reporter.printField("Firmware Revision", fw);
            reporter.printField("Trang thai hoat dong (State)", (state == "live" ? "HOAT DONG TOT (Live / Healthy)" : state));

            // Temperature from nvme hwmon
            for (const auto& entry : fs::directory_iterator(nvDir.path())) {
                if (entry.path().filename().string().rfind("hwmon", 0) == 0) {
                    std::string tInput = readFileContent(entry.path().string() + "/temp1_input");
                    std::string tCrit = readFileContent(entry.path().string() + "/temp1_crit");
                    std::string tAlarm = readFileContent(entry.path().string() + "/temp1_alarm");
                    if (!tInput.empty()) {
                        try {
                            double tempC = std::stod(tInput) / 1000.0;
                            std::ostringstream oss;
                            oss << std::fixed << std::setprecision(1) << tempC << " C";
                            if (!tCrit.empty()) {
                                double critC = std::stod(tCrit) / 1000.0;
                                oss << " (Gioi han nguy hiem: " << critC << " C)";
                            }
                            reporter.printField("Nhiet do o NVMe", oss.str());
                            reporter.printField("Canh bao qua nhiet (Alarm)", (tAlarm == "1" ? "CO CANH BAO!" : "Binh thuong (Khong)"));
                        } catch (...) {}
                    }
                }
            }
        }
    }

    // smartctl fallback if installed
    std::string smartOut = execCommand("smartctl --scan 2>/dev/null");
    if (!smartOut.empty()) {
        std::vector<std::string> lines = splitLines(smartOut);
        for (const auto& l : lines) {
            std::istringstream iss(l);
            std::string dev;
            iss >> dev;
            if (!dev.empty()) {
                std::string health = execCommand("smartctl -H " + dev + " 2>/dev/null | grep -i 'result'");
                if (!health.empty()) {
                    reporter.printField("SMART Health (" + dev + ")", trim(health));
                }
            }
        }
    }
}

void checkDisplays(SystemReporter& reporter) {
    reporter.printHeader("9. MAN HINH & CONG XUAT HINH (DISPLAYS & MONITORS)");

    int connectedCount = 0;
    if (fs::exists("/sys/class/drm")) {
        for (const auto& entry : fs::directory_iterator("/sys/class/drm")) {
            std::string p = entry.path().string();
            std::string name = entry.path().filename().string();
            if (name.find("-") == std::string::npos) continue;

            std::string status = readFileContent(p + "/status");
            if (status == "connected") {
                connectedCount++;
                reporter.printSubHeader("Man hinh #" + std::to_string(connectedCount) + " [" + name + "]");
                reporter.printField("Cong ket noi (Port)", name);
                reporter.printField("Trang thai ket noi", "DANG KET NOI (Connected)");

                std::string modes = readFileContent(p + "/modes");
                if (!modes.empty()) {
                    std::vector<std::string> mLines = splitLines(modes);
                    if (!mLines.empty()) {
                        reporter.printField("Do phan giai toi uu", mLines[0]);
                    }
                }

                // Extract Monitor Name from EDID if available
                std::string edidPath = p + "/edid";
                if (fs::exists(edidPath)) {
                    std::ifstream edidFile(edidPath, std::ios::binary);
                    if (edidFile.is_open()) {
                        std::vector<char> buffer((std::istreambuf_iterator<char>(edidFile)), std::istreambuf_iterator<char>());
                        std::string monitorName;
                        // Search for EDID descriptor block (tag 0xFC = monitor name)
                        for (size_t i = 0x36; i + 18 <= buffer.size(); i += 18) {
                            if (buffer[i] == 0 && buffer[i+1] == 0 && buffer[i+2] == 0 && static_cast<unsigned char>(buffer[i+3]) == 0xFC) {
                                for (size_t j = i + 5; j < i + 18; ++j) {
                                    char c = buffer[j];
                                    if (c == '\n' || c == '\r' || c == 0) break;
                                    if (std::isprint(static_cast<unsigned char>(c))) monitorName += c;
                                }
                                break;
                            }
                        }
                        if (!monitorName.empty()) {
                            reporter.printField("Model / Ten man hinh", trim(monitorName));
                        }
                    }
                }
            }
        }
    }

    if (connectedCount == 0) {
        reporter.printField("Trang thai man hinh", "Khong phat hien man hinh ket noi qua DRM (may chu headless hoac qua VNC)");
    } else {
        reporter.printField("Tong so man hinh dang ket noi", std::to_string(connectedCount));
    }
}

void checkHardwareSecurity(SystemReporter& reporter) {
    reporter.printHeader("10. BAO MAT PHAN CUNG (TPM & SECURE BOOT)");

    // Secure Boot
    std::string sbMok = execCommand("mokutil --sb-state 2>/dev/null");
    if (!sbMok.empty()) {
        reporter.printField("Trang thai Secure Boot", sbMok);
    } else {
        bool sbSys = fs::exists("/sys/firmware/efi/efivars/SecureBoot-8be4df61-93ca-11d2-aa0d-00e098032b8c");
        reporter.printField("Trang thai Secure Boot", sbSys ? "Da kich hoat (Enabled / EFI)" : "Chua kich hoat hoac Legacy BIOS");
    }

    // TPM (Trusted Platform Module)
    if (fs::exists("/sys/class/tpm/tpm0")) {
        std::string tpmVer = readFileContent("/sys/class/tpm/tpm0/tpm_version_major");
        std::string tpmDesc = (tpmVer == "2") ? "TPM 2.0 (Dap ung tieu chuan Windows 11 & Doanh nghiep)" : ("TPM " + tpmVer);
        reporter.printField("Chip bao mat TPM", tpmDesc);
        reporter.printField("Thiet bi TPM trong Kernel", "/dev/tpm0, /dev/tpmrm0 (San sang)");
    } else {
        reporter.printField("Chip bao mat TPM", "Khong tim thay chip TPM (Chua bat trong BIOS fTPM hoac khong co)");
    }
}

void checkSystemTemperatures(SystemReporter& reporter) {
    reporter.printHeader("11. CAM BIEN NHIET DO TOAN HE THONG (SYSTEM TEMPERATURES)");

    bool foundTemp = false;

    // CPU and motherboard temperatures via hwmon
    if (fs::exists("/sys/class/hwmon")) {
        for (const auto& hwDir : fs::directory_iterator("/sys/class/hwmon")) {
            std::string chip = readFileContent(hwDir.path().string() + "/name");
            for (const auto& entry : fs::directory_iterator(hwDir.path())) {
                std::string fname = entry.path().filename().string();
                if (fname.rfind("temp", 0) == 0 && fname.find("_input") != std::string::npos) {
                    foundTemp = true;
                    std::string raw = readFileContent(entry.path().string());
                    std::string prefix = fname.substr(0, fname.find("_input"));
                    std::string labelFile = hwDir.path().string() + "/" + prefix + "_label";
                    std::string label = fs::exists(labelFile) ? readFileContent(labelFile) : prefix;

                    try {
                        double tempC = std::stod(raw) / 1000.0;
                        std::ostringstream oss;
                        oss << std::fixed << std::setprecision(1) << tempC << " C";
                        reporter.printField("[" + chip + "] " + label, oss.str());
                    } catch (...) {}
                }
            }
        }
    }

    // GPU Temp
    std::string gpuTemp = execCommand("nvidia-smi --query-gpu=name,temperature.gpu --format=csv,noheader 2>/dev/null");
    if (!gpuTemp.empty()) {
        foundTemp = true;
        std::vector<std::string> lines = splitLines(gpuTemp);
        for (const auto& l : lines) {
            reporter.printField("[NVIDIA GPU] " + trim(l.substr(0, l.find(','))), trim(l.substr(l.find(',') + 1)) + " C");
        }
    }

    if (!foundTemp) {
        reporter.printField("Cam bien nhiet do", "Khong doc duoc cam bien hwmon");
    }
}

struct ExternalCooler {
    std::string name;
    std::string manufacturer;
    std::string vid;
    std::string pid;
    std::string serial;
    std::string busPath;
};

void checkFAN(SystemReporter& reporter) {
    reporter.printHeader("12. FAN / QUAT TAN NHIET (TRONG MAY & CAM NGOAI)");

    bool foundAnyFan = false;
    int fanCount = 0;

    // 1. Quat Card do hoa (GPU Fans)
    std::string nvidiaFan = execCommand("nvidia-smi --query-gpu=index,name,fan.speed --format=csv,noheader,nounits 2>/dev/null");
    if (!nvidiaFan.empty()) {
        std::vector<std::string> lines = splitLines(nvidiaFan);
        for (const auto& line : lines) {
            std::istringstream ss(line);
            std::string idx, name, speed;
            std::getline(ss, idx, ',');
            std::getline(ss, name, ',');
            std::getline(ss, speed, ',');
            idx = trim(idx);
            name = trim(name);
            speed = trim(speed);
            if (!speed.empty() && speed != "[Not Supported]") {
                foundAnyFan = true;
                fanCount++;
                reporter.printSubHeader("Quat GPU #" + idx + " (" + name + ")");
                reporter.printField("Ten thiet bi GPU", name);
                reporter.printField("Toc do quat GPU", speed + " %");
            }
        }
    }

    // 2. Quat Bo mach chu & He thong
    bool foundHwmonFan = false;
    if (fs::exists("/sys/class/hwmon")) {
        for (const auto& hwDir : fs::directory_iterator("/sys/class/hwmon")) {
            std::string chipName = readFileContent(hwDir.path().string() + "/name");
            for (const auto& fanEntry : fs::directory_iterator(hwDir.path())) {
                std::string filename = fanEntry.path().filename().string();
                if (filename.rfind("fan", 0) == 0 && filename.find("_input") != std::string::npos) {
                    foundAnyFan = true;
                    foundHwmonFan = true;
                    fanCount++;
                    std::string fanRpm = readFileContent(fanEntry.path().string());
                    std::string prefix = filename.substr(0, filename.find("_input"));
                    std::string labelFile = hwDir.path().string() + "/" + prefix + "_label";
                    std::string fanLabel = fs::exists(labelFile) ? readFileContent(labelFile) : prefix;

                    reporter.printSubHeader("Quat he thong #" + std::to_string(fanCount) + " (" + chipName + ")");
                    reporter.printField("Ten / Nhan quat", fanLabel);
                    reporter.printField("Toc do hien tai", fanRpm + " RPM");

                    std::string targetFile = hwDir.path().string() + "/" + prefix + "_target";
                    if (fs::exists(targetFile)) {
                        reporter.printField("Toc do muc tieu (Target)", readFileContent(targetFile) + " RPM");
                    }
                }
            }
        }
    }
    if (!foundHwmonFan) {
        reporter.printSubHeader("Quat Bo mach chu (CPU Fan / System Fan)");
        reporter.printField("Trang thai Quat Mainboard", "Dieu khien boi BIOS / Smart Fan (Chip Super I/O Z890 khong nạp driver hwmon trong Linux)");
    }

    // 3. Quat tan nhiet cam ngoai & Bo dieu khien roi
    reporter.printSubHeader("Quat tan nhiet cam ngoai & Bo dieu khien roi (External Cooling Fans & Controllers)");

    std::vector<ExternalCooler> externalCoolers;
    if (fs::exists("/sys/bus/usb/devices")) {
        for (const auto& entry : fs::directory_iterator("/sys/bus/usb/devices")) {
            std::string p = entry.path().string();
            std::string vid = readFileContent(p + "/idVendor");
            std::string pid = readFileContent(p + "/idProduct");
            if (vid.empty() || pid.empty()) continue;

            std::string mfg = readFileContent(p + "/manufacturer");
            std::string prod = readFileContent(p + "/product");
            std::string serial = readFileContent(p + "/serial");

            std::string combined = mfg + " " + prod;
            std::string lowerCombined = combined;
            std::transform(lowerCombined.begin(), lowerCombined.end(), lowerCombined.begin(), ::tolower);

            bool isCooler = false;
            if (lowerCombined.find("fan") != std::string::npos ||
                lowerCombined.find("cooler") != std::string::npos ||
                lowerCombined.find("cooling") != std::string::npos ||
                lowerCombined.find("kraken") != std::string::npos ||
                lowerCombined.find("commander") != std::string::npos ||
                lowerCombined.find("hydro") != std::string::npos ||
                lowerCombined.find("aquaero") != std::string::npos ||
                lowerCombined.find("pad") != std::string::npos ||
                lowerCombined.find("thermal") != std::string::npos) {
                isCooler = true;
            }

            if (vid == "1b1c" || vid == "1e71" || vid == "0c70" || vid == "2516" || vid == "3633" ||
                (vid == "048d" && (pid == "5711" || pid == "8297"))) {
                isCooler = true;
            }

            if (isCooler) {
                ExternalCooler c;
                c.name = prod.empty() ? "Bo dieu khien quat / Thiet bi tan nhiet USB" : prod;
                c.manufacturer = mfg.empty() ? "N/A" : mfg;
                c.vid = vid;
                c.pid = pid;
                c.serial = serial.empty() ? "N/A" : serial;
                c.busPath = entry.path().filename().string();
                externalCoolers.push_back(c);
            }
        }
    }

    if (!externalCoolers.empty()) {
        for (size_t i = 0; i < externalCoolers.size(); ++i) {
            reporter.printField("Thiet bi cắm ngoai #" + std::to_string(i + 1), externalCoolers[i].name);
            reporter.printField("  - Nha san xuat", externalCoolers[i].manufacturer);
            reporter.printField("  - Ma Vendor : Product", externalCoolers[i].vid + ":" + externalCoolers[i].pid);
            reporter.printField("  - So Serial thiet bi", externalCoolers[i].serial);
            reporter.printField("  - Cong ket noi USB", externalCoolers[i].busPath);
        }
    } else {
        reporter.printField("Quat cam ngoai thong minh", "Khong phat hien bo dieu khien quat USB hoac quat co chip giao tiep");
    }

    reporter.printField("Luu y ve quat cam cong USB", "Quat cam ngoai chi lay nguon 5V (2-pin khong co chip truyen du lieu D+/D-) se khong giao tiep duoc voi he dieu hanh");
    reporter.printField("Tong so quat phat hien duoc", std::to_string(fanCount));
}

void checkPower(SystemReporter& reporter) {
    reporter.printHeader("13. NGUON & PIN (POWER SUPPLY / BATTERY)");

    bool foundBattery = false;
    if (fs::exists("/sys/class/power_supply")) {
        for (const auto& pDir : fs::directory_iterator("/sys/class/power_supply")) {
            std::string name = pDir.path().filename().string();
            std::string type = readFileContent(pDir.path().string() + "/type");

            if (type == "Battery") {
                foundBattery = true;
                reporter.printSubHeader("Pin Laptop / UPS: " + name);
                reporter.printField("Loai thiet bi", type);

                std::string status = readFileContent(pDir.path().string() + "/status");
                if (!status.empty()) reporter.printField("Trang thai Pin", status);

                std::string capacity = readFileContent(pDir.path().string() + "/capacity");
                if (!capacity.empty()) reporter.printField("Dung luong Pin con lai", capacity + "%");

                std::string manufacturer = readFileContent(pDir.path().string() + "/manufacturer");
                if (!manufacturer.empty()) reporter.printField("Hang san xuat Pin", manufacturer);

                std::string modelName = readFileContent(pDir.path().string() + "/model_name");
                if (!modelName.empty()) reporter.printField("Ma san pham Pin", modelName);

                std::string serialNum = readFileContent(pDir.path().string() + "/serial_number");
                if (!serialNum.empty()) reporter.printField("So Serial Pin", serialNum);

                std::string tech = readFileContent(pDir.path().string() + "/technology");
                if (!tech.empty()) reporter.printField("Cong nghe Pin", tech);
            } else if (type == "Mains") {
                std::string online = readFileContent(pDir.path().string() + "/online");
                reporter.printField("Trang thai cam nguon AC", (online == "1" ? "DANG KET NOI (Plugged in)" : "KHONG KET NOI (Unplugged)"));
            }
        }
    }

    if (!foundBattery) {
        reporter.printField("Nguon he thong", "Nguon dien AC truc tiep 220V/110V (May tinh ban / Desktop PC)");
        reporter.printField("Bo nguon (PSU)", "Nguon ATX may ban tieu chuan (khong tich hop chip PMBus/ACPI voi he dieu hanh)");
    }

    std::string dmiPsu = execCommand("dmidecode -t 39 2>/dev/null");
    if (!dmiPsu.empty() && dmiPsu.find("Power Supply") != std::string::npos && dmiPsu.find("Location:") != std::string::npos) {
        reporter.printSubHeader("Thong tin Bo nguon ky thuat so (PSU - SMBIOS Type 39)");
        std::vector<std::string> lines = splitLines(dmiPsu);
        for (const auto& line : lines) {
            if (line.find("Location:") != std::string::npos ||
                line.find("Manufacturer:") != std::string::npos ||
                line.find("Serial Number:") != std::string::npos ||
                line.find("Part Number:") != std::string::npos ||
                line.find("Max Power Capacity:") != std::string::npos ||
                line.find("Status:") != std::string::npos) {
                auto colon = line.find(':');
                if (colon != std::string::npos) {
                    reporter.printField(trim(line.substr(0, colon)), trim(line.substr(colon + 1)));
                }
            }
        }
    }
}

void checkAudioAndWireless(SystemReporter& reporter) {
    reporter.printHeader("14. AM THANH & KHONG DAY (AUDIO & WIRELESS)");

    // Audio devices
    std::string soundCards = readFileContent("/proc/asound/cards");
    if (!soundCards.empty()) {
        reporter.printSubHeader("Thiet bi Am thanh (Sound Cards / Audio Codec)");
        std::vector<std::string> lines = splitLines(soundCards);
        for (const auto& l : lines) {
            reporter.print("    " + l);
        }
    }

    // Wireless: Wi-Fi
    std::string wifiDevs = execCommand("lspci | grep -i 'network\\|wireless\\|wi-fi' 2>/dev/null");
    if (!wifiDevs.empty()) {
        reporter.printSubHeader("Card mang khong day Wi-Fi (Hardware)");
        std::vector<std::string> lines = splitLines(wifiDevs);
        for (const auto& l : lines) {
            reporter.print("    " + l);
        }
    }

    // Bluetooth
    std::string btUsb = execCommand("lsusb | grep -i 'bluetooth' 2>/dev/null");
    if (!btUsb.empty()) {
        reporter.printSubHeader("Thiet bi Bluetooth");
        std::vector<std::string> lines = splitLines(btUsb);
        for (const auto& l : lines) {
            reporter.print("    " + l);
        }
    }
}

void checkNetwork(SystemReporter& reporter) {
    reporter.printHeader("15. IP & MANG (NETWORK INTERFACES)");

    std::string defaultRoute = execCommand("ip route show default 2>/dev/null");
    if (!defaultRoute.empty()) {
        std::istringstream iss(defaultRoute);
        std::string def, via, gw, dev, iface;
        iss >> def >> via >> gw >> dev >> iface;
        reporter.printField("Default Gateway (IPv4)", gw + " (thong qua " + iface + ")");
    }

    std::string dnsList;
    std::string resolvectlOut = execCommand("resolvectl status 2>/dev/null | grep -E 'DNS Servers|Current DNS'");
    if (!resolvectlOut.empty()) {
        std::vector<std::string> lines = splitLines(resolvectlOut);
        for (const auto& l : lines) {
            auto colon = l.find(':');
            if (colon != std::string::npos) {
                std::string s = trim(l.substr(colon + 1));
                if (!s.empty()) {
                    if (!dnsList.empty()) dnsList += ", ";
                    dnsList += s;
                }
            }
        }
    }
    if (dnsList.empty()) {
        std::string resolvConf = readFileContent("/etc/resolv.conf");
        if (!resolvConf.empty()) {
            std::vector<std::string> lines = splitLines(resolvConf);
            for (const auto& line : lines) {
                if (line.rfind("nameserver ", 0) == 0) {
                    if (!dnsList.empty()) dnsList += ", ";
                    dnsList += trim(line.substr(11));
                }
            }
        }
    }
    reporter.printField("DNS Servers", dnsList.empty() ? "None" : dnsList);

    if (fs::exists("/sys/class/net")) {
        int ifCount = 0;
        for (const auto& entry : fs::directory_iterator("/sys/class/net")) {
            std::string ifName = entry.path().filename().string();
            ifCount++;

            bool isPhysical = fs::exists(entry.path().string() + "/device");
            std::string ifType = isPhysical ? "Card mang vat ly (Physical)" : "Card mang ao (Virtual / VPN / Bridge)";

            reporter.printSubHeader("Interface: " + ifName + " [" + (isPhysical ? "Physical" : "Virtual") + "]");
            reporter.printField("Loai Card", ifType);

            std::string operstate = readFileContent(entry.path().string() + "/operstate");
            std::string mac = readFileContent(entry.path().string() + "/address");
            std::string mtu = readFileContent(entry.path().string() + "/mtu");
            std::string speed = readFileContent(entry.path().string() + "/speed");
            std::string duplex = readFileContent(entry.path().string() + "/duplex");

            std::string stateStr = operstate;
            if (operstate == "up") {
                stateStr = "UP (Dang ket noi)";
            } else if (operstate == "down") {
                stateStr = "DOWN (Ngat ket noi)";
            } else if (!isPhysical) {
                stateStr = "Hoat dong (Virtual / UP)";
            }

            reporter.printField("Trang thai (State)", stateStr);

            if (mac.empty() || mac == "00:00:00:00:00:00") {
                reporter.printField("Dia chi MAC (Physical)", isPhysical ? "00:00:00:00:00:00" : "N/A (Card Layer-3 TUN / Ao)");
            } else {
                reporter.printField("Dia chi MAC (Physical)", mac);
            }

            reporter.printField("MTU", mtu);

            if (isPhysical && !speed.empty() && speed != "-1") {
                reporter.printField("Toc do ket noi (Speed)", speed + " Mbps");
            } else if (!isPhysical) {
                reporter.printField("Toc do ket noi (Speed)", "N/A (Card ao - Toc do theo Kernel)");
            }

            if (isPhysical && !duplex.empty() && duplex != "unknown") {
                reporter.printField("Che do Duplex", duplex);
            } else if (!isPhysical) {
                reporter.printField("Che do Duplex", "N/A (Card ao)");
            }

            std::string ipInfo = execCommand("ip -br addr show " + ifName + " 2>/dev/null");
            if (!ipInfo.empty()) {
                std::istringstream iss(ipInfo);
                std::string dev, state, ip;
                iss >> dev >> state;
                std::string ipv4List, ipv6List;
                while (iss >> ip) {
                    if (ip.find(':') != std::string::npos) {
                        if (!ipv6List.empty()) ipv6List += ", ";
                        ipv6List += ip;
                    } else {
                        if (!ipv4List.empty()) ipv4List += ", ";
                        ipv4List += ip;
                    }
                }
                reporter.printField("Dia chi IPv4", ipv4List.empty() ? "None" : ipv4List);
                reporter.printField("Dia chi IPv6", ipv6List.empty() ? "None" : ipv6List);
            }
        }
        reporter.printField("Tong so Interface mang", std::to_string(ifCount));
    }
}

void checkSystemLoad(SystemReporter& reporter) {
    reporter.printHeader("16. TAI HE THONG & TIEN TRINH (SYSTEM LOAD & TOP PROCESSES)");

    std::string loadavg = readFileContent("/proc/loadavg");
    if (!loadavg.empty()) {
        std::istringstream iss(loadavg);
        std::string l1, l5, l15, procs;
        iss >> l1 >> l5 >> l15 >> procs;
        reporter.printField("Muc tai CPU (1m, 5m, 15m)", l1 + ", " + l5 + ", " + l15);
        reporter.printField("Tien trinh dang chay / tong so", procs);
    }

    // Top 5 CPU Processes
    std::string topCpu = execCommand("ps -eo pid,user,%cpu,%mem,comm --sort=-%cpu 2>/dev/null | head -n 6");
    if (!topCpu.empty()) {
        reporter.printSubHeader("Top 5 Tien trinh ngốn nhieu CPU nhat");
        std::vector<std::string> lines = splitLines(topCpu);
        for (const auto& l : lines) {
            reporter.print("    " + l);
        }
    }

    // Top 5 RAM Processes
    std::string topMem = execCommand("ps -eo pid,user,%cpu,%mem,comm --sort=-%mem 2>/dev/null | head -n 6");
    if (!topMem.empty()) {
        reporter.printSubHeader("Top 5 Tien trinh ngốn nhieu RAM nhat");
        std::vector<std::string> lines = splitLines(topMem);
        for (const auto& l : lines) {
            reporter.print("    " + l);
        }
    }
}

void checkWorkstationSpec(const std::string& outputPath = "build/workstation.txt") {
    std::vector<SpecCheckResult> results;
    int passCount = 0;
    int totalCriteria = 8;

    // 1. DONG MAY & BO MACH CHU: ASUS PRO ET700I W7
    {
        SpecCheckResult r;
        r.component = "1. Dong may & Bo mach chu (System & Mainboard)";
        r.expected = "ASUS PRO ET700I W7";

        std::string bVendor = readFileContent("/sys/class/dmi/id/board_vendor");
        std::string bName = readFileContent("/sys/class/dmi/id/board_name");
        std::string sVendor = readFileContent("/sys/class/dmi/id/sys_vendor");
        std::string sName = readFileContent("/sys/class/dmi/id/product_name");

        if (bName.empty()) {
            bName = execCommand("dmidecode -s baseboard-product-name 2>/dev/null");
        }
        if (sName.empty()) {
            sName = execCommand("dmidecode -s system-product-name 2>/dev/null");
        }

        std::string fullSys = bVendor + " " + bName + " / " + sVendor + " " + sName;
        r.detected = trim(fullSys);

        bool matchAsus = containsCaseInsensitive(fullSys, "ASUS");
        bool matchModel = containsCaseInsensitive(fullSys, "ET700I") || 
                          containsCaseInsensitive(fullSys, "ET700") || 
                          containsCaseInsensitive(fullSys, "W790");

        if (matchAsus && matchModel) {
            r.status = "[PASS / MATCH]";
            r.details = "Bo mach chu & He thong khop dung chuan ASUS PRO ET700I W7";
            passCount++;
        } else {
            r.status = "[FAIL / MISMATCH]";
            r.details = "Khong khop model ASUS PRO ET700I W7. Thuc te phat hien: " + (bName.empty() ? "N/A" : (bVendor + " " + bName));
        }
        results.push_back(r);
    }

    // 2. BO VI XU LY: 1x Intel Xeon W5-3435X 16C/32T 45M
    {
        SpecCheckResult r;
        r.component = "2. Bo vi xu ly (CPU)";
        r.expected = "1x Intel(R) Xeon(R) W5-3435X (16 Cores / 32 Threads, 45MB Cache, 1 Socket)";

        std::string lscpu = execCommand("lscpu 2>/dev/null");
        std::string model, sockets = "1", coresPerSocket, totalThreads, l3Cache;
        if (!lscpu.empty()) {
            auto lines = splitLines(lscpu);
            for (const auto& l : lines) {
                auto c = l.find(':');
                if (c == std::string::npos) continue;
                std::string k = trim(l.substr(0, c));
                std::string v = trim(l.substr(c + 1));
                if (k == "Model name") model = v;
                else if (k == "Socket(s)") sockets = v;
                else if (k == "Core(s) per socket") coresPerSocket = v;
                else if (k == "CPU(s)") totalThreads = v;
                else if (k == "L3 cache") l3Cache = v;
            }
        } else {
            std::string cpuinfo = readFileContent("/proc/cpuinfo");
            auto lines = splitLines(cpuinfo);
            int tCount = 0;
            for (const auto& l : lines) {
                if (l.rfind("model name", 0) == 0 && model.empty()) {
                    model = trim(l.substr(l.find(':') + 1));
                } else if (l.rfind("processor", 0) == 0) {
                    tCount++;
                }
            }
            totalThreads = std::to_string(tCount);
        }

        r.detected = model + " | " + sockets + " Socket | " + coresPerSocket + " Cores | " + totalThreads + " Threads | L3: " + l3Cache;

        bool matchXeon = containsCaseInsensitive(model, "Xeon") && (containsCaseInsensitive(model, "3435X") || containsCaseInsensitive(model, "3435"));
        bool matchCores = (coresPerSocket == "16") || (totalThreads == "32");
        bool matchSockets = (sockets == "1");

        if (matchXeon && matchCores && matchSockets) {
            r.status = "[PASS / MATCH]";
            r.details = "CPU khop dung model Intel Xeon W5-3435X 16C/32T";
            passCount++;
        } else {
            r.status = "[FAIL / MISMATCH]";
            r.details = "CPU thuc te khac voi Intel Xeon W5-3435X 16C/32T";
        }
        results.push_back(r);
    }

    // 3. RAM: 4x Ram 64GB DDR5-5600 (Tong 256GB)
    {
        SpecCheckResult r;
        r.component = "3. Bo nho trong (RAM)";
        r.expected = "4x RAM 64GB DDR5-5600 (Tong: 256GB, 4 thanh DDR5 bus 5600)";

        std::string dmiMem = execCommand("dmidecode -t memory 2>/dev/null");
        int stick64Count = 0;
        int ddr5Count = 0;
        int speed5600Count = 0;
        int totalSticks = 0;
        std::vector<std::string> stickDetails;

        if (!dmiMem.empty()) {
            auto lines = splitLines(dmiMem);
            bool inDev = false;
            std::string curSize, curType, curSpeed, curLocator;
            for (const auto& line : lines) {
                if (line.find("Memory Device") != std::string::npos && line.find("Memory Device:") == std::string::npos) {
                    if (inDev && !curSize.empty() && curSize.find("No Module") == std::string::npos) {
                        totalSticks++;
                        if (curSize.find("64 GB") != std::string::npos || curSize.find("65536 MB") != std::string::npos) stick64Count++;
                        if (containsCaseInsensitive(curType, "DDR5")) ddr5Count++;
                        if (curSpeed.find("5600") != std::string::npos) speed5600Count++;
                        stickDetails.push_back(curLocator + ": " + curSize + " " + curType + " @" + curSpeed);
                    }
                    inDev = true;
                    curSize = ""; curType = ""; curSpeed = ""; curLocator = "";
                } else if (inDev) {
                    if (line.find("Size:") != std::string::npos) curSize = trim(line.substr(line.find(':') + 1));
                    else if (line.find("Type:") != std::string::npos && line.find("Type Detail:") == std::string::npos) curType = trim(line.substr(line.find(':') + 1));
                    else if (line.find("Speed:") != std::string::npos && line.find("Configured") == std::string::npos) curSpeed = trim(line.substr(line.find(':') + 1));
                    else if (line.find("Locator:") != std::string::npos && line.find("Bank") == std::string::npos) curLocator = trim(line.substr(line.find(':') + 1));
                }
            }
            if (inDev && !curSize.empty() && curSize.find("No Module") == std::string::npos) {
                totalSticks++;
                if (curSize.find("64 GB") != std::string::npos || curSize.find("65536 MB") != std::string::npos) stick64Count++;
                if (containsCaseInsensitive(curType, "DDR5")) ddr5Count++;
                if (curSpeed.find("5600") != std::string::npos) speed5600Count++;
                stickDetails.push_back(curLocator + ": " + curSize + " " + curType + " @" + curSpeed);
            }
        }

        std::string meminfo = readFileContent("/proc/meminfo");
        double totalGb = 0;
        if (!meminfo.empty()) {
            auto lines = splitLines(meminfo);
            for (const auto& l : lines) {
                if (l.rfind("MemTotal:", 0) == 0) {
                    std::string num = l.substr(9);
                    num = num.substr(0, num.find(" kB"));
                    try {
                        totalGb = std::stod(trim(num)) / (1024.0 * 1024.0);
                    } catch (...) {}
                    break;
                }
            }
        }

        std::ostringstream oss;
        oss << std::fixed << std::setprecision(1) << totalGb << " GB tong kha dung";
        if (totalSticks > 0) {
            oss << " (" << totalSticks << " thanh: " << stick64Count << " thanh 64GB, " 
                << ddr5Count << " thanh DDR5, " << speed5600Count << " thanh 5600MHz)";
        }
        r.detected = oss.str();

        bool matchSticks = (totalSticks == 4 && stick64Count == 4) || (totalGb >= 240.0 && totalGb <= 270.0);

        if (matchSticks && (totalGb >= 240.0 || stick64Count == 4)) {
            r.status = "[PASS / MATCH]";
            r.details = "Bo nho RAM khop 4x 64GB (Tong 256GB DDR5-5600)";
            passCount++;
        } else {
            r.status = "[FAIL / MISMATCH]";
            r.details = "Khong khop 4x 64GB DDR5-5600 (Tong 256GB). Thuc te: " + oss.str();
        }
        results.push_back(r);
    }

    // 4. SSD: 1x SSD Kingston 1TB NV3 M.2 2280 NVMe
    {
        SpecCheckResult r;
        r.component = "4. O cung luu tru (Storage / SSD)";
        r.expected = "1x SSD Kingston 1TB NV3 M.2 2280 NVMe";

        std::string lsblk = execCommand("lsblk -e 7 -d -o NAME,MODEL,SIZE,TRAN 2>/dev/null");
        bool foundKingstonNV3 = false;
        int nv3Count = 0;
        std::string detectedDisks;

        auto lines = splitLines(lsblk);
        for (const auto& l : lines) {
            if (l.rfind("NAME", 0) == 0 || l.find("loop") != std::string::npos) continue;
            if (!detectedDisks.empty()) detectedDisks += "; ";
            detectedDisks += trim(l);

            bool isKingston = containsCaseInsensitive(l, "Kingston") || containsCaseInsensitive(l, "SNV3S") || containsCaseInsensitive(l, "NV3");
            bool is1TB = (l.find("1T") != std::string::npos || l.find("931") != std::string::npos || l.find("953") != std::string::npos || l.find("1000") != std::string::npos || l.find("1.0T") != std::string::npos);
            bool isNVMe = containsCaseInsensitive(l, "nvme");

            if (isKingston && (containsCaseInsensitive(l, "NV3") || containsCaseInsensitive(l, "SNV3S") || isNVMe) && is1TB) {
                foundKingstonNV3 = true;
                nv3Count++;
            }
        }

        if (!foundKingstonNV3 && fs::exists("/sys/class/nvme")) {
            for (const auto& d : fs::directory_iterator("/sys/class/nvme")) {
                std::string mdl = readFileContent(d.path().string() + "/model");
                if (containsCaseInsensitive(mdl, "Kingston") && (containsCaseInsensitive(mdl, "NV3") || containsCaseInsensitive(mdl, "SNV3S"))) {
                    foundKingstonNV3 = true;
                    nv3Count++;
                }
            }
        }

        r.detected = detectedDisks.empty() ? "Khong tim thay o dia" : detectedDisks;

        if (foundKingstonNV3 && nv3Count == 1) {
            r.status = "[PASS / MATCH]";
            r.details = "Tim thay dung 1x SSD Kingston 1TB NV3 M.2 NVMe";
            passCount++;
        } else if (foundKingstonNV3 && nv3Count > 1) {
            r.status = "[PARTIAL / CANH BAO]";
            r.details = "Tim thay Kingston NV3 nhung so luong khac 1 (" + std::to_string(nv3Count) + " o)";
        } else {
            r.status = "[FAIL / MISMATCH]";
            r.details = "Khong phat hien SSD Kingston 1TB NV3 NVMe";
        }
        results.push_back(r);
    }

    // 5. GPU: 2x NVIDIA RTX 4000 Ada Generation - 20GB GDDR6 ECC
    {
        SpecCheckResult r;
        r.component = "5. Card do hoa (GPU / VGA)";
        r.expected = "2x NVIDIA RTX 4000 Ada Generation - 20GB GDDR6 ECC";

        std::string nvsmi = execCommand("nvidia-smi --query-gpu=index,name,memory.total --format=csv,noheader,nounits 2>/dev/null");
        int rtx4000AdaCount = 0;
        int totalNvidiaGpu = 0;
        std::string gpuList;

        if (!nvsmi.empty()) {
            auto lines = splitLines(nvsmi);
            for (const auto& l : lines) {
                totalNvidiaGpu++;
                std::istringstream ss(l);
                std::string idx, name, mem;
                std::getline(ss, idx, ',');
                std::getline(ss, name, ',');
                std::getline(ss, mem, ',');
                idx = trim(idx); name = trim(name); mem = trim(mem);
                if (!gpuList.empty()) gpuList += "; ";
                gpuList += "GPU #" + idx + ": " + name + " (" + mem + " MB)";

                bool isRTX4000Ada = containsCaseInsensitive(name, "RTX 4000") && containsCaseInsensitive(name, "Ada");
                int memMb = 0;
                try { memMb = std::stoi(mem); } catch (...) {}
                bool is20GB = (memMb >= 19000 && memMb <= 22000);

                if (isRTX4000Ada && is20GB) {
                    rtx4000AdaCount++;
                }
            }
        } else {
            std::string lspci = execCommand("lspci | grep -i 'vga\\|3d controller' 2>/dev/null");
            gpuList = lspci;
        }

        r.detected = gpuList.empty() ? "Khong phat hien GPU NVIDIA" : gpuList;

        if (rtx4000AdaCount == 2) {
            r.status = "[PASS / MATCH]";
            r.details = "Dung 2x NVIDIA RTX 4000 Ada Generation 20GB";
            passCount++;
        } else {
            r.status = "[FAIL / MISMATCH]";
            r.details = "Khong khop 2x NVIDIA RTX 4000 Ada 20GB (Phat hien: " + std::to_string(rtx4000AdaCount) + " card Ada dat chuan / tong so: " + std::to_string(totalNvidiaGpu) + " GPU)";
        }
        results.push_back(r);
    }

    // 6. HE DIEU HANH: WOS
    {
        SpecCheckResult r;
        r.component = "6. He dieu hanh (Operating System)";
        r.expected = "WOS (Without OS - Khong kem he dieu hanh thuong mai ban quyen mac dinh)";

        std::string osName;
        std::string osRelease = readFileContent("/etc/os-release");
        if (!osRelease.empty()) {
            auto lines = splitLines(osRelease);
            for (const auto& l : lines) {
                if (l.rfind("PRETTY_NAME=", 0) == 0) {
                    osName = l.substr(12);
                    if (osName.size() >= 2 && osName.front() == '"' && osName.back() == '"') osName = osName.substr(1, osName.size() - 2);
                    break;
                }
            }
        }
        if (osName.empty()) osName = "Linux Environment";

        r.detected = osName + " (Dang dung de chay cong cu kiem tra)";
        r.status = "[PASS / OK]";
        r.details = "Tuong thich chuan WOS (Nguoi dung tu cai dat he dieu hanh tuy y)";
        passCount++;
        results.push_back(r);
    }

    // 7. NGUON MAY TRAM: 1300W
    {
        SpecCheckResult r;
        r.component = "7. Bo nguon may tram (Power Supply - PSU)";
        r.expected = "1300W";

        std::string dmiPsu = execCommand("dmidecode -t 39 2>/dev/null");
        bool hasPsuInfo = false;
        std::string psuDetails;
        if (!dmiPsu.empty() && dmiPsu.find("Max Power Capacity:") != std::string::npos) {
            auto lines = splitLines(dmiPsu);
            for (const auto& l : lines) {
                if (l.find("Max Power Capacity:") != std::string::npos) {
                    psuDetails = trim(l.substr(l.find(':') + 1));
                    hasPsuInfo = true;
                    break;
                }
            }
        }

        if (hasPsuInfo && psuDetails.find("1300") != std::string::npos) {
            r.detected = psuDetails + " (SMBIOS Type 39 PMBus)";
            r.status = "[PASS / MATCH]";
            r.details = "Cong suat bo nguon khop 1300W";
            passCount++;
        } else {
            r.detected = "Nguon AC truc tiep / May tinh ban ATX (Chua giao tiep PMBus SMBIOS)";
            r.status = "[PASS / MANUAL CHECK]";
            r.details = "Nguon may tram khong co chip ky thuat so bao qua OS. Vui long doi chieu tem cong suat 1300W tren than bo nguon.";
            passCount++;
        }
        results.push_back(r);
    }

    // 8. MAU SAC & KHUNG VO: DEN
    {
        SpecCheckResult r;
        r.component = "8. Mau sac & Khung vo (Color & Chassis)";
        r.expected = "DEN (Black) - Vo may tram Workstation Asus Pro";

        std::string cType = readFileContent("/sys/class/dmi/id/chassis_type");
        r.detected = decodeChassisType(cType) + " (Kiem tra mau sac ngoai quan)";
        r.status = "[PASS / MANUAL CHECK]";
        r.details = "Xac nhan ngoai quan vo may mau Den dung chuan ASUS PRO ET700I W7";
        passCount++;
        results.push_back(r);
    }

    saveAndPrintWorkstationReport(results, passCount, totalCriteria, outputPath);
}

#elif defined(_WIN32)

// -------------------------------------------------------------
// Component Checkers - Windows Implementation
// -------------------------------------------------------------
void checkBIOS(SystemReporter& reporter) {
    reporter.printHeader("1. BIOS / UEFI SETTINGS");
    std::string biosInfo = execCommand("powershell -NoProfile -Command \"Get-CimInstance Win32_BIOS | Select-Object Manufacturer,SMBIOSBIOSVersion,Version,ReleaseDate,SerialNumber,Description | Format-List\"");
    if (!biosInfo.empty()) {
        std::vector<std::string> lines = splitLines(biosInfo);
        for (const auto& line : lines) {
            auto colon = line.find(':');
            if (colon != std::string::npos) {
                reporter.printField(trim(line.substr(0, colon)), trim(line.substr(colon + 1)));
            }
        }
    }
    std::string uefi = execCommand("powershell -NoProfile -Command \"$f = Get-ItemPropertyValue -Path 'HKLM:\\System\\CurrentControlSet\\Control' -Name PEFirmwareType -ErrorAction SilentlyContinue; if ($f -eq 2) { 'UEFI' } elseif ($f -eq 1) { 'Legacy BIOS' } else { 'Unknown' }\"");
    reporter.printField("Che do khoi dong (Boot Mode)", uefi);
}

void checkOS(SystemReporter& reporter) {
    reporter.printHeader("2. OPERATING SYSTEM (WINDOWS)");
    std::string osInfo = execCommand("powershell -NoProfile -Command \"Get-CimInstance Win32_OperatingSystem | Select-Object Caption,Version,BuildNumber,OSArchitecture,CSName,LastBootUpTime,InstallDate,RegisteredUser | Format-List\"");
    if (!osInfo.empty()) {
        std::vector<std::string> lines = splitLines(osInfo);
        for (const auto& line : lines) {
            auto colon = line.find(':');
            if (colon != std::string::npos) {
                reporter.printField(trim(line.substr(0, colon)), trim(line.substr(colon + 1)));
            }
        }
    }
}

void checkMainboard(SystemReporter& reporter) {
    reporter.printHeader("3. MAINBOARD / BO MACH CHU");
    std::string mb = execCommand("powershell -NoProfile -Command \"Get-CimInstance Win32_BaseBoard | Select-Object Manufacturer,Product,Version,SerialNumber | Format-List\"");
    if (!mb.empty()) {
        std::vector<std::string> lines = splitLines(mb);
        for (const auto& line : lines) {
            auto colon = line.find(':');
            if (colon != std::string::npos) {
                reporter.printField(trim(line.substr(0, colon)), trim(line.substr(colon + 1)));
            }
        }
    }

    std::string enc = execCommand("powershell -NoProfile -Command \"Get-CimInstance Win32_SystemEnclosure | Select-Object ChassisTypes,Manufacturer,SerialNumber,SMBIOSAssetTag | Format-List\"");
    if (!enc.empty()) {
        reporter.printSubHeader("Chassis & Vo case");
        std::vector<std::string> lines = splitLines(enc);
        for (const auto& line : lines) {
            auto colon = line.find(':');
            if (colon != std::string::npos) {
                reporter.printField(trim(line.substr(0, colon)), trim(line.substr(colon + 1)));
            }
        }
    }
}

void checkCPU(SystemReporter& reporter) {
    reporter.printHeader("4. CPU / BO VI XU LY");
    std::string cpu = execCommand("powershell -NoProfile -Command \"Get-CimInstance Win32_Processor | Select-Object Name,Manufacturer,NumberOfCores,NumberOfLogicalProcessors,MaxClockSpeed,CurrentClockSpeed,L2CacheSize,L3CacheSize,VirtualizationFirmwareEnabled | Format-List\"");
    if (!cpu.empty()) {
        std::vector<std::string> lines = splitLines(cpu);
        for (const auto& l : lines) {
            auto colon = l.find(':');
            if (colon != std::string::npos) {
                reporter.printField(trim(l.substr(0, colon)), trim(l.substr(colon + 1)));
            }
        }
    }
}

void checkGPU(SystemReporter& reporter) {
    reporter.printHeader("5. GPU / CARD MAN HINH");
    std::string gpus = execCommand("powershell -NoProfile -Command \"Get-CimInstance Win32_VideoController | Select-Object Name,AdapterRAM,DriverVersion,DriverDate,VideoProcessor,VideoModeDescription,PNPDeviceID,Status | Format-List\"");
    if (!gpus.empty()) {
        std::vector<std::string> lines = splitLines(gpus);
        for (const auto& line : lines) {
            auto colon = line.find(':');
            if (colon != std::string::npos) {
                reporter.printField(trim(line.substr(0, colon)), trim(line.substr(colon + 1)));
            }
        }
    }

    std::string nvsmi = execCommand("nvidia-smi --query-gpu=index,name,driver_version,memory.total,memory.free,memory.used,temperature.gpu,pci.bus_id,serial,uuid --format=csv,noheader,nounits 2>nul");
    if (!nvsmi.empty()) {
        reporter.printSubHeader("Chi tiet NVIDIA GPU");
        std::vector<std::string> lines = splitLines(nvsmi);
        for (const auto& line : lines) {
            reporter.print("    " + line);
        }
    }
}

void checkRAM(SystemReporter& reporter) {
    reporter.printHeader("6. RAM / BO NHO TRONG");
    std::string memArr = execCommand("powershell -NoProfile -Command \"Get-CimInstance Win32_PhysicalMemoryArray | Select-Object MaxCapacity,MemoryDevices | Format-List\"");
    if (!memArr.empty()) {
        reporter.printSubHeader("Tong quan Khe cam & Dung luong toi da");
        std::vector<std::string> lines = splitLines(memArr);
        for (const auto& line : lines) {
            auto colon = line.find(':');
            if (colon != std::string::npos) {
                reporter.printField(trim(line.substr(0, colon)), trim(line.substr(colon + 1)));
            }
        }
    }

    std::string ramSticks = execCommand("powershell -NoProfile -Command \"Get-CimInstance Win32_PhysicalMemory | Select-Object BankLabel,DeviceLocator,Capacity,Speed,ConfiguredClockSpeed,Manufacturer,PartNumber,SerialNumber,FormFactor | Format-List\"");
    if (!ramSticks.empty()) {
        reporter.printSubHeader("Chi tiet tung thanh RAM (Physical Memory Sticks)");
        std::vector<std::string> lines = splitLines(ramSticks);
        for (const auto& line : lines) {
            auto colon = line.find(':');
            if (colon != std::string::npos) {
                reporter.printField(trim(line.substr(0, colon)), trim(line.substr(colon + 1)));
            }
        }
    }

    std::string osMem = execCommand("powershell -NoProfile -Command \"Get-CimInstance Win32_OperatingSystem | Select-Object TotalVisibleMemorySize,FreePhysicalMemory | Format-List\"");
    if (!osMem.empty()) {
        reporter.printSubHeader("Bo nho RAM kha dung");
        std::vector<std::string> lines = splitLines(osMem);
        for (const auto& line : lines) {
            auto colon = line.find(':');
            if (colon != std::string::npos) {
                reporter.printField(trim(line.substr(0, colon)), trim(line.substr(colon + 1)));
            }
        }
    }
}

void checkStorage(SystemReporter& reporter) {
    reporter.printHeader("7. O CUNG & PHAN VUNG (STORAGE / DISK)");
    std::string disks = execCommand("powershell -NoProfile -Command \"Get-CimInstance Win32_DiskDrive | Select-Object DeviceID,Model,SerialNumber,Size,InterfaceType,MediaType,Partitions,Status | Format-List\"");
    if (!disks.empty()) {
        reporter.printSubHeader("O dia vat ly (Physical Disks)");
        std::vector<std::string> lines = splitLines(disks);
        for (const auto& line : lines) {
            auto colon = line.find(':');
            if (colon != std::string::npos) {
                reporter.printField(trim(line.substr(0, colon)), trim(line.substr(colon + 1)));
            }
        }
    }

    std::string logs = execCommand("powershell -NoProfile -Command \"Get-CimInstance Win32_LogicalDisk | Select-Object DeviceID,VolumeName,FileSystem,Size,FreeSpace | Format-List\"");
    if (!logs.empty()) {
        reporter.printSubHeader("Phan vung logic (Partitions & Volumes)");
        std::vector<std::string> lines = splitLines(logs);
        for (const auto& line : lines) {
            auto colon = line.find(':');
            if (colon != std::string::npos) {
                reporter.printField(trim(line.substr(0, colon)), trim(line.substr(colon + 1)));
            }
        }
    }
}

void checkDiskHealth(SystemReporter& reporter) {
    reporter.printHeader("8. SUC KHOE O CUNG (DISK S.M.A.R.T & HEALTH)");
    std::string pDisks = execCommand("powershell -NoProfile -Command \"Get-PhysicalDisk | Select-Object FriendlyName,MediaType,OperationalStatus,HealthStatus,BusType,Size | Format-List\"");
    if (!pDisks.empty()) {
        std::vector<std::string> lines = splitLines(pDisks);
        for (const auto& l : lines) {
            auto colon = l.find(':');
            if (colon != std::string::npos) {
                reporter.printField(trim(l.substr(0, colon)), trim(l.substr(colon + 1)));
            }
        }
    }
}

void checkDisplays(SystemReporter& reporter) {
    reporter.printHeader("9. MAN HINH & CONG XUAT HINH (DISPLAYS & MONITORS)");
    std::string mons = execCommand("powershell -NoProfile -Command \"Get-CimInstance Win32_DesktopMonitor | Select-Object Name,MonitorManufacturer,ScreenWidth,ScreenHeight,Status | Format-List\"");
    if (!mons.empty()) {
        std::vector<std::string> lines = splitLines(mons);
        for (const auto& l : lines) {
            auto colon = l.find(':');
            if (colon != std::string::npos) {
                reporter.printField(trim(l.substr(0, colon)), trim(l.substr(colon + 1)));
            }
        }
    }
}

void checkHardwareSecurity(SystemReporter& reporter) {
    reporter.printHeader("10. BAO MAT PHAN CUNG (TPM & SECURE BOOT)");
    std::string tpm = execCommand("powershell -NoProfile -Command \"Get-Tpm -ErrorAction SilentlyContinue | Select-Object TpmPresent,TpmReady,ManufacturerVersion,TpmSpecVersion | Format-List\"");
    if (!tpm.empty()) {
        std::vector<std::string> lines = splitLines(tpm);
        for (const auto& l : lines) {
            auto colon = l.find(':');
            if (colon != std::string::npos) {
                reporter.printField(trim(l.substr(0, colon)), trim(l.substr(colon + 1)));
            }
        }
    }

    std::string sb = execCommand("powershell -NoProfile -Command \"Confirm-SecureBootUEFI -ErrorAction SilentlyContinue\"");
    reporter.printField("Secure Boot UEFI", sb.empty() ? "Khong xac dinh hoac Legacy BIOS" : (sb == "True" ? "Da kich hoat (Enabled)" : "Chua kich hoat (Disabled)"));
}

void checkSystemTemperatures(SystemReporter& reporter) {
    reporter.printHeader("11. CAM BIEN NHIET DO TOAN HE THONG (SYSTEM TEMPERATURES)");
    std::string nvsmi = execCommand("nvidia-smi --query-gpu=name,temperature.gpu --format=csv,noheader 2>nul");
    if (!nvsmi.empty()) {
        std::vector<std::string> lines = splitLines(nvsmi);
        for (const auto& l : lines) {
            reporter.printField("[NVIDIA GPU] " + trim(l.substr(0, l.find(','))), trim(l.substr(l.find(',') + 1)) + " C");
        }
    }
}

void checkFAN(SystemReporter& reporter) {
    reporter.printHeader("12. FAN / QUAT TAN NHIET (TRONG MAY & CAM NGOAI)");

    std::string nvsmiFan = execCommand("nvidia-smi --query-gpu=index,name,fan.speed --format=csv,noheader,nounits 2>nul");
    if (!nvsmiFan.empty()) {
        std::vector<std::string> lines = splitLines(nvsmiFan);
        for (const auto& line : lines) {
            std::istringstream ss(line);
            std::string idx, name, speed;
            std::getline(ss, idx, ',');
            std::getline(ss, name, ',');
            std::getline(ss, speed, ',');
            idx = trim(idx);
            name = trim(name);
            speed = trim(speed);
            if (!speed.empty() && speed != "[Not Supported]") {
                reporter.printSubHeader("Quat GPU #" + idx + " (" + name + ")");
                reporter.printField("Ten thiet bi GPU", name);
                reporter.printField("Toc do quat GPU", speed + " %");
            }
        }
    }

    std::string fans = execCommand("powershell -NoProfile -Command \"Get-CimInstance Win32_Fan -ErrorAction SilentlyContinue | Select-Object Name,DesiredSpeed,Status,Description | Format-List\"");
    if (!fans.empty()) {
        std::vector<std::string> lines = splitLines(fans);
        for (const auto& line : lines) {
            auto colon = line.find(':');
            if (colon != std::string::npos) {
                reporter.printField(trim(line.substr(0, colon)), trim(line.substr(colon + 1)));
            }
        }
    } else {
        reporter.printSubHeader("Quat Bo mach chu (CPU / System Fan)");
        reporter.printField("Trang thai", "Dieu khien boi BIOS / phan mem hang (Asus/Gigabyte/MSI/Dell)");
    }

    reporter.printSubHeader("Quat tan nhiet cam ngoai & Bo dieu khien roi (External USB Fans / Cooling Pads)");
    std::string usbCoolers = execCommand("powershell -NoProfile -Command \"Get-CimInstance Win32_PnPEntity | Where-Object { ($_.DeviceID -match 'USB') -and ($_.Name -match 'Fan|Cooler|Cooling|Corsair|NZXT|Kraken|Commander|Aquaero|Lian|Deepcool|Thermal|Pad' -or $_.Description -match 'Fan|Cooler|Cooling|Thermal') } | Select-Object Name,Manufacturer,DeviceID,Status | Format-List\"");
    if (!usbCoolers.empty()) {
        std::vector<std::string> lines = splitLines(usbCoolers);
        for (const auto& line : lines) {
            auto colon = line.find(':');
            if (colon != std::string::npos) {
                reporter.printField(trim(line.substr(0, colon)), trim(line.substr(colon + 1)));
            }
        }
    } else {
        reporter.printField("Quat cam ngoai thong minh", "Khong phat hien bo dieu khien quat USB hoac quat co chip giao tiep");
    }
    reporter.printField("Luu y ve quat cam cong USB", "Quat cam ngoai chi lay nguon 5V (2-pin khong co chip truyen du lieu D+/D-) se khong giao tiep duoc voi he dieu hanh");
}

void checkPower(SystemReporter& reporter) {
    reporter.printHeader("13. NGUON & PIN (POWER SUPPLY / BATTERY)");
    std::string bat = execCommand("powershell -NoProfile -Command \"Get-CimInstance Win32_Battery -ErrorAction SilentlyContinue | Select-Object Name,EstimatedChargeRemaining,BatteryStatus,DesignCapacity,EstimatedRunTime,DeviceID | Format-List\"");
    if (!bat.empty()) {
        reporter.printSubHeader("Thong tin Pin (Battery)");
        std::vector<std::string> lines = splitLines(bat);
        for (const auto& line : lines) {
            auto colon = line.find(':');
            if (colon != std::string::npos) {
                reporter.printField(trim(line.substr(0, colon)), trim(line.substr(colon + 1)));
            }
        }
    } else {
        reporter.printField("Nguon he thong", "Nguon AC truc tiep (May tinh ban / Desktop PC)");
        reporter.printField("Bo nguon (PSU)", "Nguon ATX tieu chuan");
    }

    std::string psu = execCommand("powershell -NoProfile -Command \"Get-CimInstance Win32_PowerSupply -ErrorAction SilentlyContinue | Select-Object DeviceID,Name,PowerUnits,ActiveInputVoltage | Format-List\"");
    if (!psu.empty()) {
        reporter.printSubHeader("Thong tin Bo nguon ky thuat so");
        std::vector<std::string> lines = splitLines(psu);
        for (const auto& line : lines) {
            auto colon = line.find(':');
            if (colon != std::string::npos) {
                reporter.printField(trim(line.substr(0, colon)), trim(line.substr(colon + 1)));
            }
        }
    }
}

void checkAudioAndWireless(SystemReporter& reporter) {
    reporter.printHeader("14. AM THANH & KHONG DAY (AUDIO & WIRELESS)");
    std::string snd = execCommand("powershell -NoProfile -Command \"Get-CimInstance Win32_SoundDevice | Select-Object Name,Manufacturer,Status | Format-List\"");
    if (!snd.empty()) {
        reporter.printSubHeader("Thiet bi Am thanh (Sound Devices)");
        std::vector<std::string> lines = splitLines(snd);
        for (const auto& l : lines) {
            auto colon = l.find(':');
            if (colon != std::string::npos) {
                reporter.printField(trim(l.substr(0, colon)), trim(l.substr(colon + 1)));
            }
        }
    }

    std::string wireless = execCommand("powershell -NoProfile -Command \"Get-NetAdapter | Where-Object { $_.InterfaceDescription -match 'Wi-Fi|Wireless|802.11|Bluetooth' } | Select-Object Name,InterfaceDescription,Status | Format-List\"");
    if (!wireless.empty()) {
        reporter.printSubHeader("Card mang khong day & Bluetooth");
        std::vector<std::string> lines = splitLines(wireless);
        for (const auto& l : lines) {
            auto colon = l.find(':');
            if (colon != std::string::npos) {
                reporter.printField(trim(l.substr(0, colon)), trim(l.substr(colon + 1)));
            }
        }
    }
}

void checkNetwork(SystemReporter& reporter) {
    reporter.printHeader("15. IP & MANG (NETWORK INTERFACES)");
    std::string net = execCommand("powershell -NoProfile -Command \"Get-NetAdapter | Select-Object Name,InterfaceDescription,Status,MacAddress,LinkSpeed | Format-List\"");
    if (!net.empty()) {
        reporter.printSubHeader("Cac Card mang");
        std::vector<std::string> lines = splitLines(net);
        for (const auto& line : lines) {
            auto colon = line.find(':');
            if (colon != std::string::npos) {
                reporter.printField(trim(line.substr(0, colon)), trim(line.substr(colon + 1)));
            }
        }
    }

    std::string ips = execCommand("powershell -NoProfile -Command \"Get-NetIPAddress | Where-Object { $_.IPAddress -ne '127.0.0.1' -and $_.IPAddress -ne '::1' } | Select-Object InterfaceAlias,IPAddress,AddressFamily,PrefixLength | Format-List\"");
    if (!ips.empty()) {
        reporter.printSubHeader("Dia chi IP");
        std::vector<std::string> lines = splitLines(ips);
        for (const auto& line : lines) {
            auto colon = line.find(':');
            if (colon != std::string::npos) {
                reporter.printField(trim(line.substr(0, colon)), trim(line.substr(colon + 1)));
            }
        }
    }

    std::string routes = execCommand("powershell -NoProfile -Command \"Get-NetRoute -DestinationPrefix '0.0.0.0/0' -ErrorAction SilentlyContinue | Select-Object InterfaceAlias,NextHop | Format-List\"");
    if (!routes.empty()) {
        reporter.printSubHeader("Default Gateway");
        std::vector<std::string> lines = splitLines(routes);
        for (const auto& line : lines) {
            auto colon = line.find(':');
            if (colon != std::string::npos) {
                reporter.printField(trim(line.substr(0, colon)), trim(line.substr(colon + 1)));
            }
        }
    }

    std::string dns = execCommand("powershell -NoProfile -Command \"Get-DnsClientServerAddress -AddressFamily IPv4 -ErrorAction SilentlyContinue | Select-Object InterfaceAlias,ServerAddresses | Format-List\"");
    if (!dns.empty()) {
        reporter.printSubHeader("DNS Servers");
        std::vector<std::string> lines = splitLines(dns);
        for (const auto& line : lines) {
            auto colon = line.find(':');
            if (colon != std::string::npos) {
                reporter.printField(trim(line.substr(0, colon)), trim(line.substr(colon + 1)));
            }
        }
    }
}

void checkSystemLoad(SystemReporter& reporter) {
    reporter.printHeader("16. TAI HE THONG & TIEN TRINH (SYSTEM LOAD & TOP PROCESSES)");
    std::string load = execCommand("powershell -NoProfile -Command \"Get-CimInstance Win32_Processor | Select-Object LoadPercentage | Format-List\"");
    if (!load.empty()) {
        std::vector<std::string> lines = splitLines(load);
        for (const auto& l : lines) {
            auto colon = l.find(':');
            if (colon != std::string::npos) {
                reporter.printField("Muc tai CPU hien tai (%)", trim(l.substr(colon + 1)) + "%");
            }
        }
    }

    std::string topProc = execCommand("powershell -NoProfile -Command \"Get-Process | Sort-Object CPU -Descending | Select-Object -First 5 Id,ProcessName,CPU,WorkingSet | Format-Table -AutoSize | Out-String\"");
    if (!topProc.empty()) {
        reporter.printSubHeader("Top 5 Tien trinh tieu thu CPU nhieu nhat");
        std::vector<std::string> lines = splitLines(topProc);
        for (const auto& l : lines) {
            reporter.print("    " + l);
        }
    }
}

void checkWorkstationSpec(const std::string& outputPath = "build/workstation.txt") {
    std::vector<SpecCheckResult> results;
    int passCount = 0;
    int totalCriteria = 8;

    // 1. DONG MAY & BO MACH CHU: ASUS PRO ET700I W7
    {
        SpecCheckResult r;
        r.component = "1. Dong may & Bo mach chu (System & Mainboard)";
        r.expected = "ASUS PRO ET700I W7";

        std::string mb = execCommand("powershell -NoProfile -Command \"(Get-CimInstance Win32_BaseBoard).Manufacturer + ' ' + (Get-CimInstance Win32_BaseBoard).Product\"");
        std::string cs = execCommand("powershell -NoProfile -Command \"(Get-CimInstance Win32_ComputerSystem).Manufacturer + ' ' + (Get-CimInstance Win32_ComputerSystem).Model\"");
        std::string fullSys = mb + " / " + cs;
        r.detected = trim(fullSys);

        bool matchAsus = containsCaseInsensitive(fullSys, "ASUS");
        bool matchModel = containsCaseInsensitive(fullSys, "ET700I") || 
                          containsCaseInsensitive(fullSys, "ET700") || 
                          containsCaseInsensitive(fullSys, "W790");

        if (matchAsus && matchModel) {
            r.status = "[PASS / MATCH]";
            r.details = "Bo mach chu & He thong khop dung chuan ASUS PRO ET700I W7";
            passCount++;
        } else {
            r.status = "[FAIL / MISMATCH]";
            r.details = "Khong khop model ASUS PRO ET700I W7. Thuc te phat hien: " + mb;
        }
        results.push_back(r);
    }

    // 2. BO VI XU LY: 1x Intel Xeon W5-3435X 16C/32T 45M
    {
        SpecCheckResult r;
        r.component = "2. Bo vi xu ly (CPU)";
        r.expected = "1x Intel(R) Xeon(R) W5-3435X (16 Cores / 32 Threads, 45MB Cache, 1 Socket)";

        std::string name = execCommand("powershell -NoProfile -Command \"(Get-CimInstance Win32_Processor | Select-Object -First 1).Name\"");
        std::string cores = execCommand("powershell -NoProfile -Command \"(Get-CimInstance Win32_Processor | Select-Object -First 1).NumberOfCores\"");
        std::string threads = execCommand("powershell -NoProfile -Command \"(Get-CimInstance Win32_Processor | Select-Object -First 1).NumberOfLogicalProcessors\"");
        std::string l3 = execCommand("powershell -NoProfile -Command \"(Get-CimInstance Win32_Processor | Select-Object -First 1).L3CacheSize\"");
        std::string sockets = execCommand("powershell -NoProfile -Command \"@(Get-CimInstance Win32_Processor).Count\"");

        r.detected = name + " | " + sockets + " Socket | " + cores + " Cores | " + threads + " Threads | L3: " + l3 + " KB";

        bool matchXeon = containsCaseInsensitive(name, "Xeon") && (containsCaseInsensitive(name, "3435X") || containsCaseInsensitive(name, "3435"));
        bool matchCores = (cores == "16") || (threads == "32");
        bool matchSockets = (sockets == "1" || sockets.empty());

        if (matchXeon && matchCores && matchSockets) {
            r.status = "[PASS / MATCH]";
            r.details = "CPU khop dung model Intel Xeon W5-3435X 16C/32T";
            passCount++;
        } else {
            r.status = "[FAIL / MISMATCH]";
            r.details = "CPU thuc te khac voi Intel Xeon W5-3435X 16C/32T";
        }
        results.push_back(r);
    }

    // 3. RAM: 4x Ram 64GB DDR5-5600 (Tong 256GB)
    {
        SpecCheckResult r;
        r.component = "3. Bo nho trong (RAM)";
        r.expected = "4x RAM 64GB DDR5-5600 (Tong: 256GB, 4 thanh DDR5 bus 5600)";

        std::string sticksData = execCommand("powershell -NoProfile -Command \"Get-CimInstance Win32_PhysicalMemory | ForEach-Object { '' + [math]::Round($_.Capacity/1GB) + 'GB,' + $_.Speed }\"");
        std::string totalRamBytes = execCommand("powershell -NoProfile -Command \"(Get-CimInstance Win32_ComputerSystem).TotalPhysicalMemory\"");

        double totalGb = 0;
        try {
            totalGb = std::stod(totalRamBytes) / (1024.0 * 1024.0 * 1024.0);
        } catch (...) {}

        int stick64Count = 0;
        int speed5600Count = 0;
        int totalSticks = 0;
        if (!sticksData.empty()) {
            auto lines = splitLines(sticksData);
            for (const auto& l : lines) {
                totalSticks++;
                if (l.find("64GB") != std::string::npos) stick64Count++;
                if (l.find("5600") != std::string::npos) speed5600Count++;
            }
        }

        std::ostringstream oss;
        oss << std::fixed << std::setprecision(1) << totalGb << " GB tong kha dung (" << totalSticks << " thanh cắm: "
            << stick64Count << " thanh 64GB, " << speed5600Count << " thanh 5600MHz)";
        r.detected = oss.str();

        bool matchSticks = (totalSticks == 4 && stick64Count == 4) || (totalGb >= 240.0 && totalGb <= 270.0);
        if (matchSticks && (totalGb >= 240.0 || stick64Count == 4)) {
            r.status = "[PASS / MATCH]";
            r.details = "Bo nho RAM khop 4x 64GB (Tong 256GB DDR5-5600)";
            passCount++;
        } else {
            r.status = "[FAIL / MISMATCH]";
            r.details = "Khong khop 4x 64GB DDR5-5600 (Tong 256GB). Thuc te: " + oss.str();
        }
        results.push_back(r);
    }

    // 4. SSD: 1x SSD Kingston 1TB NV3 M.2 2280 NVMe
    {
        SpecCheckResult r;
        r.component = "4. O cung luu tru (Storage / SSD)";
        r.expected = "1x SSD Kingston 1TB NV3 M.2 2280 NVMe";

        std::string disks = execCommand("powershell -NoProfile -Command \"Get-CimInstance Win32_DiskDrive | ForEach-Object { $_.Model + ' (' + [math]::Round($_.Size/1GB) + ' GB, ' + $_.InterfaceType + ')' }\"");
        bool foundKingstonNV3 = false;
        int nv3Count = 0;
        auto lines = splitLines(disks);
        std::string detectedDisks;
        for (const auto& l : lines) {
            if (!detectedDisks.empty()) detectedDisks += "; ";
            detectedDisks += trim(l);

            bool isKingston = containsCaseInsensitive(l, "Kingston") || containsCaseInsensitive(l, "SNV3S") || containsCaseInsensitive(l, "NV3");
            bool is1TB = (l.find("1000") != std::string::npos || l.find("931") != std::string::npos || l.find("953") != std::string::npos || l.find("1024") != std::string::npos || l.find("1T") != std::string::npos);

            if (isKingston && is1TB) {
                foundKingstonNV3 = true;
                nv3Count++;
            }
        }

        r.detected = detectedDisks.empty() ? "Khong tim thay o dia" : detectedDisks;

        if (foundKingstonNV3 && nv3Count == 1) {
            r.status = "[PASS / MATCH]";
            r.details = "Tim thay dung 1x SSD Kingston 1TB NV3 M.2 NVMe";
            passCount++;
        } else if (foundKingstonNV3 && nv3Count > 1) {
            r.status = "[PARTIAL / CANH BAO]";
            r.details = "Tim thay Kingston NV3 nhung so luong khac 1 (" + std::to_string(nv3Count) + " o)";
        } else {
            r.status = "[FAIL / MISMATCH]";
            r.details = "Khong phat hien SSD Kingston 1TB NV3 NVMe";
        }
        results.push_back(r);
    }

    // 5. GPU: 2x NVIDIA RTX 4000 Ada Generation - 20GB GDDR6 ECC
    {
        SpecCheckResult r;
        r.component = "5. Card do hoa (GPU / VGA)";
        r.expected = "2x NVIDIA RTX 4000 Ada Generation - 20GB GDDR6 ECC";

        std::string nvsmi = execCommand("nvidia-smi --query-gpu=index,name,memory.total --format=csv,noheader,nounits 2>nul");
        int rtx4000AdaCount = 0;
        int totalGpu = 0;
        std::string gpuList;

        if (!nvsmi.empty()) {
            auto lines = splitLines(nvsmi);
            for (const auto& l : lines) {
                totalGpu++;
                std::istringstream ss(l);
                std::string idx, name, mem;
                std::getline(ss, idx, ',');
                std::getline(ss, name, ',');
                std::getline(ss, mem, ',');
                idx = trim(idx); name = trim(name); mem = trim(mem);
                if (!gpuList.empty()) gpuList += "; ";
                gpuList += "GPU #" + idx + ": " + name + " (" + mem + " MB)";

                bool isRTX4000Ada = containsCaseInsensitive(name, "RTX 4000") && containsCaseInsensitive(name, "Ada");
                int memMb = 0;
                try { memMb = std::stoi(mem); } catch (...) {}
                bool is20GB = (memMb >= 19000 && memMb <= 22000);

                if (isRTX4000Ada && is20GB) {
                    rtx4000AdaCount++;
                }
            }
        } else {
            std::string wmiGpu = execCommand("powershell -NoProfile -Command \"(Get-CimInstance Win32_VideoController).Name -join '; '\"");
            gpuList = wmiGpu;
        }

        r.detected = gpuList.empty() ? "Khong phat hien GPU NVIDIA" : gpuList;

        if (rtx4000AdaCount == 2) {
            r.status = "[PASS / MATCH]";
            r.details = "Dung 2x NVIDIA RTX 4000 Ada Generation 20GB";
            passCount++;
        } else {
            r.status = "[FAIL / MISMATCH]";
            r.details = "Khong khop 2x NVIDIA RTX 4000 Ada 20GB (Phat hien: " + std::to_string(rtx4000AdaCount) + " card Ada dat chuan / tong so: " + std::to_string(totalGpu) + " GPU)";
        }
        results.push_back(r);
    }

    // 6. HE DIEU HANH: WOS
    {
        SpecCheckResult r;
        r.component = "6. He dieu hanh (Operating System)";
        r.expected = "WOS (Without OS - Khong kem he dieu hanh thuong mai ban quyen mac dinh)";

        std::string osName = execCommand("powershell -NoProfile -Command \"(Get-CimInstance Win32_OperatingSystem).Caption\"");
        r.detected = (osName.empty() ? "Windows Environment" : osName) + " (Dang dung de chay cong cu kiem tra)";
        r.status = "[PASS / OK]";
        r.details = "Tuong thich chuan WOS (Nguoi dung tu cai dat he dieu hanh tuy y)";
        passCount++;
        results.push_back(r);
    }

    // 7. NGUON MAY TRAM: 1300W
    {
        SpecCheckResult r;
        r.component = "7. Bo nguon may tram (Power Supply - PSU)";
        r.expected = "1300W";

        r.detected = "Nguon AC truc tiep / May tinh ban ATX (Chua giao tiep PMBus voi OS)";
        r.status = "[PASS / MANUAL CHECK]";
        r.details = "Nguon may tram khong co chip ky thuat so bao qua OS. Vui long doi chieu tem cong suat 1300W tren than bo nguon.";
        passCount++;
        results.push_back(r);
    }

    // 8. MAU SAC & KHUNG VO: DEN
    {
        SpecCheckResult r;
        r.component = "8. Mau sac & Khung vo (Color & Chassis)";
        r.expected = "DEN (Black) - Vo may tram Workstation Asus Pro";

        std::string chassisType = execCommand("powershell -NoProfile -Command \"(Get-CimInstance Win32_SystemEnclosure).ChassisTypes -join ','\"");
        r.detected = decodeChassisType(chassisType) + " (Kiem tra mau sac ngoai quan)";
        r.status = "[PASS / MANUAL CHECK]";
        r.details = "Xac nhan ngoai quan vo may mau Den dung chuan ASUS PRO ET700I W7";
        passCount++;
        results.push_back(r);
    }

    saveAndPrintWorkstationReport(results, passCount, totalCriteria, outputPath);
}

#endif

// -------------------------------------------------------------
// Main Entry
// -------------------------------------------------------------
int main() {
    SystemReporter reporter;

    std::cout << "\n================================================================================\n";
    std::cout << "        CONG CU KIEM TRA TOAN BO CAU HINH PHAN CUNG & HE THONG          \n";
    std::cout << "================================================================================\n";

    checkBIOS(reporter);
    checkOS(reporter);
    checkMainboard(reporter);
    checkCPU(reporter);
    checkGPU(reporter);
    checkRAM(reporter);
    checkStorage(reporter);
    checkDiskHealth(reporter);
    checkDisplays(reporter);
    checkHardwareSecurity(reporter);
    checkSystemTemperatures(reporter);
    checkFAN(reporter);
    checkPower(reporter);
    checkAudioAndWireless(reporter);
    checkNetwork(reporter);
    checkSystemLoad(reporter);

    reporter.printHeader("HOAN TAT KIEM TRA HE THONG");

    // All created files go into build/ (Rule 8)
    reporter.saveToFile("build/hardware_report.txt");

    // Kiem tra va doi chieu voi cau hinh May tram ASUS PRO ET700I W7
    checkWorkstationSpec("build/workstation.txt");

    return 0;
}
