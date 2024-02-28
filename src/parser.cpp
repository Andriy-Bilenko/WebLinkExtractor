#include "../inc/parser.hpp"

#include <bits/types/struct_tm.h>
#include <curl/curl.h>
// #include <pybind11/embed.h>
#include <Python.h>
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

std::mutex py_mut;

std::string callPythonUrlJoin(const std::string& base, const std::string& relative) {
    std::lock_guard<std::mutex> lock(py_mut);
    std::string rez_link;

    // Initialize Python interpreter
    Py_Initialize();

    // Import the urlparse module
    PyObject* urlparseModule = PyImport_ImportModule("urllib.parse");
    if (!urlparseModule) {
        std::cerr << "Failed to import urllib.parse module\n";
        return "";
    }

    // Get the urljoin function from urlparse
    PyObject* urljoinFunc = PyObject_GetAttrString(urlparseModule, "urljoin");
    if (!urljoinFunc || !PyCallable_Check(urljoinFunc)) {
        std::cerr << "Failed to get the urljoin function\n";
        Py_XDECREF(urlparseModule);
        return "";
    }

    // Convert C++ strings to Python strings
    PyObject* pyBase = PyUnicode_FromString(base.c_str());
    PyObject* pyRelative = PyUnicode_FromString(relative.c_str());

    // Call urljoin function
    PyObject* args = PyTuple_Pack(2, pyBase, pyRelative);
    PyObject* result = PyObject_CallObject(urljoinFunc, args);

    if (result != nullptr) {
        // Convert result to C++ string
        rez_link = PyUnicode_AsUTF8(result);
        Py_DECREF(result);
    } else {
        std::cerr << "Failed to call urljoin function\n";
    }

    // Clean up
    Py_XDECREF(args);
    Py_XDECREF(pyBase);
    Py_XDECREF(pyRelative);
    Py_XDECREF(urljoinFunc);
    Py_XDECREF(urlparseModule);

    // Finalize Python interpreter
    Py_Finalize();
    return rez_link;
}

std::mutex g_curl_callback_mut;

size_t writeCallback(void* contents, size_t size, size_t nmemb, std::string* output) {
    std::lock_guard<std::mutex> lock(g_curl_callback_mut);
    output->append(static_cast<char*>(contents), size * nmemb);
    return size * nmemb;
}

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
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
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
                std::cout << count << "Link: " << callPythonUrlJoin(m_root_link, match[1].str()) << std::endl;
            }
            rez.push_back(callPythonUrlJoin(m_root_link, match[1].str()));
        }
        ++linkIterator;
        ++count;
    }
    return rez;
}

std::vector<LinkData> Parser::parse() {
    // TODO: finish

    // auto start = std::chrono::high_resolution_clock::now();

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
    const int NUM_THREADS = 6;  // Number of threads
                                // std::vector<std::thread> threads;
    Threadpool pool(NUM_THREADS);
    for (int i = 0; i < NUM_THREADS * 4; ++i) {
        pool.enque_task(parseOnce, root_link_data2);
        // pool.enque_task(parseOnce, root_link_data);
        // pool.enque_task(parseOnce, root_link_data);
    }

    std::thread tasks_thr([&]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(4000));
        pool.stop();
    });
    pool.run_tasks();

    tasks_thr.join();

    // Spawn threads

    // for (int i = 0; i < NUM_THREADS; ++i) {
    //     threads.emplace_back(parseOnce, root_link_data);
    // }
    // for (auto& thread : threads) {
    //     thread.join();
    // }
    //  for (; m_unprocessed_links.size() > 0; m_unprocessed_links.pop()) {
    //      std::clog << "unprocessed: " << m_unprocessed_links.front();
    //  }

    // auto end = std::chrono::high_resolution_clock::now();
    // std::chrono::duration<double> duration = end - start;
    // std::cout << "Total time taken: " << duration.count() << " seconds" << std::endl;
    std::cout << ANSI_BLUE << "parsed.\r\n" << ANSI_RESET;
    return m_rows;
}

/*std::string Parser::urlJoin(const std::string& base, const std::string& relative) {
    m_std_cout_mut.lock();
    std::clog << "441+++++++++++++link\r\n";
    m_std_cout_mut.unlock();
    // std::lock_guard<std::mutex> lock(m_py_mut);
    namespace py = pybind11;
    py::gil_scoped_acquire acquire;  // GIL(global interpreter lock), like a mutex for py
    m_std_cout_mut.lock();
    std::clog << "442+++++++++++++link\r\n";
    m_std_cout_mut.unlock();
    py::object urllib = py::module::import("urllib.parse");
    m_std_cout_mut.lock();
    std::clog << "443+++++++++++++link\r\n";
    m_std_cout_mut.unlock();
    py::object result = urllib.attr("urljoin")(base, relative);
    m_std_cout_mut.lock();
    std::clog << "444+++++++++++++link\r\n";
    m_std_cout_mut.unlock();
    // pybind11::gil_scoped_release release;
    return result.cast<std::string>();
    // std::string a = base + relative;
    // return a;
}*/

bool Parser::isFullURL(const std::string& url) {
    std::regex urlRegex("^(https?|ftp):\\/\\/(www\\.)?[a-zA-Z0-9-]+(\\.[a-zA-Z]{2,})+(\\/[^\\s]*)?$");
    return std::regex_match(url, urlRegex);
}
