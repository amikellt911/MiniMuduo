#include <time.h>

#include "MiniMuduo/base/Timestamp.h"
namespace MiniMuduo
{
    namespace base{
    Timestamp::Timestamp() : microSecondsSinceEpoch_(0)
    {
    }

    Timestamp::Timestamp(int64_t microSecondsSinceEpoch)
        : microSecondsSinceEpoch_(microSecondsSinceEpoch)
    {
    }

    Timestamp Timestamp::now()
    {
        return Timestamp(time(NULL));
    }
    std::string Timestamp::toString() const
    {
        char buf[128] = {0};
        tm *tm_time = localtime(&microSecondsSinceEpoch_);
        snprintf(buf, 128, "%4d_%02d_%02d_%02d_%02d_%02d",
                 tm_time->tm_year + 1900,
                 tm_time->tm_mon + 1,
                 tm_time->tm_mday,
                 tm_time->tm_hour,
                 tm_time->tm_min,
                 tm_time->tm_sec);
        return buf;
    }

    Timestamp Timestamp::operator+(double interval)
    {
        return Timestamp(microSecondsSinceEpoch_ + static_cast<int64_t>(interval * kMicroSecondsPerSecond));

    }

    bool Timestamp::operator<(const Timestamp &that) const
    {
        return microSecondsSinceEpoch_ < that.microSecondsSinceEpoch_;
    }

    int64_t Timestamp::microSecondsSinceEpoch() const
    {
        return microSecondsSinceEpoch_;
    }
    }
}
