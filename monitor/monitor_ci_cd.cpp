#include "CppUnitTest.h"
#include <windows.h>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

double ClampDiskActive(double rawValue);
double BytesToMegabytes(double bytes);
double BytesToGigabytes(unsigned long long bytes);
double CalculateRamPercentage(double used, double total);

namespace MonitorTests
{
    TEST_CLASS(UnitTests)
    {
    public:

        BEGIN_TEST_METHOD_ATTRIBUTE(Test_ClampDiskActive_UpperBoundary)
            TEST_OWNER(L"Developer")
            TEST_PRIORITY(1)
            TEST_METHOD_ATTRIBUTE(L"Category", L"Unit")
            END_TEST_METHOD_ATTRIBUTE()

            TEST_METHOD(Test_ClampDiskActive_UpperBoundary)
        {
            double result = ClampDiskActive(150.5);
            Assert::AreEqual(100.0, result, 0.001);
        }

        BEGIN_TEST_METHOD_ATTRIBUTE(Test_ClampDiskActive_NormalValue)
            TEST_METHOD_ATTRIBUTE(L"Category", L"Unit")
            END_TEST_METHOD_ATTRIBUTE()

            TEST_METHOD(Test_ClampDiskActive_NormalValue)
        {
            double result = ClampDiskActive(45.2);
            Assert::AreEqual(45.2, result, 0.001);
        }

        BEGIN_TEST_METHOD_ATTRIBUTE(Test_BytesToMegabytes)
            TEST_METHOD_ATTRIBUTE(L"Category", L"Unit")
            END_TEST_METHOD_ATTRIBUTE()

            TEST_METHOD(Test_BytesToMegabytes)
        {
            double bytes = 1048576.0;
            double result = BytesToMegabytes(bytes);
            Assert::AreEqual(1.0, result, 0.001);
        }

        BEGIN_TEST_METHOD_ATTRIBUTE(Test_CalculateRamPercentage_ZeroTotal)
            TEST_METHOD_ATTRIBUTE(L"Category", L"Unit")
            END_TEST_METHOD_ATTRIBUTE()

            TEST_METHOD(Test_CalculateRamPercentage_ZeroTotal)
        {
            double result = CalculateRamPercentage(8.0, 0.0);
            Assert::AreEqual(0.0, result, 0.001);
        }
    };

    TEST_CLASS(RegressionTests)
    {
    public:
        TEST_METHOD(Test_WinApi_GlobalMemoryStatusEx)
        {
            MEMORYSTATUSEX mem;
            mem.dwLength = sizeof(mem);
            BOOL success = GlobalMemoryStatusEx(&mem);
            Assert::IsTrue(success == TRUE);
        }
    };
}
