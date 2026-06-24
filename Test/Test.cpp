#include "gtest/gtest.h"
#include "../monitor/monitor.h"
#include <thread>
#include <chrono>

TEST(SystemMonitorTest, TrackRamAndCpuData) {
    g_running = true;
    g_ramT.store(0.0);
    g_ramU.store(0.0);

    std::thread monitorThread(SystemMonitorThread);

    std::this_thread::sleep_for(std::chrono::milliseconds(2500));

    g_running = false;
    if (monitorThread.joinable()) {
        monitorThread.join();
    }

    EXPECT_GT(g_ramT.load(), 0.0);
    EXPECT_LE(g_ramU.load(), g_ramT.load());
    EXPECT_GE(g_ramP.load(), 0.0);
    EXPECT_LE(g_ramP.load(), 100.0);
    EXPECT_GE(g_cpu.load(), 0.0);
    EXPECT_LE(g_cpu.load(), 100.0);
}

TEST(SystemMonitorTest, DiskLimitsValidation) {
    g_running = true;
    g_diskActive.store(0.0);

    std::thread monitorThread(SystemMonitorThread);
    std::this_thread::sleep_for(std::chrono::milliseconds(1500));

    g_running = false;
    if (monitorThread.joinable()) {
        monitorThread.join();
    }

    EXPECT_LE(g_diskActive.load(), 100.0);
    EXPECT_GE(g_diskActive.load(), 0.0);
    EXPECT_GE(g_diskRead.load(), 0.0);
    EXPECT_GE(g_diskWrite.load(), 0.0);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}