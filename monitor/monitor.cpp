#include "monitor.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <fstream>
#include <ctime>
#include <iomanip>
#include <vector>
#include <algorithm>
#include <string>
#include <windows.h>
#include <pdh.h>
#include <psapi.h>
#include <tlhelp32.h>
#include <winreg.h>

#pragma comment(lib, "pdh.lib")
#pragma comment(lib, "psapi.lib")
#pragma comment(lib, "advapi32.lib")

std::atomic<double> g_cpu{ 0.0 };
std::atomic<double> g_ramP{ 0.0 };
std::atomic<double> g_ramU{ 0.0 };
std::atomic<double> g_ramT{ 0.0 };
std::atomic<double> g_diskActive{ 0.0 };
std::atomic<double> g_diskRead{ 0.0 };
std::atomic<double> g_diskWrite{ 0.0 };
std::atomic<unsigned int> g_gpu{ 0 };
std::atomic<unsigned int> g_gpuTemp{ 0 };
std::atomic<double> g_vramU{ 0.0 };
std::atomic<double> g_vramT{ 0.0 };
std::atomic<bool> g_gpuOk{ false };
std::atomic<bool> g_running{ true };

std::atomic<double> g_netSent{ 0.0 };
std::atomic<double> g_netRecv{ 0.0 };

std::atomic<int> g_batteryPercent{ -1 };
std::atomic<bool> g_isCharging{ false };

std::atomic<double> g_fps{ 0.0 };
std::atomic<bool> g_fpsEnabled{ false };

std::atomic<bool> g_cpuAlerted{ false };
std::atomic<bool> g_ramAlerted{ false };
std::atomic<bool> g_gpuAlerted{ false };

const double CPU_WARNING = 85.0;
const double RAM_WARNING = 90.0;
const unsigned int GPU_WARNING = 85;

void SystemMonitorThread() {
    PDH_HQUERY query;
    PDH_HCOUNTER cCpu, cDiskA, cDiskR, cDiskW, cNetSent, cNetRecv;

    PdhOpenQuery(NULL, 0, &query);
    PdhAddEnglishCounterA(query, "\\Processor Information(_Total)\\% Processor Utility", 0, &cCpu);
    PdhAddEnglishCounterA(query, "\\PhysicalDisk(_Total)\\% Disk Time", 0, &cDiskA);
    PdhAddEnglishCounterA(query, "\\PhysicalDisk(_Total)\\Disk Read Bytes/sec", 0, &cDiskR);
    PdhAddEnglishCounterA(query, "\\PhysicalDisk(_Total)\\Disk Write Bytes/sec", 0, &cDiskW);
    PdhAddEnglishCounterA(query, "\\Network Interface(*)\\Bytes Sent/sec", 0, &cNetSent);
    PdhAddEnglishCounterA(query, "\\Network Interface(*)\\Bytes Received/sec", 0, &cNetRecv);
    PdhCollectQueryData(query);

    while (g_running) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        PdhCollectQueryData(query);

        PDH_FMT_COUNTERVALUE vCpu, vDiskA, vDiskR, vDiskW, vNetSent, vNetRecv;
        PdhGetFormattedCounterValue(cCpu, PDH_FMT_DOUBLE, NULL, &vCpu);
        PdhGetFormattedCounterValue(cDiskA, PDH_FMT_DOUBLE, NULL, &vDiskA);
        PdhGetFormattedCounterValue(cDiskR, PDH_FMT_DOUBLE, NULL, &vDiskR);
        PdhGetFormattedCounterValue(cDiskW, PDH_FMT_DOUBLE, NULL, &vDiskW);
        PdhGetFormattedCounterValue(cNetSent, PDH_FMT_DOUBLE, NULL, &vNetSent);
        PdhGetFormattedCounterValue(cNetRecv, PDH_FMT_DOUBLE, NULL, &vNetRecv);

        g_cpu.store(vCpu.doubleValue);
        g_diskActive.store(vDiskA.doubleValue > 100.0 ? 100.0 : vDiskA.doubleValue);
        g_diskRead.store(vDiskR.doubleValue / 1024.0 / 1024.0);
        g_diskWrite.store(vDiskW.doubleValue / 1024.0 / 1024.0);
        g_netSent.store(vNetSent.doubleValue / 1024.0 / 1024.0);
        g_netRecv.store(vNetRecv.doubleValue / 1024.0 / 1024.0);

        MEMORYSTATUSEX mem;
        mem.dwLength = sizeof(mem);
        GlobalMemoryStatusEx(&mem);
        g_ramT.store(mem.ullTotalPhys / 1024.0 / 1024.0 / 1024.0);
        g_ramU.store(g_ramT.load() - (mem.ullAvailPhys / 1024.0 / 1024.0 / 1024.0));
        g_ramP.store((g_ramU.load() / g_ramT.load()) * 100.0);

        SYSTEM_POWER_STATUS powerStatus;
        if (GetSystemPowerStatus(&powerStatus)) {
            g_batteryPercent.store(powerStatus.BatteryLifePercent);
            g_isCharging.store(powerStatus.ACLineStatus == 1);
        }
    }
    PdhCloseQuery(query);
}

void GpuMonitorThread() {
    HMODULE h = LoadLibraryA("nvml.dll");
    if (!h) h = LoadLibraryA("C:\\Windows\\System32\\DriverStore\\FileRepository\\nv_dispi.inf_amd64_eb90bba13149c063\\nvml.dll");
    if (!h) {
        g_gpuOk.store(false);
        return;
    }

    auto init = (int(*)(...))GetProcAddress(h, "nvmlInit");
    auto getHandle = (int(*)(...))GetProcAddress(h, "nvmlDeviceGetHandleByIndex");
    auto getUtil = (int(*)(...))GetProcAddress(h, "nvmlDeviceGetUtilizationRates");
    auto getMem = (int(*)(...))GetProcAddress(h, "nvmlDeviceGetMemoryInfo");
    auto getTemp = (int(*)(...))GetProcAddress(h, "nvmlDeviceGetTemperature");

    void* dev;
    if (init && init() == 0 && getHandle && getHandle(0, &dev) == 0) {
        g_gpuOk.store(true);
        while (g_running) {
            unsigned int util[2] = { 0 }, temp = 0;
            unsigned long long mem[3] = { 0 };

            if (getUtil) getUtil(dev, &util);
            if (getMem) getMem(dev, &mem);
            if (getTemp) getTemp(dev, 0, &temp);

            g_gpu.store(util[0]);
            g_vramT.store(mem[0] / 1024.0 / 1024.0 / 1024.0);
            g_vramU.store(mem[2] / 1024.0 / 1024.0 / 1024.0);
            g_gpuTemp.store(temp);

            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
    FreeLibrary(h);
}

std::vector<ProcessInfo> GetTopProcesses(int count) {
    std::vector<ProcessInfo> processes;
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) return processes;

    PROCESSENTRY32W pe;
    pe.dwSize = sizeof(PROCESSENTRY32W);

    if (Process32FirstW(hSnapshot, &pe)) {
        do {
            HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pe.th32ProcessID);
            if (hProcess) {
                PROCESS_MEMORY_COUNTERS pmc;
                if (GetProcessMemoryInfo(hProcess, &pmc, sizeof(pmc))) {
                    FILETIME createTime, exitTime, kernelTime, userTime;
                    if (GetProcessTimes(hProcess, &createTime, &exitTime, &kernelTime, &userTime)) {
                        ULARGE_INTEGER kt, ut;
                        kt.LowPart = kernelTime.dwLowDateTime;
                        kt.HighPart = kernelTime.dwHighDateTime;
                        ut.LowPart = userTime.dwLowDateTime;
                        ut.HighPart = userTime.dwHighDateTime;
                        double cpu = (kt.QuadPart + ut.QuadPart) / 10000.0;

                        ProcessInfo info;
                        info.name = pe.szExeFile;
                        info.cpuUsage = cpu;
                        info.memoryUsage = pmc.WorkingSetSize / 1024 / 1024;
                        processes.push_back(info);
                    }
                }
                CloseHandle(hProcess);
            }
        } while (Process32NextW(hSnapshot, &pe));
    }

    CloseHandle(hSnapshot);

    std::sort(processes.begin(), processes.end(),
        [](const ProcessInfo& a, const ProcessInfo& b) {
            return a.cpuUsage > b.cpuUsage;
        });

    if (processes.size() > count) {
        processes.resize(count);
    }

    return processes;
}

FPSMonitor::FPSMonitor() : frameCount(0), fps(0.0) {
    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&lastTime);
    g_fpsEnabled.store(true);
}

void FPSMonitor::Frame() {
    LARGE_INTEGER currentTime;
    QueryPerformanceCounter(&currentTime);
    frameCount++;

    double deltaTime = (double)(currentTime.QuadPart - lastTime.QuadPart) / frequency.QuadPart;
    if (deltaTime >= 1.0) {
        fps = frameCount / deltaTime;
        frameCount = 0;
        lastTime = currentTime;
        g_fps.store(fps);
    }
}

void AlertThread() {
    while (g_running) {
        std::this_thread::sleep_for(std::chrono::seconds(2));

        double cpu = g_cpu.load();
        double ram = g_ramP.load();
        unsigned int gpu = g_gpu.load();

        if (cpu < CPU_WARNING - 10) g_cpuAlerted.store(false);
        if (ram < RAM_WARNING - 10) g_ramAlerted.store(false);
        if (gpu < GPU_WARNING - 10) g_gpuAlerted.store(false);

        if (cpu > CPU_WARNING && !g_cpuAlerted.load()) {
            std::string msg = "HIGH CPU USAGE: " + std::to_string(cpu) + "%\n";
            std::cout << "\a" << msg;
            g_cpuAlerted.store(true);
            MessageBoxA(NULL, msg.c_str(), "System Monitor Alert", MB_OK | MB_ICONWARNING);
        }

        if (ram > RAM_WARNING && !g_ramAlerted.load()) {
            std::string msg = "HIGH RAM USAGE: " + std::to_string(ram) + "%\n";
            std::cout << "\a" << msg;
            g_ramAlerted.store(true);
            MessageBoxA(NULL, msg.c_str(), "System Monitor Alert", MB_OK | MB_ICONWARNING);
        }

        if (g_gpuOk.load() && gpu > GPU_WARNING && !g_gpuAlerted.load()) {
            std::string msg = "HIGH GPU USAGE: " + std::to_string(gpu) + "%\n";
            std::cout << "\a" << msg;
            g_gpuAlerted.store(true);
            MessageBoxA(NULL, msg.c_str(), "System Monitor Alert", MB_OK | MB_ICONWARNING);
        }
    }
}

void FPSTestThread() {
    FPSMonitor fpsMonitor;
    while (g_running) {
        fpsMonitor.Frame();
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }
}

void PrintHeader() {
    std::cout << "\n========================================\n";
    std::cout << "       SYSTEM MONITOR\n";
    std::cout << "========================================\n";
}

void PrintCPU() {
    std::cout << "\nCPU: " << std::fixed << std::setprecision(1) << g_cpu.load() << "%";
    if (g_cpu.load() > CPU_WARNING) {
        std::cout << " [!HIGH!]";
    }
    std::cout << std::endl;
}

void PrintRAM() {
    std::cout << "RAM: " << std::fixed << std::setprecision(1) << g_ramP.load() << "%";
    if (g_ramP.load() > RAM_WARNING) {
        std::cout << " [!HIGH!]";
    }
    std::cout << " | Used: " << std::fixed << std::setprecision(1) << g_ramU.load()
        << "GB / " << std::fixed << std::setprecision(1) << g_ramT.load() << "GB\n";
}

void PrintDisk() {
    std::cout << "DISK: Active " << std::fixed << std::setprecision(1) << g_diskActive.load() << "%"
        << " | R: " << std::fixed << std::setprecision(1) << g_diskRead.load() << "MB/s"
        << " | W: " << std::fixed << std::setprecision(1) << g_diskWrite.load() << "MB/s\n";
}

void PrintNetwork() {
    std::cout << "NET: Sent " << std::fixed << std::setprecision(2) << g_netSent.load() << "MB/s"
        << " | Recv " << std::fixed << std::setprecision(2) << g_netRecv.load() << "MB/s\n";
}

void PrintGPU() {
    if (g_gpuOk.load()) {
        std::cout << "GPU: " << g_gpu.load() << "%";
        if (g_gpu.load() > GPU_WARNING) {
            std::cout << " [!HIGH!]";
        }
        std::cout << " | Temp: " << g_gpuTemp.load() << "C"
            << " | VRAM: " << std::fixed << std::setprecision(1) << g_vramU.load()
            << "GB / " << std::fixed << std::setprecision(1) << g_vramT.load() << "GB\n";
    }
    else {
        std::cout << "GPU: Not detected\n";
    }
}

void PrintProcesses() {
    std::cout << "\nTop processes:\n";
    auto procs = GetTopProcesses(5);
    if (procs.empty()) {
        std::cout << "  (none)\n";
    }
    else {
        for (size_t i = 0; i < procs.size(); i++) {
            std::wstring wname = procs[i].name;
            std::string name(wname.begin(), wname.end());
            std::cout << "  " << i + 1 << ". " << name
                << " (CPU: " << std::fixed << std::setprecision(0) << procs[i].cpuUsage << "ms"
                << " | RAM: " << procs[i].memoryUsage << "MB)\n";
        }
    }
}

void PrintBattery() {
    if (g_batteryPercent.load() >= 0 && g_batteryPercent.load() <= 100) {
        std::cout << "BATTERY: " << g_batteryPercent.load() << "%";
        if (g_isCharging.load()) {
            std::cout << " (charging)";
        }
        std::cout << "\n";
    }
}

void PrintFPS() {
    if (g_fpsEnabled.load()) {
        std::cout << "FPS: " << std::fixed << std::setprecision(1) << g_fps.load() << "\n";
    }
}

void PrintFooter() {
    std::cout << "\nPress Ctrl+C to exit\n";
    std::cout << "----------------------------------------\n";
}