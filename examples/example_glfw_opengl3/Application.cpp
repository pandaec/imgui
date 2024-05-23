#include "Application.h"
#include "imgui.h"
#include <vector>
#include <examples/LogParser/LogParser.h>
#include <iostream>
#include <filesystem>
#include <thread>
#include <mutex>

namespace fs = std::filesystem;

struct FindInfo {
    char filter_text[1024] = { 0 };
    LogParser::LogStats log_stats;
};


class Application
{
private:
    LogParser::LogStats db;
    LogParser::LogStats original_db;
    LogParser::LogStats new_db;
    bool show_demo_window = false;
    bool show_log_window = true;
    bool show_import_window = false;
    long scroll_to_id = -1;
    bool scrolled = true;
    float item_height = -1;
    float clipper_display_item_size = -1;

    // File Loading related
    LogParser::LoadFileStats load_stats;
    LogParser::LoadFileStats original_load_stats;

    FindInfo find_info;

public:
    Application() {}

    void Init() {
#ifdef _DEBUG
        loadDefaultLogs();
#endif
        resetLogWindow();
    }

    void RenderUI()
    {
        ImGui::DockSpaceOverViewport(ImGui::GetMainViewport());

        if (original_db.logs.size() != new_db.logs.size()) {
            original_db = new_db;
            db = new_db;
        }

        if (show_demo_window) {
            ImGui::ShowDemoWindow(&show_demo_window);
        }

        ShowExampleAppMainMenuBar();

        if (show_log_window) {
            ShowLogWindow();
        }

        if (show_import_window) {
            ShowImportWindow();
        }

        ShowLoadingModal();
    }

    void ShowExampleAppMainMenuBar()
    {
        if (ImGui::BeginMainMenuBar()) {
            if (ImGui::BeginMenu("Log")) {
                show_log_window = true;
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Import")) {
                show_import_window = true;
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Demo")) {
                show_demo_window = true;
                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
        }
    }

    void ShowLogWindow()
    {
        ImGui::Begin("Log Viewer", &show_log_window);

        static char filter_str[1024] = "";
        bool scroll_to_top = false;
        if (ImGui::InputTextWithHint("Filter", "Filter", filter_str, IM_ARRAYSIZE(filter_str), ImGuiInputTextFlags_EnterReturnsTrue)) {
            db.logs.clear();
            resetFindWindow();
            scroll_to_top = true;
            for (const LogParser::LogDetailNew& info : original_db.logs) {
                if (strstr(info.prority.c_str(), filter_str)
                    || strstr((*info.thread_name).c_str(), filter_str)
                    || strstr(info.dt.c_str(), filter_str)
                    || strstr(info.content.c_str(), filter_str)) {
                    db.logs.push_back(info);
                }
            }
        }

        ImGui::Spacing();

        static ImVector<long> selection;
        ImGui::Spacing();

        ImGui::BeginChild("ChildL", ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y * 0.7f), ImGuiChildFlags_None, ImGuiWindowFlags_HorizontalScrollbar);

        ImGui::BeginChild("##cliptest", ImVec2(0, 0));

        if (scroll_to_top) {
            ImGui::SetScrollY(0);
        }
        else {
            if (item_height > 0 && clipper_display_item_size > 0 && !scrolled) {
                scrolled = true;

                int target_row;
                for (target_row = 0; target_row < db.logs.size(); target_row++) {
                    if (db.logs[target_row].id == scroll_to_id) {
                        break;
                    }
                }

                float scroll_y;
                if (target_row - (clipper_display_item_size / 2) < 0) {
                    scroll_y = 0;
                }
                else {
                    scroll_y = (target_row - (clipper_display_item_size / 2)) * item_height;
                }

                ImGui::SetScrollY(scroll_y);
            }
        }

        if (ImGui::BeginTable("LogTable", 5, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable))
        {
            ImGui::TableSetupColumn("ID", ImGuiTableColumnFlags_DefaultHide);
            ImGui::TableSetupColumn("Time", ImGuiTableColumnFlags_WidthFixed);
            ImGui::TableSetupColumn("Lv", ImGuiTableColumnFlags_WidthFixed);
            ImGui::TableSetupColumn("Thread", ImGuiTableColumnFlags_WidthFixed);
            ImGui::TableSetupColumn("Content", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableHeadersRow();

            int x = 0;

            ImGuiListClipper clipper;
            clipper.Begin(db.logs.size());
            while (clipper.Step()) {
                for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++)
                {
                    ImGui::TableNextRow();
                    const LogParser::LogDetailNew* d = &db.logs[i];
                    if (ImGui::TableSetColumnIndex(0)) {
                        ImGui::Text("%ld", d->id);
                    }
                    if (ImGui::TableSetColumnIndex(1)) {
                        const bool item_is_selected = selection.contains(d->id) && scroll_to_id != d->id;
                        char label[128];
                        snprintf(label, sizeof(label), "%s##%ld", d->dt.c_str(), d->id);
                        if (ImGui::Selectable(label, item_is_selected, ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowOverlap, ImVec2(0, 0))) {
                            selection.clear();
                            selection.push_back(d->id);
                        }
                    }
                    if (ImGui::TableSetColumnIndex(2)) {
                        ImGui::Text("%s", d->prority.c_str());
                    }
                    if (ImGui::TableSetColumnIndex(3)) {
                        ImGui::Text("%s", d->thread_name->c_str());
                    }
                    if (ImGui::TableSetColumnIndex(4)) {
                        char str[512] = { 0 };
                        const char* src = d->content.c_str();
                        size_t k = 0;
                        while (k < sizeof(str) && src[k] != '\0' && src[k] != '\n') {
                            str[k] = src[k];
                            k++;
                        }
                        ImGui::Text("%s", str);
                    }

                    if (scroll_to_id == d->id) {
                        ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, IM_COL32(255, 255, 0, 128));
                    }

                    if (clipper.ItemsHeight > 0) {
                        item_height = clipper.ItemsHeight;
                        clipper_display_item_size = clipper.DisplayEnd - clipper.DisplayStart;
                    }
                }
            }
            clipper.End();
            ImGui::EndTable();
        }

        ImGui::EndChild();

        ImGui::EndChild();

        ImGui::Spacing();

        static char text[1024 * 1024] = {};
        if (selection.size() > 0) {
            int id = selection[0];
            for (const LogParser::LogDetailNew& info : original_db.logs) {
                if (info.id == id) {
                    std::string s = std::string(*info.file_name);
                    s += "\n";
                    s += info.content;
                    strncpy(text, s.c_str(), 1024 * 1024);
                    break;
                }
            }
        }

        ImGui::BeginChild("ChildL1", ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y),
            ImGuiChildFlags_None, ImGuiWindowFlags_HorizontalScrollbar | ImGuiChildFlags_Border | ImGuiChildFlags_ResizeX);

        if (ImGui::BeginTabBar("##Tabs", ImGuiTabBarFlags_None)) {
            if (ImGui::BeginTabItem("Info")) {
                if (selection.size() > 0) {
                    ImGui::TextWrapped(text);
                }
                else {
                    ImGui::TextWrapped("");
                }
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Raw")) {
                ImGui::InputTextMultiline("##source", text, IM_ARRAYSIZE(text), ImGui::GetContentRegionAvail(), ImGuiInputTextFlags_AllowTabInput);

                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Find")) {
                if (ImGui::InputTextWithHint("Filter", "Filter", find_info.filter_text, IM_ARRAYSIZE(find_info.filter_text), ImGuiInputTextFlags_EnterReturnsTrue)) {
                    find_info.log_stats.logs.clear();

                    for (const LogParser::LogDetailNew& info : original_db.logs) {
                        if (strstr(info.prority.c_str(), filter_str)
                            || strstr((*info.thread_name).c_str(), filter_str)
                            || strstr(info.dt.c_str(), filter_str)
                            || strstr(info.content.c_str(), filter_str)) {

                            if (strstr(info.prority.c_str(), find_info.filter_text)
                                || strstr((*info.thread_name).c_str(), find_info.filter_text)
                                || strstr(info.dt.c_str(), find_info.filter_text)
                                || strstr(info.content.c_str(), find_info.filter_text)) {

                                find_info.log_stats.logs.push_back(info);
                            }
                        }
                    }
                }

                ImGui::Spacing();

                ImGui::BeginChild("ChildFindTab", ImGui::GetContentRegionAvail(), ImGuiChildFlags_None, ImGuiWindowFlags_HorizontalScrollbar);

                if (ImGui::BeginTable("find_table", 5, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable)) {
                    ImGui::TableSetupColumn("ID", ImGuiTableColumnFlags_DefaultHide);
                    ImGui::TableSetupColumn("Time", ImGuiTableColumnFlags_WidthFixed);
                    ImGui::TableSetupColumn("Lv", ImGuiTableColumnFlags_WidthFixed);
                    ImGui::TableSetupColumn("Thread", ImGuiTableColumnFlags_WidthFixed);
                    ImGui::TableSetupColumn("Content", ImGuiTableColumnFlags_WidthStretch);
                    ImGui::TableHeadersRow();

                    ImGuiListClipper clipper;
                    clipper.Begin(find_info.log_stats.logs.size());
                    while (clipper.Step()) {
                        for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++) {
                            ImGui::TableNextRow();
                            const LogParser::LogDetailNew* d = &find_info.log_stats.logs[i];
                            if (ImGui::TableSetColumnIndex(0)) {
                                ImGui::Text("%d", d->id);
                            }

                            if (ImGui::TableSetColumnIndex(1)) {
                                char label[128];
                                snprintf(label, sizeof(label), "%s##%ld", d->dt.c_str(), d->id);
                                if (ImGui::Selectable(label, false, ImGuiSelectableFlags_SpanAllColumns, ImVec2(0, 0))) {
                                    scroll_to_id = d->id;
                                    scrolled = false;
                                    selection.clear();
                                    selection.push_back(d->id);
                                }
                            }

                            if (ImGui::TableSetColumnIndex(2)) {
                                ImGui::Text("%s", d->prority.c_str());
                            }

                            if (ImGui::TableSetColumnIndex(3)) {
                                ImGui::Text("%s", d->thread_name->c_str());
                            }

                            if (ImGui::TableSetColumnIndex(4)) {
                                char str[512] = { 0 };
                                const char* src = d->content.c_str();
                                size_t k = 0;
                                while (k < sizeof(str) && src[k] != '\0' && src[k] != '\n') {
                                    str[k] = src[k];
                                    k++;
                                }
                                ImGui::Text("%s", str);
                            }
                        }
                    }
                    clipper.End();

                    ImGui::EndTable();
                }
                ImGui::EndChild();

                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }

        ImGui::EndChild();

        ImGui::End();
    }

    void ShowImportWindow() {
        ImGui::Begin("Import", &show_import_window);

        static char dir_str[1024] = "D:/Projects/log-parser/WV-ST-20240308/";
        static std::vector<std::string> left_files;
        static std::vector<std::string> right_files;
        if (ImGui::Button("Load")) {
            left_files.clear();
            right_files.clear();

            std::string path = std::string(dir_str);
            for (const auto& entry : fs::directory_iterator(path)) {
                left_files.push_back(entry.path().filename().string());
            }
        }
        ImGui::SameLine();
        ImGui::InputText("Log Directory", dir_str, IM_ARRAYSIZE(dir_str));

        ImGui::BeginChild("left pane", ImVec2(150, 0), ImGuiChildFlags_Border | ImGuiChildFlags_ResizeX);
        if (ImGui::Button("Stage All")) {
            for (int i = 0; i < left_files.size(); i++) {
                right_files.push_back(left_files[i]);
            }
            left_files.clear();
        }

        int remove_i = -1;
        for (int i = 0; i < left_files.size(); i++) {
            if (ImGui::Selectable(left_files[i].c_str(), false)) {
                if (canMoveFileLeftToRight(i, &left_files, &right_files)) {
                    remove_i = i;
                }
            }
        }
        if (remove_i > -1) {
            right_files.push_back(left_files[remove_i]);
            left_files.erase(left_files.begin() + remove_i);
        }

        ImGui::EndChild();

        ImGui::SameLine();

        ImGui::BeginChild("right pane", ImVec2(0, 0), ImGuiChildFlags_Border);

        if (ImGui::Button("Load Logs")) {
            std::vector<std::string> paths;
            std::string dir = std::string(dir_str);
            if (dir.back() != '\\' && dir.back() != '/') {
                dir = dir + "/";
            }
            for (int i = 0; i < right_files.size(); i++) {
                std::string p = dir + right_files[i];
                paths.push_back(p);
            }

            resetLogWindow();
            std::thread writer(&writerThread, std::ref(load_stats), std::ref(new_db), paths);
            writer.detach();
        }

        remove_i = -1;
        for (int i = 0; i < right_files.size(); i++) {
            if (ImGui::Selectable(right_files[i].c_str(), false)) {
                if (canMoveFileLeftToRight(i, &right_files, &left_files)) {
                    remove_i = i;
                }
            }
        }
        if (remove_i > -1) {
            left_files.push_back(right_files[remove_i]);
            right_files.erase(right_files.begin() + remove_i);
        }
        ImGui::EndChild();
        ImGui::End();
    }

    void ShowLoadingModal() {
        if (load_stats.loading) {
            ImGui::OpenPopup("Importing...");
        }

        // Always center this window when appearing
        ImVec2 center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

        if (ImGui::BeginPopupModal("Importing...", NULL, ImGuiWindowFlags_AlwaysAutoResize))
        {
            char s[1024];
            sprintf(s, "Files Loaded: %d/%d", load_stats.cur_file_count, load_stats.total_file_count);
            ImGui::Text(s);
            ImGui::Text(load_stats.cur_file_name.c_str());

            if (!load_stats.loading) {
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }
    }
private:

    bool canMoveFileLeftToRight(size_t from_i, std::vector<std::string> const* from, std::vector<std::string>* const to) {
        std::string const* target = &(from->at(from_i));
        for (size_t i = 0; i < to->size(); i++) {
            if (*target == to->at(i)) {
                return false;
            }
        }
        return true;
    }

    void resetLogWindow() {
        db = {};
        original_db = {};
        resetFindWindow();
    }

    void resetFindWindow() {
        find_info = {};
        scroll_to_id = -1;
        scrolled = true;
    }

#ifdef _DEBUG
    void loadDefaultLogs() {
        std::vector<std::string> paths;
        for (int i = 55; i < 57; i++) {
            char p[1024];
            sprintf(p, "D:/Projects/log-parser/WV-ST-20240308/WV-ST-20240308-%04d.log", i);
            std::string path = std::string(p);
            paths.push_back(path);
        }
        std::thread writer(&writerThread, std::ref(load_stats), std::ref(new_db), paths);
        writer.detach();
    }
#endif

    static void writerThread(LogParser::LoadFileStats& data, LogParser::LogStats& new_db, std::vector<std::string> paths) {
        new_db = LogParser::load_files_new(paths, &data);
    }

    // Temporary c_str case insensitive equality test
    // https://stackoverflow.com/questions/27303062/strstr-function-like-that-ignores-upper-or-lower-case
    static char* stristr(const char* haystack, const char* needle) {
        do {
            const char* h = haystack;
            const char* n = needle;
            while (tolower((unsigned char)*h) == tolower((unsigned char)*n) && *n) {
                h++;
                n++;
            }
            if (*n == 0) {
                return (char*)haystack;
            }
        } while (*haystack++);
        return 0;
    }
};
