#include "../inc/threadpool.hpp"

#include <functional>
#include <iostream>
#include <mutex>
#include <thread>
#include <tuple>

#include "../inc/defines.hpp"

Threadpool::Threadpool(int num_threads) { set_threads(num_threads); }
void Threadpool::set_threads(int num_threads) { m_max_threads_count = num_threads; }

void Threadpool::enque_task(std::function<void(LinkData linkdata)> task, LinkData linkdata) {
    // std::lock_guard<std::mutex> lock(m_queue_mut);
    m_tasks2go.emplace(std::make_tuple(task, linkdata));
}

void Threadpool::do_task(std::tuple<std::function<void(LinkData linkdata)>, LinkData> task_data) {
    m_threads.push_back(std::thread([task_data, this]() {
        {
            std::lock_guard<std::mutex> log_lock(m_log_mut);
            std::cout << ANSI_GREEN << "thread #" << std::this_thread::get_id() << " started\r\n" << ANSI_RESET;
        }
        auto fun = std::get<0>(task_data);
        auto arg = std::get<1>(task_data);
        fun(arg);
        {
            std::lock_guard<std::mutex> log_lock(m_log_mut);
            std::cout << ANSI_GREEN << "thread #" << std::this_thread::get_id() << " finishing\r\n" << ANSI_RESET;
        }
        m_cv_mut.lock();
        {
            std::lock_guard<std::mutex> lock(m_thr_data_mut);
            m_thr_data.id = std::this_thread::get_id();
            m_thr_data.notify_condition = true;
        }
        {
            std::lock_guard<std::mutex> lock(m_log_mut);
            std::cout << ANSI_GREEN << "thread #" << std::this_thread::get_id() << " sending END SIG\r\n" << ANSI_RESET;
        }
        m_cv.notify_one();
    }));
}

void Threadpool::run_tasks() {
    std::thread thr_enqueing([this]() {  // doing no harm
        while (true) {
            // std::lock_guard<std::mutex> queue_lock(m_queue_mut);
            if (((int)m_threads.size() < (int)m_max_threads_count) && (m_tasks2go.size() != 0)) {
                do_task(m_tasks2go.front());
                m_tasks2go.pop();
            }
            if (m_should_stop) {
                break;
            }
            std::this_thread::yield();
        }
    });
    std::thread thr_recycling([this]() {  // doing segfault
        while (m_threads.size() > 0 || !(m_should_stop && m_threads.size() == 0)) {
            if (m_threads.size() != 0) {
                {
                    std::lock_guard<std::mutex> lock(m_log_mut);
                    std::cout << ANSI_YELLOW << "thread recycling start\r\n" << ANSI_RESET;
                }
                std::unique_lock<std::mutex> lock(m_thr_data_mut);
                m_cv.wait(lock, [this] { return m_thr_data.notify_condition; });
                m_thr_data.notify_condition = false;
                {
                    std::lock_guard<std::mutex> lock(m_log_mut);
                    std::cout << ANSI_YELLOW << "thread recycling end\r\n" << ANSI_RESET;
                }
                for (int i = 0; i < (int)m_threads.size(); ++i) {
                    if (m_threads.at(i).get_id() == m_thr_data.id) {
                        m_threads.at(i).join();
                        m_threads.erase(m_threads.begin() + i);
                        {
                            std::lock_guard<std::mutex> lock(m_log_mut);
                            std::cout << ANSI_RED << "thread #" << m_thr_data.id << " ended.\r\n" << ANSI_RESET;
                        }
                    }
                }
                m_cv_mut.unlock();
            }
            {
                std::lock_guard<std::mutex> lock(m_log_mut);
                std::lock_guard<std::mutex> que_lock(m_queue_mut);
                std::cout << ANSI_RED << "NOTHING to do "
                          << "should_stop:" << m_should_stop << " threads:" << m_threads.size()
                          << " queue.size:" << m_tasks2go.size() << "\r\n"
                          << ANSI_RESET;
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }

            std::this_thread::yield();
        }
    });

    thr_enqueing.join();
    thr_recycling.join();
}
void Threadpool::stop() { m_should_stop = true; }

