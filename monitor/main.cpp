#include "monitor.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <windows.h>

int main() {
    SetConsoleTitleA("System Monitor");

    std::thread t1(SystemMonitorThread);
    std::thread t2(GpuMonitorThread);
    std::thread t3(AlertThread);
    std::thread t4(FPSTestThread);

    std::cout << "System Monitor started\n";

    while (g_running) {
        system("cls");

        PrintHeader();
        PrintCPU();
        PrintRAM();
        PrintDisk();
        PrintNetwork();
        PrintGPU();
        PrintBattery();
        PrintFPS();
        PrintProcesses();
        PrintFooter();

        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }

    g_running = false;

    if (t1.joinable()) t1.join();
    if (t2.joinable()) t2.join();
    if (t3.joinable()) t3.join();
    if (t4.joinable()) t4.join();

    std::cout << "\nMonitor stopped\n";
    return 0;
}