#include "../inc/parser.hpp"

#include <Python.h>
#include <curl/curl.h>
#include <unistd.h>

#include <cstddef>
#include <exception>
#include <iostream>
#include <mutex>
#include <regex>
#include <thread>
#include <vector>

#include "../inc/defines.hpp"
#include "../inc/threadpool.hpp"

std::mutex Parser::m_curl_callback_mut;
std::mutex Parser::m_py_mut;

Parser::Parser(unsigned int threads_count) : m_threads_count(threads_count) {}
Parser::Parser(const std::string& root_link, unsigned int depth_level, unsigned int threads_count, bool write_to_db,
               bool verbose)
    : m_root_link(root_link),
      m_depth_level(depth_level),
      m_threads_count(threads_count),
      m_write_to_db(write_to_db),
      m_verbose(verbose) {}

std::vector<LinkData> Parser::get_currently_available() const {
    std::lock_guard<std::mutex> lock(m_rows_mut);
    return m_rows;
}

void Parser::set_root_link(const std::string& root_link) { m_root_link = root_link; }
void Parser::set_depth_level(unsigned int depth_level) { m_depth_level = depth_level; }
void Parser::set_threads_number(unsigned int threads) { m_threads_count = threads; }

std::string Parser::check_alive_and_get_html(const std::string& link) {
    CURL* curl = curl_easy_init();
    if (!curl) {
        if (m_verbose) {
            std::lock_guard<std::mutex> lock(m_std_cout_mut);
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
        if (m_verbose) {
            std::lock_guard<std::mutex> lock(m_std_cout_mut);
            std::cerr << "Failed to fetch URL." << std::endl;
        }
        curl_easy_cleanup(curl);
        return "";
    } else {
    }
    curl_easy_cleanup(curl);

    return htmlContent;
}

std::vector<std::string> Parser::get_urls_from_html(const std::string& html) {
    std::vector<std::string> rez;
    std::regex linkRegex("<a\\s+[^>]*href=['\"]([^'\"]+)['\"][^>]*>");
    auto linkIterator = std::sregex_iterator(html.begin(), html.end(), linkRegex);
    auto endIterator = std::sregex_iterator();
    size_t count{};
    while (linkIterator != endIterator) {
        std::smatch match = *linkIterator;
        if (isFullURL(match[1].str())) {
            if (m_verbose) {
                std::lock_guard<std::mutex> lock(m_std_cout_mut);
                std::cout << count << "Link: " << match[1].str() << std::endl;
            }
            rez.push_back(match[1].str());
        } else {
            if (m_verbose) {
                std::lock_guard<std::mutex> lock(m_std_cout_mut);
                std::cout << count << "Link: " << pyUrlJoin(m_root_link, match[1].str()) << std::endl;
            }
            rez.push_back(pyUrlJoin(m_root_link, match[1].str()));
        }
        ++linkIterator;
        ++count;
    }
    return rez;
}

std::vector<LinkData> Parser::parse() {
    // TODO: finish

    auto start = std::chrono::high_resolution_clock::now();

    auto parseOnce = [this](LinkData linkdata) {
        std::vector<std::string> extracted_urls;
        std::string htmlContent = check_alive_and_get_html(linkdata.link);
        if (htmlContent == "") {
        } else {
            // m_rows.push_back(linkdata);  // verified
            if (linkdata.depth_level < m_depth_level) {
                extracted_urls = get_urls_from_html(htmlContent);
                for (auto url : extracted_urls) {
                    LinkData tmp_linkdata = {linkdata.depth_level + 1, url, linkdata.full_route + " -> " + url, -1};
                    // m_unprocessed_links.push(tmp_linkdata);
                }
            }
        }
    };

    LinkData root_link_data = {0, m_root_link, m_root_link, -1};
    LinkData root_link_data2 = {0, "https://www.google.com", "https://www.google.com", -1};
    const int NUM_THREADS = 60;  // 60

    Threadpool pool(NUM_THREADS);
    for (int i = 0; i < 300; ++i) {  // 300
        pool.enque_task(parseOnce, root_link_data2);
        // pool.enque_task(parseOnce, root_link_data);
        // pool.enque_task(parseOnce, root_link_data);
    }
    std::thread tasks_thr([&]() { pool.run_tasks(); });
    pool.finish_when_empty();

    tasks_thr.join();

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> duration = end - start;
    std::cout << "Total time taken: " << duration.count() << " seconds" << std::endl;
    std::cout << ANSI_BLUE << "parsed.\r\n" << ANSI_RESET;
    return m_rows;
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
        std::cerr << "Failed to import urllib.parse module\n";
        return "";
    }
    PyObject* urljoinFunc = PyObject_GetAttrString(urlparseModule, "urljoin");
    if (!urljoinFunc || !PyCallable_Check(urljoinFunc)) {
        std::cerr << "Failed to get the urljoin function\n";
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
        std::cerr << "Failed to call urljoin function\n";
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
