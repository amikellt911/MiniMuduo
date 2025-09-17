#include <sys/time.h>

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

    // Timestamp::Timestamp(const Timestamp &that)
    // : microSecondsSinceEpoch_(that.microSecondsSinceEpoch())
    // {
    // }

    Timestamp& Timestamp::operator=(const Timestamp &that)
    {
        microSecondsSinceEpoch_ = that.microSecondsSinceEpoch_;
        return *this;
    }

    Timestamp Timestamp::now()
    {
        // return Timestamp(time(NULL));
        struct timeval tv;
        // gettimeofday 可以获取到微秒精度的时间
        gettimeofday(&tv, NULL);
        int64_t seconds = tv.tv_sec;
        return Timestamp(seconds * kMicroSecondsPerSecond + tv.tv_usec);
    }
    Timestamp Timestamp::invalid()
    {
        return Timestamp();
    }
    std::string Timestamp::toString() const
    {
        char buf[128] = {0};
        time_t seconds = static_cast<time_t>(microSecondsSinceEpoch_ / 1000000);
        tm tm_time;
        localtime_r(&seconds, &tm_time); // 线程安全版本，避免用静态对象
        snprintf(buf, sizeof(buf), "%4d_%02d_%02d_%02d_%02d_%02d",
                tm_time.tm_year + 1900,
                tm_time.tm_mon + 1,
                tm_time.tm_mday,
                tm_time.tm_hour,
                tm_time.tm_min,
                tm_time.tm_sec);
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


    bool Timestamp::operator==(const Timestamp &that) const
    {
        return microSecondsSinceEpoch() == that.microSecondsSinceEpoch();

    }
    bool Timestamp::operator!=(const Timestamp &that) const
    {
        return microSecondsSinceEpoch() != that.microSecondsSinceEpoch();
    }
    }
}
