#include "Application.h"
#include "imgui.h"
#include <vector>
#include <examples/LogParser/LogParser.h>
#include <iostream>
#include <filesystem>
#include <thread>
#include <mutex>

namespace fs = std::filesystem;

class Application
{
private:
    LogParser::LogStats db;
    LogParser::LogStats original_db;
    LogParser::LogStats new_db;
    int pageCount = 0;
    const int PAGE_SIZE = 1000;
    bool show_demo_window = false;
    bool show_log_window = true;
    bool show_import_window = false;

    // File Loading related
    LogParser::LoadFileStats load_stats;
    LogParser::LoadFileStats original_load_stats;

public:
    Application() {}

    void Init() {
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

    void RenderUI()
    {
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
        if (ImGui::InputTextWithHint("filter", "enter text here", filter_str, IM_ARRAYSIZE(filter_str), ImGuiInputTextFlags_EnterReturnsTrue)) {
            db.logs.clear();
            for (const LogParser::LogDetailNew& info : original_db.logs) {
                if (strstr(info.prority.c_str(), filter_str)
                    || strstr((*info.thread_name).c_str(), filter_str)
                    || strstr(info.dt.c_str(), filter_str)
                    || strstr(info.content.c_str(), filter_str)) {
                    db.logs.push_back(info);
                }
            }
            pageCount = 0;
        }

        ImGui::Spacing();

        ImGui::Text("Page:");
        ImGui::SameLine();
        ImGui::Text("%d", pageCount);
        ImGui::SameLine();

        float spacing = ImGui::GetStyle().ItemInnerSpacing.x;
        ImGui::PushButtonRepeat(true);
        if (ImGui::ArrowButton("##left", ImGuiDir_Left)) { prevPage(); }
        ImGui::SameLine(0.0f, spacing);
        if (ImGui::ArrowButton("##right", ImGuiDir_Right)) { nextPage(); }
        ImGui::PopButtonRepeat();

        static ImVector<long> selection;
        ImGui::Spacing();

        ImGui::BeginChild("ChildL", ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y * 0.7f), ImGuiChildFlags_None, ImGuiWindowFlags_HorizontalScrollbar);

        static ImGuiTableFlags flags = ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable;

        if (ImGui::BeginTable("table1", 5, flags))
        {
            ImGui::TableSetupColumn("AAA", ImGuiTableColumnFlags_WidthFixed);
            ImGui::TableSetupColumn("AAA", ImGuiTableColumnFlags_WidthFixed);
            ImGui::TableSetupColumn("BBB", ImGuiTableColumnFlags_WidthFixed);
            ImGui::TableSetupColumn("CCC", ImGuiTableColumnFlags_WidthFixed);
            ImGui::TableSetupColumn("DDD", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableHeadersRow();
            int maxSize = 1000 + pageCount * PAGE_SIZE < db.logs.size() ? 1000 + pageCount * PAGE_SIZE : db.logs.size();
            int x = 0;

            for (int i = pageCount * PAGE_SIZE; i < maxSize; i++)
            {
                ImGui::TableNextRow();
                const LogParser::LogDetailNew* d = &db.logs[i];
                if (ImGui::TableSetColumnIndex(0)) {
                    const bool item_is_selected = selection.contains(d->id);
                    ImGuiSelectableFlags selectable_flags = ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowOverlap;
                    char str[32];
                    sprintf(str, "%ld", d->id);
                    if (ImGui::Selectable(str, item_is_selected, selectable_flags, ImVec2(0, 0))) {
                        selection.clear();
                        selection.push_back(d->id);
                    }
                }
                if (ImGui::TableSetColumnIndex(1)) {
                    ImGui::Text("%s", d->prority.c_str());
                }
                if (ImGui::TableSetColumnIndex(2)) {
                    ImGui::Text("%s", d->thread_name->c_str());
                }
                if (ImGui::TableSetColumnIndex(3)) {
                    ImGui::Text("%s", d->dt.c_str());
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
            ImGui::EndTable();
        }

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

            if (ImGui::BeginTabItem("Text")) {
                ImGui::InputTextMultiline("##source", text, IM_ARRAYSIZE(text), ImGui::GetContentRegionAvail(), ImGuiInputTextFlags_AllowTabInput);

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
            for (int i = 0; i < right_files.size(); i++) {
                std::string p = std::string(dir_str) + right_files[i];
                paths.push_back(p);
            }

            pageCount = 0;
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
    inline void nextPage() {
        if ((pageCount + 1) * PAGE_SIZE < db.logs.size()) {
            pageCount = pageCount + 1;
        }
    }

    inline void prevPage() {
        if (pageCount - 1 >= 0) {
            pageCount = pageCount - 1;
        }
    }

    bool canMoveFileLeftToRight(size_t from_i, std::vector<std::string> const* from, std::vector<std::string>* const to) {
        std::string const* target = &(from->at(from_i));
        for (size_t i = 0; i < to->size(); i++) {
            if (*target == to->at(i)) {
                return false;
            }
        }
        return true;
    }

    static void writerThread(LogParser::LoadFileStats& data, LogParser::LogStats& new_db, std::vector<std::string> paths) {
        new_db = LogParser::load_files_new(paths, &data);
    }
};
