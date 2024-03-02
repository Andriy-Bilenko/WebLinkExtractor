#include <curl/curl.h>

#include <iostream>
#include <string>
#include <thread>
#include <vector>

#include "inc/dataStructs.hpp"
#include "inc/parser.hpp"

// run with make run ARGS="https://bilenko.sytes.net"
int main(int argc, const char* argv[]) {
    std::string url{""};

    // TODO: more validity checks
    // TODO: set depth_level threads write_to_db verbose from argv
    if (argc != 2) {
        std::cerr << "usage: ./this_program https://desired_link\r\n";
        return -1;
    } else {
        url = argv[1];
    }
    int depth_level = 2;
    int threads = 60;
    bool write_to_db = false;
    int verbose = 2;

    CURLcode res = curl_global_init(CURL_GLOBAL_ALL);
    if (res != CURLE_OK) {
        std::cerr << "ERROR: CURL global init\r\n";
        return -1;
    }

    Parser parser(url, depth_level, threads, write_to_db, verbose);
    std::vector<LinkData> parse_results = parser.parse();

    curl_global_cleanup();
    return 0;
}

