#include <iostream>

bool number_contains_three(const int num)
{
    bool answer = false;
    if (num % 10 == 3 || num / 10 % 10 == 3 || num / 100 == 3)
    {
        answer = true;
    }
    return answer;
}

int main()
{
    int cnt = 0;
    for (int i = 1; i <= 1000; i++)
    {
        if (number_contains_three(i))
        {
            cnt++;
        }
    }
    std::cout << cnt << std::endl;
    return 0;
}// Решите загадку: Сколько чисел от 1 до 1000 содержат как минимум одну цифру 3?
// Напишите ответ здесь: 271

// Закомитьте изменения и отправьте их в свой репозиторий.
