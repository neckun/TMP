#include "gtest/gtest.h"
#include <thread>
#include <chrono>
#include <atomic>

extern std::atomic<double> g_cpu, g_ramP, g_ramU, g_ramT;
extern std::atomic<double> g_diskActive, g_diskRead, g_diskWrite;
extern std::atomic<bool> g_running;

void SystemMonitorThread();
void GpuMonitorThread();

// Тест 1: Проверяем, что мониторинг системы запускается и записывает адекватные данные RAM
TEST(SystemMonitorTest, TrackRamAndCpuData) {
    g_running = true;
    g_ramT.store(0.0);
    g_ramU.store(0.0);

    // Запускаем поток мониторинга асинхронно
    std::thread monitorThread(SystemMonitorThread);

    // Даем потоку поработать 2.5 секунды, чтобы выполнилось хотя бы 2 итерации цикла Pdh
    std::this_thread::sleep_for(std::chrono::milliseconds(2500));

    // Останавливаем поток мониторинга ресурсов
    g_running = false;
    if (monitorThread.joinable()) {
        monitorThread.join();
    }

    // Проверяем результаты через Google Test макросы
    // Общий объем ОЗУ на ПК должен быть строго больше нуля
    EXPECT_GT(g_ramT.load(), 0.0);

    // Использованная память не может превышать общий объем ОЗУ
    EXPECT_LE(g_ramU.load(), g_ramT.load());

    // Процент использования RAM должен быть в диапазоне от 0% до 100%
    EXPECT_GE(g_ramP.load(), 0.0);
    EXPECT_LE(g_ramP.load(), 100.0);

    // Нагрузка на CPU также должна укладываться в валидный диапазон
    EXPECT_GE(g_cpu.load(), 0.0);
    EXPECT_LE(g_cpu.load(), 100.0);
}

// Тест 2
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
