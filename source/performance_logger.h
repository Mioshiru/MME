#ifndef RME_PERFORMANCE_LOGGER_H_
#define RME_PERFORMANCE_LOGGER_H_

#include <string>
#include <chrono>
#include <unordered_map>

class PerformanceLogger {
public:
    static void BeginFrame();
    static void EndFrame();
    
    static void StartTimer(const std::string& name);
    static void StopTimer(const std::string& name);
    
private:
    static std::chrono::time_point<std::chrono::high_resolution_clock> frame_start;
    static std::unordered_map<std::string, std::chrono::time_point<std::chrono::high_resolution_clock>> active_timers;
    static std::unordered_map<std::string, double> accumulated_times; // in ms
    static int frame_count;
};

// Helper macro for scoped timing
struct ScopedTimer {
    std::string name;
    ScopedTimer(const std::string& n) : name(n) { PerformanceLogger::StartTimer(name); }
    ~ScopedTimer() { PerformanceLogger::StopTimer(name); }
};

#define PROFILE_SCOPE(name) ScopedTimer __timer_##__LINE__(name)

#endif // RME_PERFORMANCE_LOGGER_H_
