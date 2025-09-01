#ifndef STAGING_H
#define STAGING_H

#include <filesystem>
#include <string>
#include <unordered_set>
#include <vector>

namespace staging {

void stage(const std::vector<std::string> &file_patterns);
void unstage(const std::vector<std::string> &file_patterns);
void list();
void clear();
void help();

std::unordered_set<std::filesystem::path> get_staged_files();

} // namespace staging

#endif
