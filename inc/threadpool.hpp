#pragma once

#include <atomic>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <tuple>
#include <vector>

#include "dataStructs.hpp"

class Threadpool {
   public:
    Threadpool();
    Threadpool(int num_threads);
    // change max number of threads after creation
    void set_threads(int num_threads);
    void set_verbose(bool verbose);
    // setting one mutex for all logging across Threadpool and Parser
    void set_log_mutex(std::mutex& mut);
    // adding task to the waiting queue
    void enque_task(std::function<void(LinkData linkdata)> task, LinkData linkdata);
    // start putting tasks from queue to m_threads
    void run_tasks();
    // for signaling when should not wait endlessly
    void finish_when_empty();

   private:
    // get one task from the queue and put to the m_threads
    void do_task(std::tuple<std::function<void(LinkData linkdata)>, LinkData> task_data);

    std::atomic<int> m_max_threads_count;
    std::atomic<bool> m_verbose_logging;
    std::atomic<bool> m_should_stop;
    std::vector<std::thread> m_threads;
    std::queue<std::tuple<std::function<void(LinkData linkdata)>, LinkData>> m_queue;
    ThreadData m_finishing_thr_data;  // data of thread that is being finished
    std::mutex m_queue_mut;           // for queue
    std::mutex m_log_mut;             // for logging if no other mutex set to m_log_mut_ptr
    std::mutex* m_log_mut_ptr{nullptr};
    std::mutex m_threads_mut;   // for m_threads
    std::mutex m_thr_data_mut;  // for m_finishing_thr_data
    std::mutex m_cv_mut;        // for m_cv and synchronization between thread finishing and cleaner thread

    std::condition_variable m_cv;
};
