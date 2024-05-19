#include "LogParser.h"

namespace LogParser {

    const boost::regex pat(R"(^\[([A-Z]{3}) ([A-Za-z0-9\s_\-!@#$%^&*()_+|<?.:=\[\]/,]+?),(\d{2}-\d{2}\s\d{2}:\d{2}:\d{2}\.\d{3})\]:(.*)$)");

    const LogStats load_logs_new() {
        std::vector<std::string> paths;
        for (int i = 55; i < 57; i++) {
            char p[1024];
            sprintf(p, "D:/Projects/log-parser/WV-ST-20240308/WV-ST-20240308-%04d.log", i);
            std::string path = std::string(p);
            paths.push_back(path);
        }
        int progress = 0;
        LogParser::LoadFileStats stats = {};
        return load_files_new(paths, &stats);
    }

    const LogStats load_files_new(const std::vector<std::string>& const paths, LogParser::LoadFileStats* stats) {
        stats->loading = true;
        stats->cur_file_count = 0;
        stats->total_file_count = paths.size();
        long id = 0;
        LogStats stat{};
        for (std::string path : paths) {
            stats->cur_file_count += 1;
            stats->cur_file_name = getFileName(path);
            load_file_new(&id, &path, &stat);
        }
        stats->loading = false;
        return stat;
    }
    const void load_file_new(long* id, const std::string* path, LogStats* stats) {
        std::ifstream file(*path);
        if (!file.is_open()) {
            std::cout << "Failed to open the file." << std::endl;
            return;
        }

        std::string line;
        while (std::getline(file, line)) {
            boost::smatch matches;
            if (boost::regex_match(line, matches, pat)) {
                if (matches.size() == 5) {
                    struct LogDetailNew d = {};
                    d.id = *id;
                    d.prority = matches[1];
                    d.dt = matches[3];
                    d.content = matches[4];

                    std::string thread_name = matches[2];

                    auto it = stats->thread_name_map.find(thread_name);
                    if (it == stats->thread_name_map.end()) {
                        stats->thread_name_map[thread_name] = std::make_shared<const std::string>(thread_name);
                        it = stats->thread_name_map.find(thread_name);
                    }
                    d.thread_name = it->second.get();

                    auto it1 = stats->file_name_map.find(*path);
                    if (it1 == stats->file_name_map.end()) {
                        stats->file_name_map[*path] = std::make_shared<const std::string>(*path);
                        it1 = stats->file_name_map.find(*path);
                    }
                    d.file_name = it1->second.get();

                    stats->logs.push_back(d);
                    *id = *id + 1;
                }
            }
            else {
                if (stats->logs.size() > 0) {
                    LogDetailNew* lastItem = &stats->logs.at(stats->logs.size() - 1);
                    lastItem->content = lastItem->content + "\n" + line;
                }
            }
        }

        file.close();
    }

    const std::string getFileName(const std::string& path) {
        size_t pos = path.find_last_of("/\\");
        if (pos == std::string::npos) {
            return path;
        }
        return path.substr(pos + 1);
    }

}
