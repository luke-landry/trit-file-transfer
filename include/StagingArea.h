#ifndef STAGING_AREA_H
#define STAGING_AREA_H

#include <unordered_set>
#include <filesystem>
#include <vector>

class StagingArea{
    public:
        StagingArea();
        void stage_files();
        const std::unordered_set<std::filesystem::path> get_staged_file_paths() const;

    private:
        std::unordered_set<std::filesystem::path> staged_file_paths_;
        std::filesystem::path staging_root_;
        void stage(const std::unordered_set<std::filesystem::path>& file_paths);
        void unstage(const std::unordered_set<std::filesystem::path>& file_paths);
        void list(const std::unordered_set<std::filesystem::path>& file_paths);
        void help();

        std::unordered_set<std::filesystem::path> resolve_file_strs_to_paths(const std::vector<std::string>& file_strs, const std::unordered_set<std::filesystem::path>& file_paths);
};

#endif