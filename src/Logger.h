#pragma once
#include <iostream>
#include <string>
#include <chrono>
#include <iomanip>
#include <sstream>

namespace Color {
    const std::string RESET = "\033[0m";
    const std::string RED = "\033[31m";
    const std::string GREEN = "\033[32m";
    const std::string YELLOW = "\033[33m";
    const std::string CYAN = "\033[36m";
    const std::string MAGENTA = "\033[35m";
}

enum class LogLevel { INFO, SUCCESS, ERROR, WARNING, WAIT };

inline void log_message(const std::string& message, LogLevel level = LogLevel::INFO) {
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    std::tm timeinfo;
    localtime_s(&timeinfo, &in_time_t);

    std::stringstream ss;
    ss << std::put_time(&timeinfo, "%Y-%m-%d %H:%M:%S");
    std::string timestamp = ss.str();

    std::string color, icon;
    switch (level) {
        case LogLevel::SUCCESS: color = Color::GREEN; icon = "✅"; break;
        case LogLevel::ERROR:   color = Color::RED; icon = "🚨"; break;
        case LogLevel::WARNING: color = Color::YELLOW; icon = "⚠️"; break;
        case LogLevel::WAIT:    color = Color::CYAN; icon = "⌛"; break;
        default:                color = Color::RESET; icon = "ℹ️"; break;
    }

    std::string border = Color::MAGENTA + "================================================================================" + Color::RESET;
    std::cout << border << std::endl;
    std::cout << color << "[" << timestamp << "] " << icon << " " << message << Color::RESET << std::endl;
    std::cout << border << std::endl;
}
