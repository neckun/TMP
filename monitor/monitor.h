#pragma once

#include <atomic>
#include <windows.h>
#include <pdh.h>

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

void SystemMonitorThread();
void GpuMonitorThread();