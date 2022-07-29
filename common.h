/**
 * @brief Файл, содержащий общие коды возврата и простейшую логирующую функцию.
 */
#ifndef COMMON_H
#define COMMON_H

#include <iostream>
#include <string>

enum class ExitStatus : uint8_t
{
    Failure = 0,
    Success,
    StartPrint,
};

inline void LogEvent( std::string errMsg )
{
    std::cout << errMsg << std::endl << std::flush;
}

#endif // COMMON_H