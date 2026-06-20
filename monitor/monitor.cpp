#include <windows.h>
#include <pdh.h>
#include <iostream>
#include <thread>
#include <chrono>
#include <atomic>

#pragma comment(lib, "pdh.lib")

std::atomic<double> g_cpu{ 0.0 }, g_ramP{ 0.0 }, g_ramU{ 0.0 }, g_ramT{ 0.0 };
std::atomic<double> g_diskActive{ 0.0 }, g_diskRead{ 0.0 }, g_diskWrite{ 0.0 };
std::atomic<unsigned int> g_gpu{ 0 }, g_gpuTemp{ 0 };
std::atomic<double> g_vramU{ 0.0 }, g_vramT{ 0.0 };
std::atomic<bool> g_gpuOk{ false }, g_running{ true };

void SystemMonitorThread() {
    PDH_HQUERY query;
    PDH_HCOUNTER cCpu, cDiskA, cDiskR, cDiskW;

    PdhOpenQuery(NULL, 0, &query);
    PdhAddEnglishCounterA(query, "\\Processor Information(_Total)\\% Processor Utility", 0, &cCpu);
    PdhAddEnglishCounterA(query, "\\PhysicalDisk(_Total)\\% Disk Time", 0, &cDiskA);
    PdhAddEnglishCounterA(query, "\\PhysicalDisk(_Total)\\Disk Read Bytes/sec", 0, &cDiskR);
    PdhAddEnglishCounterA(query, "\\PhysicalDisk(_Total)\\Disk Write Bytes/sec", 0, &cDiskW);
    PdhCollectQueryData(query);

    while (g_running) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        PdhCollectQueryData(query);

        PDH_FMT_COUNTERVALUE vCpu, vDiskA, vDiskR, vDiskW;
        PdhGetFormattedCounterValue(cCpu, PDH_FMT_DOUBLE, NULL, &vCpu);
        PdhGetFormattedCounterValue(cDiskA, PDH_FMT_DOUBLE, NULL, &vDiskA);
        PdhGetFormattedCounterValue(cDiskR, PDH_FMT_DOUBLE, NULL, &vDiskR);
        PdhGetFormattedCounterValue(cDiskW, PDH_FMT_DOUBLE, NULL, &vDiskW);

        g_cpu.store(vCpu.doubleValue);
        g_diskActive.store(vDiskA.doubleValue > 100.0 ? 100.0 : vDiskA.doubleValue);
        g_diskRead.store(vDiskR.doubleValue / 1024.0 / 1024.0);
        g_diskWrite.store(vDiskW.doubleValue / 1024.0 / 1024.0);

        MEMORYSTATUSEX mem; mem.dwLength = sizeof(mem);
        GlobalMemoryStatusEx(&mem);
        g_ramT.store(mem.ullTotalPhys / 1024.0 / 1024.0 / 1024.0);
        g_ramU.store(g_ramT.load() - (mem.ullAvailPhys / 1024.0 / 1024.0 / 1024.0));
        g_ramP.store((g_ramU.load() / g_ramT.load()) * 100.0);
    }
    PdhCloseQuery(query);
}

void GpuMonitorThread() {
    HMODULE h = LoadLibraryA("nvml.dll");
    if (!h) h = LoadLibraryA("C:\\Windows\\System32\\DriverStore\\FileRepository\\nv_dispi.inf_amd64_eb90bba13149c063\\nvml.dll");
    if (!h) return;

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
//
//int main() {
//    std::thread t1(SystemMonitorThread);
//    std::thread t2(GpuMonitorThread);
//
//    while (g_running) {
//        std::system("cls");
//        std::cout << "=== SYSTEM MONITOR ===\n";
//        std::printf("CPU:  %.2f%%\n", g_cpu.load());
//        std::printf("RAM:  %.2f%% (%.2f GB / %.2f GB)\n", g_ramP.load(), g_ramU.load(), g_ramT.load());
//        std::printf("Disk: Active: %.2f%% | R: %.2f MB/s | W: %.2f MB/s\n", g_diskActive.load(), g_diskRead.load(), g_diskWrite.load());
//
//        if (g_gpuOk.load()) {
//            std::printf("GPU:  %u%% | Temp: %u C | VRAM: %.2f GB / %.2f GB\n", g_gpu.load(), g_gpuTemp.load(), g_vramU.load(), g_vramT.load());
//        }
//        std::cout << "======================\nPress Ctrl+C to exit\n";
//        std::this_thread::sleep_for(std::chrono::seconds(1));
//    }
//
//    g_running = false;
//    if (t1.joinable()) t1.join();
//    if (t2.joinable()) t2.join();
//    return 0;
//}
