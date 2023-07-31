#pragma once

#include <chrono>
#include <iostream>

#define PROFILE_CONCAT_INTERNAL(X, Y) X ## Y
#define PROFILE_CONCAT(X, Y) PROFILE_CONCAT_INTERNAL(X, Y)
#define UNIQUE_VAR_NAME_PROFILE PROFILE_CONCAT(profileGuard, __LINE__)
/**
 * ������ �������� �����, ��������� � ������� ������ ������
 * �� ����� �������� �����, � ������� � ����� std::cerr.
 *
 * ������ �������������:
 *
 *  void Task1() {
 *      LOG_DURATION("Task 1"s); // ������� � cerr ����� ������ ������� Task1
 *      ...
 *  }
 */
#define LOG_DURATION(x) LogDuration UNIQUE_VAR_NAME_PROFILE(x)
/**
 * ��������� ���������� ������� LOG_DURATION, ��� ���� ����� ������� �����,
 * � ������� ������ ���� �������� ���������� �����.
 *
 * ������ �������������:
 *
 *  int main() {
 *      // ������� ����� ������ main � ����� std::cout
 *      LOG_DURATION("main"s, std::cout);
 *      ...
 *  }
 */
#define LOG_DURATION_STREAM(x, out) LogDuration UNIQUE_VAR_NAME_PROFILE(x, out)


class LogDuration
{
public:
    // ������� ��� ���� std::chrono::steady_clock
    // � ������� using ��� ��������
    using Clock = std::chrono::steady_clock;

    LogDuration(std::string& name)
        : id_(name)
    {}

    LogDuration(std::string_view name)
        : id_(name)
    {}

    LogDuration(const std::string& id, std::ostream& output)
        : output_(output),
          id_(id)
    {}

    ~LogDuration()
    {
        using namespace std::chrono;
        using namespace std::literals;

        const auto end_time = Clock::now();
        const auto dur = end_time - start_time_;
        output_ << id_ << ": "s << duration_cast<milliseconds>(dur).count() << " ms"s << std::endl;
    }

private:
    std::ostream& output_ = std::cerr;
    const std::string id_;
    const Clock::time_point start_time_ = Clock::now();
};