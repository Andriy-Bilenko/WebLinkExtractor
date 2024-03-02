#pragma once

#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <tuple>
#include <vector>

#include "dataStructs.hpp"
#include "threadpool.hpp"

class Parser {
   public:
    Parser();
    explicit Parser(int threads_count);
    Parser(const std::string& root_link, int depth_level, int threads_count, bool write_to_db, int verbose);

    std::vector<LinkData> get_currently_available() const;
    void set_root_link(const std::string& root_link);
    void set_depth_level(int depth_level);
    void set_threads_number(int threads);
    void set_logging_to_db(bool write_to_db);
    void set_verbose(int verbose);

    std::vector<LinkData> parse();  // maybe to a vector or directly to a db connection(use separate soci::session)

   private:
    std::string check_alive_and_get_html(const std::string& link);
    std::vector<std::string> get_urls_from_html(const std::string& html, const std::string& its_url);
    bool isFullURL(const std::string& url);

    static std::string pyUrlJoin(const std::string& base, const std::string& relative);
    static size_t curlWriteCallback(void* contents, size_t size, size_t nmemb, std::string* output);

    // member variables
    std::string m_root_link{""};
    int m_depth_level{};
    bool m_write_to_db{false};
    int m_verbose{};  // 0 - no logs, 1 - parser logs, 2 - parser and inner threadpool logs

    Threadpool m_threadpool;
    std::vector<LinkData> m_processed;
    std::mutex m_log_mut;
    mutable std::mutex m_processed_mut;

    static std::mutex m_py_mut;
    static std::mutex m_curl_callback_mut;
};
