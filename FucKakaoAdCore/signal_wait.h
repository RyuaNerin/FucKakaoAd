#pragma once

#include <mutex>
#include <condition_variable>

class signal_wait
{
public:
    explicit signal_wait(bool flag = false);

    void set();

    bool wait();

private:
    bool m_flag;
    std::mutex m_mutex;
    std::condition_variable m_signal;
};
