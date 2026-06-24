#include "monitor.h"
#include <iostream>
#include <thread>
#include <chrono>

int main() {
    std::thread t1(SystemMonitorThread);
    std::thread t2(GpuMonitorThread);

    while (g_running) {
        std::system("cls");
        std::cout << "=== SYSTEM MONITOR ===\n";
        std::printf("CPU:  %.2f%%\n", g_cpu.load());
        std::printf("RAM:  %.2f%% (%.2f GB / %.2f GB)\n", g_ramP.load(), g_ramU.load(), g_ramT.load());
        std::printf("Disk: Active: %.2f%% | R: %.2f MB/s | W: %.2f MB/s\n", g_diskActive.load(), g_diskRead.load(), g_diskWrite.load());

        if (g_gpuOk.load()) {
            std::printf("GPU:  %u%% | Temp: %u C | VRAM: %.2f GB / %.2f GB\n", g_gpu.load(), g_gpuTemp.load(), g_vramU.load(), g_vramT.load());
        }
        std::cout << "======================\nPress Ctrl+C to exit\n";
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    g_running = false;
    if (t1.joinable()) t1.join();
    if (t2.joinable()) t2.join();
    return 0;
}