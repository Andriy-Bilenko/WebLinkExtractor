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
    Parser(const std::string& root_link, int depth_level, int threads_count, int verbose);

    // returns vector of currently processed LinkData
    std::vector<LinkData> get_currently_available() const;
    // sets the base link to parse
    void set_root_link(const std::string& root_link);
    // sets how deep is recursion
    void set_depth_level(int depth_level);
    void set_threads_number(int threads);
    // 0 - no logs to stdout, 1 - links parsed, 2 - inner threadpool logs
    void set_verbose(int verbose);
    // setting postgresql credentials to connect to, and table_name to write out logs to
    void set_sql_connection(const std::string& credentials, const std::string& table_name);
    // starts parsing the root_link recursively
    std::vector<LinkData> parse();

   private:
    std::string check_alive_and_get_html(const std::string& link);
    std::vector<std::string> get_urls_from_html(const std::string& html, const std::string& its_url);
    bool isFullURL(const std::string& url);

    static std::string pyUrlJoin(const std::string& base, const std::string& relative);
    static size_t curlWriteCallback(void* contents, size_t size, size_t nmemb, std::string* output);

    // member variables
    std::string m_root_link{""};
    int m_depth_level{};
    std::string m_db_credentials{""};
    std::string m_table_name{""};
    int m_verbose{};  // 0 - no logs, 1 - parser logs, 2 - parser and inner threadpool logs

    Threadpool m_threadpool;
    std::vector<LinkData> m_processed;
    std::mutex m_log_mut;
    mutable std::mutex m_processed_mut;
    std::mutex m_db_credentials_mut;

    static std::mutex g_py_mut;
    static std::mutex g_curl_callback_mut;
};
