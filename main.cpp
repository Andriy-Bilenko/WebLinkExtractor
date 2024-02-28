#include <curl/curl.h>
// #include <pybind11/embed.h>

#include <iostream>
#include <string>
#include <thread>

#include "inc/parser.hpp"

// run with make run ARGS="https://bilenko.sytes.net"
int main(int argc, const char* argv[]) {
    std::string url{""};

    if (argc != 2) {
        std::cerr << "usage: ./this_program https://desired_link\r\n";
        return -1;
    } else {
        url = argv[1];
    }
    unsigned int depth_level = 2;
    unsigned int threads = 12;
    bool write_to_db = false;
    bool verbose = true;

    CURLcode res = curl_global_init(CURL_GLOBAL_ALL);
    if (res != CURLE_OK) {
        std::cerr << "ERROR: CURL global init\r\n";
        return -1;
    }

    // pybind11::scoped_interpreter guard{};  // Initialize Python interpreter
    // pybind11::scoped_interpreter interp;

    Parser parser(url, depth_level, threads, write_to_db, verbose);
    std::thread parser_thread{[&parser]() { parser.parse(); }};

    parser_thread.join();

    curl_global_cleanup();
    return 0;
}

