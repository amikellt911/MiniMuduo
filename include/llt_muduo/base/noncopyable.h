#pragma once

namespace llt_muduo
{

class noncopyable
{
public:
    noncopyable(const noncopyable&) = delete;
    noncopyable &operator=(const noncopyable&) = delete;
protected:
    noncopyable() = default;
    ~noncopyable() = default;
};

}