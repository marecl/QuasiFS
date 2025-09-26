#pragma once

#include <sys/types.h>

#include <iostream>
#include <format>

template <typename... Args>
void LogCustom(const std::string_view fn, const std::string_view msg, std::format_string<Args...> fmt, Args &&...args)
{
    std::cout << std::format("[{:^25s}] ", fn) << std::format("{}", msg) << std::format(fmt, std::forward<Args>(args)...) << "\n";
}

#define Log(fmt, ...)                                            \
    do                                                           \
    {                                                            \
        LogCustom(__FUNCTION__, "[INFO]\t", fmt, ##__VA_ARGS__); \
    } while (0)

#define LogTest(fmt, ...)                                                        \
    do                                                                           \
    {                                                                            \
        LogCustom(__FUNCTION__, "\033[34;1m[TEST]\033[0m ", fmt, ##__VA_ARGS__); \
    } while (0)

#define LogError(fmt, ...)                                                                                      \
    do                                                                                                          \
    {                                                                                                           \
        LogCustom(__FUNCTION__, "\033[31;1m[FAIL]\033[0m ", fmt " ({}:{})", ##__VA_ARGS__, __FILE__, __LINE__); \
    } while (0)

#define LogSuccess(fmt, ...)                                                     \
    do                                                                           \
    {                                                                            \
        LogCustom(__FUNCTION__, "\033[32;1m[SUCC]\033[0m ", fmt, ##__VA_ARGS__); \
    } while (0)
