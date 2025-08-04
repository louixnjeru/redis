#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

#include "time.h"

std::string getTime(char format[]) {
    auto now = std::chrono::system_clock::now();
    std::time_t now_time_t = std::chrono::system_clock::to_time_t(now);
    std::tm* local_tm = std::localtime(&now_time_t);

    std::ostringstream oss;

    oss << std::put_time(local_tm, format);

    return oss.str();

}
