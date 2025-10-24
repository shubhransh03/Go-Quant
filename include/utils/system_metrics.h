#ifndef SYSTEM_METRICS_H
#define SYSTEM_METRICS_H

#include <chrono>
#include <thread>
#include <atomic>
#include <memory>
#include "metrics_manager.h"

#ifdef _WIN32
#include <windows.h>
#include <psapi.h>
#elif defined(__linux__)
#include <sys/resource.h>
#include <sys/sysinfo.h>
#elif defined(__APPLE__)
#include <sys/resource.h>
#include <unistd.h>
#endif

class SystemMetrics {
public:
    static SystemMetrics& instance() {
        static SystemMetrics instance;
        return instance;
    }

    void start() {
        if (!running_) {
            running_ = true;
            collector_thread_ = std::thread(&SystemMetrics::collectMetrics, this);
        }
    }

    void stop() {
        running_ = false;
        if (collector_thread_.joinable()) {
            collector_thread_.join();
        }
    }

private:
    SystemMetrics() = default;
    ~SystemMetrics() { stop(); }
    SystemMetrics(const SystemMetrics&) = delete;
    SystemMetrics& operator=(const SystemMetrics&) = delete;

    void collectMetrics() {
        while (running_) {
            updateCPUMetrics();
            updateMemoryMetrics();
            updateThreadMetrics();
            
            // Collect every second
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }

    void updateCPUMetrics() {
        #ifdef _WIN32
            FILETIME idleTime, kernelTime, userTime;
            if (GetSystemTimes(&idleTime, &kernelTime, &userTime)) {
                ULARGE_INTEGER idle, kernel, user;
                idle.LowPart = idleTime.dwLowDateTime;
                idle.HighPart = idleTime.dwHighDateTime;
                kernel.LowPart = kernelTime.dwLowDateTime;
                kernel.HighPart = kernelTime.dwHighDateTime;
                user.LowPart = userTime.dwLowDateTime;
                user.HighPart = userTime.dwHighDateTime;

                static ULARGE_INTEGER lastIdle = {0}, lastKernel = {0}, lastUser = {0};

                ULONGLONG idleDiff = idle.QuadPart - lastIdle.QuadPart;
                ULONGLONG kernelDiff = kernel.QuadPart - lastKernel.QuadPart;
                ULONGLONG userDiff = user.QuadPart - lastUser.QuadPart;
                ULONGLONG total = kernelDiff + userDiff;
                
                if (total > 0) {
                    double cpuUsage = ((total - idleDiff) * 100.0) / total;
                    MetricsManager::instance().setSystemMetric("cpu_usage", cpuUsage);
                }

                lastIdle = idle;
                lastKernel = kernel;
                lastUser = user;
            }
        #elif defined(__linux__)
            // Linux CPU metrics implementation
            FILE* file = fopen("/proc/stat", "r");
            if (file) {
                unsigned long user, nice, system, idle;
                if (fscanf(file, "cpu %lu %lu %lu %lu", &user, &nice, &system, &idle) == 4) {
                    static unsigned long lastUser = 0, lastNice = 0, lastSystem = 0, lastIdle = 0;
                    
                    unsigned long userDiff = user - lastUser;
                    unsigned long niceDiff = nice - lastNice;
                    unsigned long systemDiff = system - lastSystem;
                    unsigned long idleDiff = idle - lastIdle;
                    unsigned long total = userDiff + niceDiff + systemDiff + idleDiff;
                    
                    if (total > 0) {
                        double cpuUsage = ((total - idleDiff) * 100.0) / total;
                        MetricsManager::instance().setSystemMetric("cpu_usage", cpuUsage);
                    }

                    lastUser = user;
                    lastNice = nice;
                    lastSystem = system;
                    lastIdle = idle;
                }
                fclose(file);
            }
        #elif defined(__APPLE__)
            // macOS: placeholder (detailed CPU collection omitted for brevity)
            // Could integrate host_processor_info; for now, skip or set -1.
            MetricsManager::instance().setSystemMetric("cpu_usage", -1.0);
        #endif
    }

    void updateMemoryMetrics() {
        #ifdef _WIN32
            PROCESS_MEMORY_COUNTERS_EX pmc;
            if (GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc))) {
                // Process memory
                double processMemoryMB = pmc.WorkingSetSize / (1024.0 * 1024.0);
                MetricsManager::instance().setSystemMetric("process_memory_mb", processMemoryMB);
                
                // Virtual memory
                double virtualMemoryMB = pmc.PrivateUsage / (1024.0 * 1024.0);
                MetricsManager::instance().setSystemMetric("virtual_memory_mb", virtualMemoryMB);
            }

            MEMORYSTATUSEX memInfo;
            memInfo.dwLength = sizeof(MEMORYSTATUSEX);
            if (GlobalMemoryStatusEx(&memInfo)) {
                // System memory usage percentage
                MetricsManager::instance().setSystemMetric("system_memory_usage", 
                    static_cast<double>(memInfo.dwMemoryLoad));
                
                // Available physical memory
                double availableMemoryGB = memInfo.ullAvailPhys / (1024.0 * 1024.0 * 1024.0);
                MetricsManager::instance().setSystemMetric("available_memory_gb", availableMemoryGB);
            }
        #elif defined(__linux__)
            // Linux memory metrics implementation
            struct sysinfo si;
            if (sysinfo(&si) == 0) {
                double totalRam = si.totalram * si.mem_unit / (1024.0 * 1024.0 * 1024.0);
                double freeRam = si.freeram * si.mem_unit / (1024.0 * 1024.0 * 1024.0);
                double usedRam = totalRam - freeRam;
                
                MetricsManager::instance().setSystemMetric("total_memory_gb", totalRam);
                MetricsManager::instance().setSystemMetric("used_memory_gb", usedRam);
                MetricsManager::instance().setSystemMetric("memory_usage_percent", 
                    (usedRam / totalRam) * 100.0);
            }
        #elif defined(__APPLE__)
            // macOS: placeholders for memory metrics
            MetricsManager::instance().setSystemMetric("total_memory_gb", -1.0);
            MetricsManager::instance().setSystemMetric("used_memory_gb", -1.0);
            MetricsManager::instance().setSystemMetric("memory_usage_percent", -1.0);
        #endif
    }

    void updateThreadMetrics() {
        #ifdef _WIN32
            HANDLE hProcess = GetCurrentProcess();
            DWORD processId = GetProcessId(hProcess);
            HANDLE h = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, processId);
            if (h != INVALID_HANDLE_VALUE) {
                THREADENTRY32 te;
                te.dwSize = sizeof(te);
                int threadCount = 0;
                
                if (Thread32First(h, &te)) {
                    do {
                        if (te.th32OwnerProcessID == processId) {
                            threadCount++;
                        }
                    } while (Thread32Next(h, &te));
                }
                CloseHandle(h);
                
                MetricsManager::instance().setSystemMetric("thread_count", threadCount);
            }
        #elif defined(__linux__)
            // Linux thread count
            char buf[32];
            snprintf(buf, sizeof(buf), "/proc/%d/stat", getpid());
            FILE* f = fopen(buf, "r");
            if (f) {
                int unused;
                char comm[256];
                char state;
                int ppid, pgrp, session, tty_nr, tpgid;
                unsigned long flags, minflt, cminflt, majflt, cmajflt, utime, stime;
                long cutime, cstime, priority, nice, num_threads;
                
                fscanf(f, "%d %s %c %d %d %d %d %d %lu %lu %lu %lu %lu %lu %lu %ld %ld %ld %ld %ld",
                    &unused, comm, &state, &ppid, &pgrp, &session, &tty_nr, &tpgid,
                    &flags, &minflt, &cminflt, &majflt, &cmajflt, &utime, &stime,
                    &cutime, &cstime, &priority, &nice, &num_threads);
                    
                MetricsManager::instance().setSystemMetric("thread_count", num_threads);
                fclose(f);
            }
        #elif defined(__APPLE__)
            // macOS: placeholder for thread count
            MetricsManager::instance().setSystemMetric("thread_count", -1.0);
        #endif
    }

    std::atomic<bool> running_{false};
    std::thread collector_thread_;
};

#endif // SYSTEM_METRICS_H