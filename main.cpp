#include <curl/curl.h>
#include <soci/into.h>
#include <soci/postgresql/soci-postgresql.h>
#include <soci/soci.h>

#include <exception>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

#include "inc/dataStructs.hpp"
#include "inc/parser.hpp"

// run with make run ARGS="https://bilenko.sytes.net"
// or with make run ARGS="-d 2 -t 60 -dbt table_name2 -db \"dbname=projecta user=user1 password=user1 hostaddr=127.0.0.1
// port=5432\" www.google.com"
int main(int argc, const char* argv[]) {
    std::string url = argv[argc - 1];
    int depth_level = 1;
    int threads = std::thread::hardware_concurrency();
    int verbose = 1;
    std::string db_credentials = {""};
    std::string table_name = {""};

    std::string usage_prompt =
        "usage:" + std::string(argv[0]) +
        " [options] https://desired_link\r\n\r\n"
        "OPTIONS\r\n"
        "-h, --help\t\t\t show list of command-line options\r\n"
        "-d, --depth DEPTH\t\t limit the depth of recursion\r\n"
        "-t, --threads THREADS\t\t set number of concurrent processing threads manually\r\n"
        "-db \"database credentials\"\t log output to the database,\r\n"
        "\t\t\t\t example: -db \"dbname=dbname user=user password=password hostaddr=127.0.0.1 port=5432\"\r\n"
        "-dbt, --db_table TABLE_NAME\t set the table name for logging to database\r\n"
        "-vb --verbose VERBOSE\t\t 0 - no loggging, 1 - output links, 2 - log internal threadpool\r\n";

    std::vector<std::pair<std::string, std::string>> flags = {{"-h", "--help"},       {"-d", "--depth"},
                                                              {"-t", "--threads"},    {"-db", "-db"},
                                                              {"-dbt", "--db_table"}, {"-vb", "--verbose"}};
    if (argc < 2 || std::string(argv[1]) == flags.at(0).first || std::string(argv[1]) == flags.at(0).second ||
        (argc % 2)) {
        std::cerr << usage_prompt;
        return -1;
    }

    for (int i = 1; i < argc - 1; i += 2) {
        std::string flag = argv[i];
        std::string argument = argv[i + 1];
        if (flag == flags.at(1).first || flag == flags.at(1).second) {  // depth
            try {
                depth_level = std::stoi(argument);
            } catch (const std::exception& e) {
                std::cerr << "ERROR, wrong argument:" << argument << "\r\n";
                return -1;
            }
        } else if (flag == flags.at(2).first || flag == flags.at(2).second) {  // threads
            try {
                threads = std::stoi(argument);
            } catch (const std::exception& e) {
                std::cerr << "ERROR, wrong argument:" << argument << "\r\n";
                return -1;
            }
        } else if (flag == flags.at(3).first || flag == flags.at(3).second) {  // db
            db_credentials = argument;
        } else if (flag == flags.at(4).first || flag == flags.at(4).second) {  // db_table
            table_name = argument;
        } else if (flag == flags.at(5).first || flag == flags.at(5).second) {  // verbose
            try {
                verbose = std::stoi(argument);
            } catch (const std::exception& e) {
                std::cerr << "ERROR, wrong argument:" << argument << "\r\n";
                return -1;
            }
        } else {
            std::cerr << usage_prompt;
            return -1;
        }
    }

    CURLcode res = curl_global_init(CURL_GLOBAL_ALL);
    if (res != CURLE_OK) {
        std::cerr << "ERROR: CURL global init\r\n";
        return -1;
    }

    Parser parser(url, depth_level, threads, verbose);
    parser.set_sql_connection(db_credentials, table_name);

    std::vector<LinkData> parse_results = parser.parse();

    // uncomment lines below to query the db to show alive links:
    /*try {
        soci::session sql(soci::postgresql, db_credentials);
        int id{};
        int depth_level{};
        std::string link{""};
        std::string full_route{""};
        int alive{};

        soci::rowset<soci::row> rows =
            (sql.prepare << "SELECT id, depth_level, link, full_route, alive FROM " + table_name + " WHERE alive = 1");

        for (soci::rowset<soci::row>::const_iterator it = rows.begin(); it != rows.end(); ++it) {
            soci::row const& row = *it;
            id = row.get<int>(0);
            depth_level = row.get<int>(1);
            link = row.get<std::string>(2);
            full_route = row.get<std::string>(3);
            alive = row.get<int>(4);

            std::cout << "id:" << id << " alive:" << alive << " depth_level:" << depth_level << " link:" << link
                      << "\r\n";
        }
    } catch (const soci::soci_error& e) {
        std::cerr << "SOCI Error: " << e.what() << "\r\n";
        return -1;
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\r\n";
        return -1;
    } catch (...) {
        std::cerr << "UNKNOWN ERROR\r\n";
        return -1;
    }*/

    curl_global_cleanup();
    return 0;
}

