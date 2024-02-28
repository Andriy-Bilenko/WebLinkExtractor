#pragma once

#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <tuple>
#include <vector>

#include "dataStructs.hpp"

class Threadpool {
   public:
    Threadpool() = default;
    Threadpool(int num_threads);
    void set_threads(int num_threads);  // setting max to this
    void enque_task(std::function<void(LinkData linkdata)> task, LinkData linkdata);
    void run_tasks();
    void stop();

   private:
    void do_task(std::tuple<std::function<void(LinkData linkdata)>, LinkData> task_data);

    int m_max_threads_count{};
    bool m_should_stop = false;
    std::vector<std::thread> m_threads;
    std::queue<std::tuple<std::function<void(LinkData linkdata)>, LinkData>> m_tasks2go;
    std::mutex m_queue_mut;
    std::mutex m_log_mut;

    ThreadData m_thr_data;
    std::condition_variable m_cv;
    std::mutex m_thr_data_mut;
    std::mutex m_cv_mut;
};
