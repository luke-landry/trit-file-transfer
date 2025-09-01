
#include "staging.h"
#include <fstream>
#include <iostream>
#include <regex>
#include "utils.h"

// Anonymous namespace to hide internal staging utility functions
namespace {

const std::filesystem::path& get_staging_file_path() {
    static const std::filesystem::path path =
        std::filesystem::current_path() / ".trit" / "staged.txt";
    return path;
}

const std::filesystem::path& get_staging_lock_path() {
    static const std::filesystem::path path =
        std::filesystem::current_path() / ".trit" / ".lock";
    return path;
}

bool lock_staging_file() {
    std::error_code ec;

    std::filesystem::create_directories(get_staging_lock_path().parent_path(), ec);
    if (ec) {
        std::cerr << "Failed to create .trit directory: " << ec.message() << '\n';
        return false;
    }

    // Using a lock directory because directory existence check and creation are
    // atomic on Linux

    // Try to atomically create the lock directory
    if (std::filesystem::create_directory(get_staging_lock_path(), ec)) {
        return true; // acquired lock
    }

    if (ec == std::errc::file_exists) {
        return false; // someone else holds the lock
    }

    std::cerr << "Failed to create lock directory: " << ec.message() << '\n';
    return false;
}

void unlock_staging_file() {
    std::error_code ec;
    std::filesystem::remove_all(get_staging_lock_path(), ec);
    if (ec && ec != std::errc::no_such_file_or_directory) {
        std::cerr << "Warning: Failed to remove staging lock: " << ec.message() << '\n';
    }
}

class StagingLock {
public:
    StagingLock() {
        if (!lock_staging_file()) {
            throw std::runtime_error("Staging file is already locked");
        }
    }
    
    ~StagingLock() {
        unlock_staging_file();
    }
    
    // Non-copyable, non-movable
    StagingLock(const StagingLock&) = delete;
    StagingLock& operator=(const StagingLock&) = delete;
};

void delete_staging_files() {
    StagingLock lock;
    std::error_code ec;
    std::filesystem::remove_all(get_staging_file_path().parent_path(), ec);
    if (ec && ec != std::errc::no_such_file_or_directory) {
        std::cerr << "Warning: Failed to remove .trit directory: " << ec.message() << '\n';
    }
}

std::unordered_set<std::filesystem::path> load_staged_files(){
    std::error_code ec;
    std::filesystem::create_directories(get_staging_file_path().parent_path(), ec);
    if (ec) {
        throw std::runtime_error("Failed to create .trit directory: " + ec.message());
    }

    StagingLock lock;

    std::unordered_set<std::filesystem::path> staged_files;
    std::ifstream infile(get_staging_file_path());

    if (!infile) {
        return staged_files;
    }

    std::string line;
    while (std::getline(infile, line)) {
        if (!line.empty()) {
            std::error_code ec;
            std::filesystem::path file_path(line);
            if(std::filesystem::exists(file_path, ec)){
                staged_files.insert(file_path);
            } else if (ec){
                std::cerr << "error loading path '" << file_path.string() << "': " << ec.message() << std::endl;
            } else {
                std::cerr << "staged file " << file_path.string() << " no longer exists" << std::endl;
            }
        }
    }

    if(staged_files.empty()) {
        delete_staging_files();
    }

    return staged_files;
}

void save_staged_files(const std::unordered_set<std::filesystem::path>& staged_files){
    std::error_code ec;

    std::filesystem::create_directories(get_staging_file_path().parent_path(), ec);
    if (ec) {
        throw std::runtime_error("Failed to create .trit directory: " + ec.message());
    }

    StagingLock lock;

    std::ofstream outfile(get_staging_file_path(), std::ios::trunc);
    if (!outfile) {
        throw std::runtime_error("Failed to open staging file for writing");
    }

    for (const auto& path : staged_files) {
        outfile << path.string() << '\n';
    }
}

void list_files(const std::unordered_set<std::filesystem::path>& files){
    for(const auto& file : files){
        std::cout << "  " << utils::relative_to_cwd(file) << '\n';
    }
}

std::unordered_set<std::filesystem::path> get_files_in_cwd(){
    std::unordered_set<std::filesystem::path> cwd_file_paths;
    for(const auto& entry : std::filesystem::recursive_directory_iterator(std::filesystem::current_path())){
        if(std::filesystem::is_regular_file(entry.path())){
            if (entry.path() == get_staging_file_path()) {
                continue; // skip staging.txt metadata
            }
            cwd_file_paths.insert(entry.path());
        }
    }
    return cwd_file_paths;
}

std::string file_pattern_to_regex(const std::string& file_pattern){

    // Selection patterns
    // name.extension - specific file
    // *.extension - all files with given extension
    // name.* - all files with given name
    // * - all files in current directory
    // ** - all files in current and subdirectories
    // ? - single character wildcard

    // User can specify a subdirectory and then selection pattern
    // e.g. my/directory/*.c
    
    std::string regex = "^";
    std::string::size_type pos = 0;

    while(pos < file_pattern.size()){
        char c = file_pattern[pos];

        if (c == '*') {
            if (pos + 1 < file_pattern.size() && file_pattern[pos + 1] == '*') {
                // ** pattern
                regex += ".*";
                ++pos;
            } else {
                regex += "[^/]*";
            }
        } else if (c == '?') {
            regex += '.';
        } else if (c == '.') {
            regex += "\\.";
        } else if (c == '/') {
            regex += "/";
        } else if (c == '\\') { // handle user backslash
            regex += "\\\\";
        } else if (std::isalnum(static_cast<unsigned char>(c))) {
            regex += c;
        } else {
            // escape any other special characters
            regex += '\\';
            regex += c;
        }

        ++pos;
    }

    regex += "$";
    return regex;
}

// Matches set of file paths against file patterns and returns matching elements relative to cwd
std::unordered_set<std::filesystem::path> match_file_patterns_to_paths(const std::vector<std::string>& file_patterns, const std::unordered_set<std::filesystem::path>& file_paths){
    // File paths are assumed to be correctly formatted, since they are always retrieved via std::filesystem
    std::unordered_set<std::filesystem::path> matched_paths;
    for(const auto& file_pattern : file_patterns){
        std::regex regex(file_pattern_to_regex(file_pattern));
        for(const auto& file_path : file_paths){
            if(std::regex_match(utils::relative_to_cwd(file_path).generic_string(), regex)){
                matched_paths.insert(file_path);
            }
        }
    }
    return matched_paths;
}

std::vector<std::string> normalize_patterns(const std::vector<std::string>& input_patterns) {
    std::vector<std::string> expanded;
    for (const auto& pattern : input_patterns) {
        std::filesystem::path p(pattern);
        std::error_code ec;
        if (std::filesystem::is_directory(p, ec) && !ec) {
            // Convert to recursive pattern
            expanded.push_back((p / "**").generic_string());
        } else {
            expanded.push_back(pattern);
        }
    }
    return expanded;
}

} // anonymous namespace

namespace staging {
    
void stage(const std::vector<std::string>& file_patterns){
    auto staged_files = load_staged_files();
    const auto files_in_cwd = get_files_in_cwd();
    const auto normalized_patterns = normalize_patterns(file_patterns);
    auto files_to_stage = match_file_patterns_to_paths(normalized_patterns, files_in_cwd);
    if(files_to_stage.empty()){
        std::cout << "No files in current directory matched the provided pattern(s)" << std::endl;
        if(staged_files.empty()){
            delete_staging_files();
        }
        return;
    }
    for(auto it = files_to_stage.begin(); it != files_to_stage.end();){
        const auto& file_to_stage = *it;
        if(staged_files.find(file_to_stage) != staged_files.end()){
            std::cout << "already staged:\t" << std::filesystem::relative(file_to_stage, std::filesystem::current_path()) << std::endl;
            it = files_to_stage.erase(it);
            continue;
        }
        staged_files.insert(file_to_stage);
        ++it;
    }
    if(files_to_stage.empty()){
        std::cout << "All matching files were already staged, no new files added" << std::endl;
        return;
    }
    save_staged_files(staged_files);
    std::cout << "Added staged files:" << std::endl;
    list_files(files_to_stage);
}

void unstage(const std::vector<std::string>& file_patterns){
    auto staged_files = load_staged_files();
    const auto normalized_patterns = normalize_patterns(file_patterns);
    const auto files_to_unstage = match_file_patterns_to_paths(normalized_patterns, staged_files);
    if(files_to_unstage.empty()){
        std::cout << "No staged files matched the provided pattern(s)" << std::endl;
        return;
    }
    for (const auto& file : files_to_unstage){ staged_files.erase(file); }

    if(staged_files.empty()){
        delete_staging_files();
        return;
    }

    save_staged_files(staged_files);
    std::cout << "Dropped staged files:" << std::endl;
    list_files(files_to_unstage);
}

void list(){
    auto staged_files = load_staged_files(); 
    if(staged_files.empty()){
        std::cout << "No files are currently staged." << std::endl;
        delete_staging_files();
        return;
    }
    std::cout << "Staged files:" << std::endl;
    list_files(staged_files);
}

void clear(){
    std::error_code ec;
    const auto path = get_staging_file_path();

    if (!std::filesystem::exists(path, ec)) {
        if (!ec) {
            std::cout << "No staged files to clear" << std::endl;
        } else {
            std::cerr << "Error checking staging file existence: " << ec.message() << std::endl;
        }
        return;
    }

    delete_staging_files();
}

void help() {
    std::cout << "Commands:\n";
    std::cout << "  trit add <file_pattern>...          Stage file(s) for transfer\n";
    std::cout << "  trit drop <file_pattern>...         Unstage previously staged file(s)\n";
    std::cout << "  trit list                           List currently staged files\n";
    std::cout << "  trit send <ip> <port> [password]    Send a file transfer request to a receiver\n";
    std::cout << "  trit receive [password]             Start listening for incoming file transfers\n\n";
    std::cout << "  trit help                           Display this help message\n";

    std::cout << "File pattern syntax:\n";
    std::cout << "  *.ext           Matches all files with the given extension in the current directory\n";
    std::cout << "  name.*          Matches all files with the given base name and any extension in the current directory\n";
    std::cout << "  *               Matches all files in the current directory\n";
    std::cout << "  **              Matches all files recursively in the current directory and subdirectories\n";
    std::cout << "  path/to/*.ext   Matches all files with the given extension in the specified subdirectory\n";
    std::cout << "  ?               Matches any single character (e.g., 'f?o.txt' matches 'foo.txt', 'fao.txt')\n\n";
}

std::unordered_set<std::filesystem::path> get_staged_files(){
    return load_staged_files();
}

} // namespace staging