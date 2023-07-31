#pragma once

#include <chrono>
#include <iostream>

#define PROFILE_CONCAT_INTERNAL(X, Y) X ## Y
#define PROFILE_CONCAT(X, Y) PROFILE_CONCAT_INTERNAL(X, Y)
#define UNIQUE_VAR_NAME_PROFILE PROFILE_CONCAT(profileGuard, __LINE__)
/**
 * Макрос замеряет время, прошедшее с момента своего вызова
 * до конца текущего блока, и выводит в поток std::cerr.
 *
 * Пример использования:
 *
 *  void Task1() {
 *      LOG_DURATION("Task 1"s); // Выведет в cerr время работы функции Task1
 *      ...
 *  }
 */
#define LOG_DURATION(x) LogDuration UNIQUE_VAR_NAME_PROFILE(x)
/**
 * Поведение аналогично макросу LOG_DURATION, при этом можно указать поток,
 * в который должно быть выведено измеренное время.
 *
 * Пример использования:
 *
 *  int main() {
 *      // Выведет время работы main в поток std::cout
 *      LOG_DURATION("main"s, std::cout);
 *      ...
 *  }
 */
#define LOG_DURATION_STREAM(x, out) LogDuration UNIQUE_VAR_NAME_PROFILE(x, out)


class LogDuration
{
public:
    // заменим имя типа std::chrono::steady_clock
    // с помощью using для удобства
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