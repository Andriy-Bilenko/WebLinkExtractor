#include "../inc/threadpool.hpp"

#include <functional>
#include <iostream>
#include <mutex>
#include <thread>
#include <tuple>

#include "../inc/defines.hpp"

// TODO: clean the logging, have it set only if verbose is true

Threadpool::Threadpool(int num_threads) { set_threads(num_threads); }
void Threadpool::set_threads(int num_threads) { m_max_threads_count = num_threads; }

void Threadpool::enque_task(std::function<void(LinkData linkdata)> task, LinkData linkdata) {
    std::lock_guard<std::mutex> lock(m_queue_mut);
    m_queue.emplace(std::make_tuple(task, linkdata));
}

void Threadpool::do_task(std::tuple<std::function<void(LinkData linkdata)>, LinkData> task_data) {
    m_threads.push_back(std::thread([task_data, this]() {
        {  // log start ===============
            std::lock_guard<std::mutex> log_lock(m_log_mut);
            std::cout << ANSI_GREEN << "thread #" << std::this_thread::get_id() << " started\r\n" << ANSI_RESET;
            std::lock_guard<std::mutex> que_lock(m_queue_mut);
            std::lock_guard<std::mutex> threads_lock(m_threads_mut);
            std::lock_guard<std::mutex> should_stop_lock(m_should_stop_mut);

            std::cout << ANSI_BLUE << "should_stop:" << m_should_stop << " threads:" << m_threads.size()
                      << " queue.size:" << m_queue.size() << "\r\n"
                      << ANSI_RESET;
        }  // log end =================

        auto fun = std::get<0>(task_data);
        auto arg = std::get<1>(task_data);

        fun(arg);

        {  // log start ===============
            std::lock_guard<std::mutex> log_lock(m_log_mut);
            std::cout << ANSI_GREEN << "thread #" << std::this_thread::get_id() << " finishing\r\n" << ANSI_RESET;
            std::lock_guard<std::mutex> que_lock(m_queue_mut);
            std::lock_guard<std::mutex> threads_lock(m_threads_mut);
            std::lock_guard<std::mutex> should_stop_lock(m_should_stop_mut);

            std::cout << ANSI_BLUE << "should_stop:" << m_should_stop << " threads:" << m_threads.size()
                      << " queue.size:" << m_queue.size() << "\r\n"
                      << ANSI_RESET;
        }  // log end =================

        m_cv_mut.lock();

        {  // setting finishing thread data
            std::lock_guard<std::mutex> thr_data_lock(m_thr_data_mut);
            m_finishing_thr_data.id = std::this_thread::get_id();
            m_finishing_thr_data.notify_condition = true;
        }
        {  // log start ==============
            std::lock_guard<std::mutex> log_lock(m_log_mut);
            std::cout << ANSI_GREEN << "thread #" << std::this_thread::get_id() << " sending END SIG\r\n" << ANSI_RESET;
        }  // log end ================

        m_cv.notify_one();
    }));
}

void Threadpool::run_tasks() {
    std::thread thr_enqueing([this]() {
        while (true) {
            {  // addtionally scoped for testing start
                std::lock_guard<std::mutex> queue_lock(m_queue_mut);
                std::lock_guard<std::mutex> threads_lock(m_threads_mut);
                std::lock_guard<std::mutex> max_threads_lock(m_max_threads_mut);

                if (((int)m_threads.size() < (int)m_max_threads_count) && (m_queue.size() != 0)) {
                    do_task(m_queue.front());
                    m_queue.pop();
                }
                {
                    std::lock_guard<std::mutex> should_stop_lock(m_should_stop_mut);
                    if (m_should_stop && (m_queue.size() == 0)) {
                        break;
                    }
                }
            }  // addtionally scoped for testing end
            std::this_thread::yield();
        }
    });
    std::thread thr_recycling([this]() {  // doing segfault
        while (true) {
            {
                std::lock_guard<std::mutex> queue_lock(m_queue_mut);
                std::lock_guard<std::mutex> threads_lock(m_threads_mut);
                std::lock_guard<std::mutex> should_stop_lock(m_should_stop_mut);
                if ((m_should_stop && m_threads.size() == 0 && m_queue.size() == 0)) {
                    break;
                }
            }
            if (m_threads.size() != 0) {
                {  // log start ==============
                    std::lock_guard<std::mutex> log_lock(m_log_mut);
                    std::lock_guard<std::mutex> que_lock(m_queue_mut);
                    std::lock_guard<std::mutex> threads_lock(m_threads_mut);
                    std::lock_guard<std::mutex> should_stop_lock(m_should_stop_mut);

                    std::cout << ANSI_YELLOW << "thread recycling wait\r\n" << ANSI_RESET;
                    std::cout << ANSI_BLUE << "should_stop:" << m_should_stop << " threads:" << m_threads.size()
                              << " queue.size:" << m_queue.size() << "\r\n"
                              << ANSI_RESET;
                }  // log end ================

                {  // condition variable waiting start
                    std::unique_lock<std::mutex> lock(m_thr_data_mut);
                    m_cv.wait(lock, [this] { return m_finishing_thr_data.notify_condition; });
                }  // condition variable waiting end

                m_finishing_thr_data.notify_condition = false;
                {  // log start ==============
                    std::lock_guard<std::mutex> log_lock(m_log_mut);
                    std::lock_guard<std::mutex> que_lock(m_queue_mut);
                    std::lock_guard<std::mutex> threads_lock(m_threads_mut);
                    std::lock_guard<std::mutex> should_stop_lock(m_should_stop_mut);

                    std::cout << ANSI_YELLOW << "thread recycling start\r\n" << ANSI_RESET;
                    std::cout << ANSI_BLUE << "should_stop:" << m_should_stop << " threads:" << m_threads.size()
                              << " queue.size:" << m_queue.size() << "\r\n"
                              << ANSI_RESET;
                }  // log end ================
                {
                    int threads_size{};
                    std::thread::id finishing_thr_id;
                    {
                        std::lock_guard<std::mutex> threads_lock(m_threads_mut);
                        threads_size = m_threads.size();
                    }
                    {
                        std::lock_guard<std::mutex> thr_data_lock(m_thr_data_mut);
                        finishing_thr_id = m_finishing_thr_data.id;
                    }
                    for (int i = 0; i < threads_size; ++i) {
                        std::thread::id i_id;
                        {
                            std::lock_guard<std::mutex> threads_lock(m_threads_mut);
                            i_id = m_threads.at(i).get_id();
                        }

                        if (i_id == finishing_thr_id) {
                            {
                                std::lock_guard<std::mutex> threads_lock(m_threads_mut);
                                m_threads.at(i).join();
                                m_threads.erase(m_threads.begin() + i);
                            }
                            {  // log start ==============
                                std::lock_guard<std::mutex> log_lock(m_log_mut);
                                std::lock_guard<std::mutex> que_lock(m_queue_mut);
                                std::lock_guard<std::mutex> threads_lock(m_threads_mut);
                                std::lock_guard<std::mutex> should_stop_lock(m_should_stop_mut);

                                std::cout << ANSI_RED << "thread #" << finishing_thr_id << " ended.\r\n" << ANSI_RESET;
                                std::cout << ANSI_BLUE << "should_stop:" << m_should_stop
                                          << " threads:" << m_threads.size() << " queue.size:" << m_queue.size()
                                          << "\r\n"
                                          << ANSI_RESET;
                            }  // log end ==============
                            break;
                        }
                    }
                }
                {  // log start ===============
                    std::lock_guard<std::mutex> lock(m_log_mut);
                    std::cout << ANSI_YELLOW << "END SIG releasing..."
                              << "\r\n"
                              << ANSI_RESET;
                }  // log end =================

                m_cv_mut.unlock();

                {  // log start ===============
                    std::lock_guard<std::mutex> lock(m_log_mut);
                    std::cout << ANSI_YELLOW << "END SIG releasing done."
                              << "\r\n"
                              << ANSI_RESET;
                }  // log end =================
            }

            std::this_thread::yield();
        }
    });

    thr_enqueing.join();
    thr_recycling.join();
}
void Threadpool::finish_when_empty() {
    std::lock_guard<std::mutex> should_stop_lock(m_should_stop_mut);
    m_should_stop = true;
}

