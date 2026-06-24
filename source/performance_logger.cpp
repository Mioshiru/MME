#include "performance_logger.h"
#include <fstream>
#include <iostream>

std::chrono::time_point<std::chrono::high_resolution_clock> PerformanceLogger::frame_start;
std::unordered_map<std::string, std::chrono::time_point<std::chrono::high_resolution_clock>> PerformanceLogger::active_timers;
std::unordered_map<std::string, double> PerformanceLogger::accumulated_times;
int PerformanceLogger::frame_count = 0;

void PerformanceLogger::BeginFrame() {
    frame_start = std::chrono::high_resolution_clock::now();
}

void PerformanceLogger::StartTimer(const std::string& name) {
    active_timers[name] = std::chrono::high_resolution_clock::now();
}

void PerformanceLogger::StopTimer(const std::string& name) {
    auto end_time = std::chrono::high_resolution_clock::now();
    if (active_timers.find(name) != active_timers.end()) {
        double elapsed = std::chrono::duration<double, std::milli>(end_time - active_timers[name]).count();
        accumulated_times[name] += elapsed;
    }
}

void PerformanceLogger::EndFrame() {
    frame_count++;
    
    // Dump to file every 60 frames (approx 1 second if VSync is on)
    if (frame_count >= 60) {
        auto total_frame_end = std::chrono::high_resolution_clock::now();
        
        std::ofstream outfile("debug.txt", std::ios_base::app);
        if (outfile.is_open()) {
            outfile << "=== Performance Report (avg over " << frame_count << " frames) ===\n";
            for (const auto& pair : accumulated_times) {
                outfile << pair.first << ": " << (pair.second / frame_count) << " ms\n";
            }
            outfile << "--------------------------------------------------\n";
            outfile.close();
        }
        
        // Reset
        accumulated_times.clear();
        frame_count = 0;
    }
}
