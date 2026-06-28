#pragma once

#include <atomic>
#include <vector>
#include <string>
#include <windows.h>
#include <psapi.h>

extern std::atomic<double> g_cpu;
extern std::atomic<double> g_ramP;
extern std::atomic<double> g_ramU;
extern std::atomic<double> g_ramT;
extern std::atomic<double> g_diskActive;
extern std::atomic<double> g_diskRead;
extern std::atomic<double> g_diskWrite;
extern std::atomic<unsigned int> g_gpu;
extern std::atomic<unsigned int> g_gpuTemp;
extern std::atomic<double> g_vramU;
extern std::atomic<double> g_vramT;
extern std::atomic<bool> g_gpuOk;
extern std::atomic<bool> g_running;

extern std::atomic<double> g_netSent;
extern std::atomic<double> g_netRecv;

extern std::atomic<int> g_batteryPercent;
extern std::atomic<bool> g_isCharging;

extern std::atomic<double> g_fps;
extern std::atomic<bool> g_fpsEnabled;

extern std::atomic<bool> g_cpuAlerted;
extern std::atomic<bool> g_ramAlerted;
extern std::atomic<bool> g_gpuAlerted;
extern const double CPU_WARNING;
extern const double RAM_WARNING;
extern const unsigned int GPU_WARNING;

struct ProcessInfo {
    std::wstring name;
    double cpuUsage;
    SIZE_T memoryUsage;
};

void SystemMonitorThread();
void GpuMonitorThread();
void AlertThread();
void FPSTestThread();

std::vector<ProcessInfo> GetTopProcesses(int count = 5);

class FPSMonitor {
private:
    LARGE_INTEGER frequency;
    LARGE_INTEGER lastTime;
    int frameCount;
    double fps;

public:
    FPSMonitor();
    void Frame();
};

void PrintHeader();
void PrintCPU();
void PrintRAM();
void PrintDisk();
void PrintNetwork();
void PrintGPU();
void PrintProcesses();
void PrintBattery();
void PrintFPS();
void PrintFooter();