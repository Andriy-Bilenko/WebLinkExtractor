#pragma once

#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <tuple>
#include <vector>

#include "dataStructs.hpp"

class Parser {
   public:
    // constructors
    Parser() = default;
    explicit Parser(unsigned int threads_count);
    Parser(const std::string& root_link, unsigned int depth_level,
           unsigned int threads_count = std::thread::hardware_concurrency(), bool write_to_db = true,
           bool verbose = false);
    // functions
    std::vector<LinkData> get_currently_available() const;
    void set_root_link(const std::string& root_link);
    void set_depth_level(unsigned int depth_level);
    void set_threads_number(unsigned int threads);

    std::vector<LinkData> parse();  // maybe to a vector or directly to a db connection(use separate soci::session)
    std::string check_alive_and_get_html(const std::string& link);
    std::vector<std::string> get_urls_from_html(const std::string& html);

   private:
    // std::string urlJoin(const std::string& base, const std::string& relative);
    bool isFullURL(const std::string& url);
    // member variables
    std::string m_root_link{""};
    unsigned int m_depth_level{};
    unsigned int m_threads_count{std::thread::hardware_concurrency()};
    bool m_write_to_db{true};
    bool m_verbose{false};
    std::vector<LinkData> m_rows;
    std::queue<LinkData> m_unprocessed_links{};
    std::mutex m_std_cout_mut;
    mutable std::mutex m_rows_mut;
    std::mutex m_no_change_during_parse_mut;
    std::mutex m_py_mut;
};
