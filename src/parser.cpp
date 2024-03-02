#include "../inc/parser.hpp"

#include <Python.h>
#include <cpython/genobject.h>
#include <curl/curl.h>
#include <unistd.h>

#include <cstddef>
#include <exception>
#include <functional>
#include <iostream>
#include <mutex>
#include <regex>
#include <thread>
#include <vector>

#include "../inc/defines.hpp"
#include "../inc/threadpool.hpp"

std::mutex Parser::m_curl_callback_mut;
std::mutex Parser::m_py_mut;

Parser::Parser() { set_threads_number(std::thread::hardware_concurrency()); }
Parser::Parser(int threads_count) { set_threads_number(threads_count); }
Parser::Parser(const std::string& root_link, int depth_level, int threads_count, bool write_to_db, int verbose) {
    set_root_link(root_link);
    set_depth_level(depth_level);
    set_threads_number(threads_count);
    set_logging_to_db(write_to_db);
    set_verbose(verbose);
}

std::vector<LinkData> Parser::get_currently_available() const {
    std::lock_guard<std::mutex> processed_data_lock(m_processed_mut);
    return m_processed;
}

void Parser::set_root_link(const std::string& root_link) { m_root_link = root_link; }
void Parser::set_depth_level(int depth_level) { m_depth_level = depth_level; }
void Parser::set_threads_number(int threads) { m_threadpool.set_threads(threads); }
void Parser::set_logging_to_db(bool write_to_db) { m_write_to_db = write_to_db; }
void Parser::set_verbose(int verbose) { m_verbose = verbose; }

std::string Parser::check_alive_and_get_html(const std::string& link) {
    CURL* curl = curl_easy_init();
    if (!curl) {
        if (m_verbose > 0) {
            std::lock_guard<std::mutex> log_lock(m_log_mut);
            std::cerr << "Error initializing libcurl." << std::endl;
        }
        return "";
    }
    curl_easy_setopt(curl, CURLOPT_URL, link.c_str());
    std::string htmlContent;
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curlWriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &htmlContent);
    sleep(1);
    if (curl_easy_perform(curl) != CURLE_OK) {
        if (m_verbose > 0) {
            std::lock_guard<std::mutex> log_lock(m_log_mut);
            std::cerr << "Failed to fetch URL." << std::endl;
        }
        curl_easy_cleanup(curl);
        return "";
    } else {
    }
    curl_easy_cleanup(curl);

    return htmlContent;
}

std::vector<std::string> Parser::get_urls_from_html(const std::string& html, const std::string& its_url) {
    std::vector<std::string> rez;
    std::regex linkRegex("<a\\s+[^>]*href=['\"]([^'\"]+)['\"][^>]*>");
    auto linkIterator = std::sregex_iterator(html.begin(), html.end(), linkRegex);
    auto endIterator = std::sregex_iterator();
    while (linkIterator != endIterator) {
        std::smatch match = *linkIterator;
        if (isFullURL(match[1].str())) {
            rez.push_back(match[1].str());
        } else {
            rez.push_back(pyUrlJoin(its_url, match[1].str()));
        }
        ++linkIterator;
    }
    return rez;
}

std::vector<LinkData> Parser::parse() {
    auto start_time = std::chrono::high_resolution_clock::now();

    std::function<void(LinkData linkdata)> parse_link = [this, &parse_link](LinkData linkdata) {
        std::vector<std::string> extracted_urls;
        std::string htmlContent = check_alive_and_get_html(linkdata.link);
        if (htmlContent == "") {
            linkdata.alive = 0;
        } else {
            linkdata.alive = 1;
        }
        {
            std::lock_guard<std::mutex> processed_data_lock(m_processed_mut);
            m_processed.push_back(linkdata);
        }
        if (m_verbose > 0) {
            std::lock_guard<std::mutex> log_lock(m_log_mut);
            std::cout << "found: " << linkdata;
        }
        if (linkdata.depth_level < m_depth_level) {
            extracted_urls = get_urls_from_html(htmlContent, linkdata.link);
            for (auto url : extracted_urls) {
                bool already_processed{false};
                {
                    std::lock_guard<std::mutex> processed_data_lock(m_processed_mut);
                    for (auto processed : m_processed) {
                        if (url == processed.link) {
                            already_processed = true;
                            break;
                        }
                    }
                }
                if (!already_processed) {
                    LinkData tmp_linkdata = {linkdata.depth_level + 1, url, linkdata.full_route + " -> " + url, -1};
                    m_threadpool.enque_task(parse_link, tmp_linkdata);
                }
            }
        }
    };

    LinkData root_link_data = {0, m_root_link, m_root_link, -1};
    if (m_verbose > 1) {
        m_threadpool.set_verbose(true);
    }
    m_threadpool.enque_task(parse_link, root_link_data);
    m_threadpool.set_log_mutex(m_log_mut);
    std::thread tasks_thr([&]() { m_threadpool.run_tasks(); });
    m_threadpool.finish_when_empty();

    tasks_thr.join();

    if (m_verbose > 0) {
        auto end_time = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> total_duration = end_time - start_time;
        std::cout << "Total time taken: " << total_duration.count() << " seconds" << std::endl;
    }
    std::cout << "Done parsing.\r\n";

    std::lock_guard<std::mutex> processed_data_lock(m_processed_mut);
    return m_processed;
}

bool Parser::isFullURL(const std::string& url) {
    std::regex urlRegex("^(https?|ftp):\\/\\/(www\\.)?[a-zA-Z0-9-]+(\\.[a-zA-Z]{2,})+(\\/[^\\s]*)?$");
    return std::regex_match(url, urlRegex);
}

std::string Parser::pyUrlJoin(const std::string& base, const std::string& relative) {
    std::lock_guard<std::mutex> lock(m_py_mut);
    std::string rez_link{""};
    Py_Initialize();
    PyObject* urlparseModule = PyImport_ImportModule("urllib.parse");
    if (!urlparseModule) {
        std::cerr << ANSI_RED << "Failed to import urllib.parse module\r\n" << ANSI_RESET;
        return "";
    }
    PyObject* urljoinFunc = PyObject_GetAttrString(urlparseModule, "urljoin");
    if (!urljoinFunc || !PyCallable_Check(urljoinFunc)) {
        std::cerr << ANSI_RED << "Failed to get the urljoin function\r\n" << ANSI_RESET;
        Py_XDECREF(urlparseModule);
        return "";
    }
    PyObject* pyBase = PyUnicode_FromString(base.c_str());
    PyObject* pyRelative = PyUnicode_FromString(relative.c_str());
    PyObject* args = PyTuple_Pack(2, pyBase, pyRelative);
    PyObject* result = PyObject_CallObject(urljoinFunc, args);
    if (result != nullptr) {
        rez_link = PyUnicode_AsUTF8(result);
        Py_DECREF(result);
    } else {
        std::cerr << ANSI_RED << "Failed to call urljoin function\r\n" << ANSI_RESET;
    }
    Py_XDECREF(args);
    Py_XDECREF(pyBase);
    Py_XDECREF(pyRelative);
    Py_XDECREF(urljoinFunc);
    Py_XDECREF(urlparseModule);
    Py_Finalize();
    return rez_link;
}

size_t Parser::curlWriteCallback(void* contents, size_t size, size_t nmemb, std::string* output) {
    std::lock_guard<std::mutex> lock(m_curl_callback_mut);
    output->append(static_cast<char*>(contents), size * nmemb);
    return size * nmemb;
}
