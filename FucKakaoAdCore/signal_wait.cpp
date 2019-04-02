#include "signal_wait.h"

signal_wait::signal_wait(bool flag)
    : m_flag(flag)
{
}

void signal_wait::set()
{
    std::lock_guard<std::mutex> _(this->m_mutex);
    this->m_flag = true;
    this->m_signal.notify_one();
}

void signal_wait::wait()
{
    std::unique_lock<std::mutex> ul(this->m_mutex);

    while (!this->m_flag)
        this->m_signal.wait(ul);
    this->m_flag = false;
}
