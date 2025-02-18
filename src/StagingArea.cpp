
#include <iostream>

#include "StagingArea.h"
#include "utils.h"

StagingArea::StagingArea(): staged_file_paths_(), staging_root_(std::filesystem::current_path()) {};

void StagingArea::stage_files(){

    // Commands are add <file1> ... <fileN>, remove <file1> ... <fileN>, list, transfer, help
    std::cout << "Stage files for transfer (enter 'help' for commands)" << std::endl;
    
    std::string action;
    while(action != "transfer"  && action != "t"){

        // Get user command and split into command tokens
        std::vector<std::string> command_tokens = utils::string_split(utils::input<std::string>());

        // First token is action
        action = command_tokens[0];

        if(action == "stage" || action == "s"){

            std::unordered_set<std::filesystem::path> cwd_file_paths;
            for(const auto& entry : std::filesystem::recursive_directory_iterator(staging_root_)){
                if(std::filesystem::is_regular_file(entry.path())){
                    cwd_file_paths.insert(entry.path());
                }
            }

            stage(
                resolve_file_strs_to_paths(
                    std::vector<std::string>(command_tokens.begin() + 1, command_tokens.end()),
                    cwd_file_paths
                )
            );

        } else if (action == "unstage" || action == "u"){

            unstage(
                resolve_file_strs_to_paths(
                    std::vector<std::string>(command_tokens.begin() + 1, command_tokens.end()),
                    staged_file_paths_
                )
            );

        } else if (action == "list" || action == "l"){

            list(staged_file_paths_);
        
        } else if (action == "help" || action == "h"){
            help();

        } else if (action != "transfer" && action != "t"){
            std::cout << "Unknown command '" << action << "'" << std::endl;
        }
    }
}

const std::unordered_set<std::filesystem::path> StagingArea::get_staged_file_paths() const {
    return staged_file_paths_;
}

// Parses file string names, wildcard * and *.x and elaborates contents of directories in given set of file paths
std::unordered_set<std::filesystem::path> StagingArea::resolve_file_strs_to_paths(const std::vector<std::string>& file_strs, const std::unordered_set<std::filesystem::path>& file_paths){

    std::unordered_set<std::filesystem::path> resolved_paths;

    for(const auto& file_str : file_strs){

        // Wildcard * means all files recursively from root
        if(file_str == "*"){

            // All files and directories are selected, so return all paths
            for(const auto& file_path : file_paths){
                if(std::filesystem::is_regular_file(file_path)){
                    resolved_paths.insert(file_path);
                } else if (std::filesystem::is_directory(file_path)){
                    for(const auto& entry : std::filesystem::recursive_directory_iterator(file_path)){
                        if(std::filesystem::is_regular_file(file_path)){
                            resolved_paths.insert(file_path);
                        }
                    }
                }
            }

            return resolved_paths;

        } else if(file_str == "*.*"){ // Wildcard *.* means all files in root
            for(const auto& file_path : file_paths){

                bool is_at_root_level = (std::filesystem::relative(file_path, staging_root_) == file_path.filename());

                if(is_at_root_level && std::filesystem::is_regular_file(file_path)){
                    resolved_paths.insert(file_path);
                }
            }
        } else if((file_str.size() > 1) && (file_str.substr(0, 2) == "*.")){ // Wildcard *.x means all files with .x extension in root
            std::string extension = file_str.substr(1);
            for(const auto& file_path : file_paths){
                bool is_at_root_level = (std::filesystem::relative(file_path, staging_root_) == file_path.filename());
                if((is_at_root_level) && (file_path.extension() == extension)){
                    resolved_paths.insert(file_path);
                }
            }
        } else { // Is a direct file or directory name
            std::filesystem::path full_path = staging_root_ / file_str;
            if(std::filesystem::exists(full_path)){
                // If item exists, add if file, otherwise add all files recursively within the directory
                if(std::filesystem::is_regular_file(full_path)){
                    resolved_paths.insert(full_path);
                } else if (std::filesystem::is_directory(full_path)){
                    for(const auto& entry : std::filesystem::recursive_directory_iterator(full_path)){
                        if(std::filesystem::is_regular_file(entry)){
                            resolved_paths.insert(entry.path());
                        }
                    }
                }
            } else {
                std::cout << "Warning: " << full_path.string() << " does not exist!" << std::endl;
            }
        }
    }
    return resolved_paths;
}

void StagingArea::stage(const std::unordered_set<std::filesystem::path>& file_paths){
    for(const auto& path : file_paths){
        std::cout << "Staged file: " << std::filesystem::relative(path, staging_root_).string() << std::endl;
        staged_file_paths_.insert(path);
    }
}

void StagingArea::unstage(const std::unordered_set<std::filesystem::path>& file_paths){
    for(const auto& path : file_paths){
        if(staged_file_paths_.erase(path) == 1){
            std::cout << "Unstaged file: " << std::filesystem::relative(path, staging_root_).string() << std::endl;
            
        } else {
            std::cout << "Warning: " << std::filesystem::relative(path, staging_root_).string() << " was not staged" << std::endl;
        }
    }
}

// Lists all files in given file paths
void StagingArea::list(const std::unordered_set<std::filesystem::path>& file_paths){
    for(const auto& path : file_paths){
        std::cout << std::filesystem::relative(path, staging_root_).string() << std::endl;
    }
}

void StagingArea::help(){
    std::cout << "Commands:\n";
    std::cout << "Stage files: stage <file1> <file2> ...\n";
    std::cout << "\tUse * to stage all files in working directory and subdirectories (recursive)\n";
    std::cout << "\tUse *.* to stage all files in the working directory (not recursive)\n";
    std::cout << "\tUse *.x to stage all files in the working directory with the same file extension x (not recursive)\n";
    std::cout << "Unstage files: unstage <file1> <file2>...\n";
    std::cout << "\tUse * to unstage all staged files\n";
    std::cout << "\tUse *.* to unstage all staged files in the working directory\n";
    std::cout << "\tUse *.x to unstage all staged files in the working directory with the same file extension x\n";
    std::cout << "List staged files: list\n";
    std::cout << "Display commands: help\n";
    std::cout << "Send transfer request: transfer\n";
    std::cout << "The stage, unstage, list, help, and transfer commands can also be abbrevated as s, u, l, h, t, respectively" << std::endl;
}