#pragma once
#include <vector>
#include <string>
#include <unordered_map>
#include <iostream>
#include <fstream>
#include <boost/regex.hpp>
#include <memory>

namespace LogParser {

    struct LogDetailNew {
        long id;
        const std::string* thread_name;
        const std::string* file_name;
        std::string prority, dt, content;
    };

    struct LogDetail {
        long id;
        std::string thread_name, prority, dt, content;
    };

    struct LogStats {
        std::unordered_map<std::string, std::shared_ptr<const std::string>> thread_name_map;
        std::unordered_map<std::string, std::shared_ptr<const std::string>> file_name_map;
        std::vector<LogDetailNew> logs;
    };

    struct LoadFileStats {
        bool loading = false;
        int total_file_count = 0;
        int cur_file_count = 0;
        std::string cur_file_name = "";

    };

    const std::vector<LogDetail> load_logs();
    const std::vector<LogDetail> load_files(const std::vector<std::string>& const paths);
    const void load_file(long* id, const std::string const* path, std::vector<LogDetail>* db);

    const LogStats load_logs_new();
    const LogStats load_files_new(const std::vector<std::string>& const paths, LogParser::LoadFileStats* stats);
    const void load_file_new(long* id, const std::string* path, LogStats* stats);

    const std::string getFileName(const std::string& path);
}
