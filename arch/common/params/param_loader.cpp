#include <filesystem>

#include "param_loader.hpp"

ArchParams::ArchParams() {
    col_num = -1;
    std::filesystem::path arch_root_path = std::filesystem::path(__FILE__).parent_path().parent_path().parent_path();
    std::filesystem::path arch_param_path = arch_root_path / "common" / "params" / "arch_params.csv";
    std::string arch_param = arch_param_path.string();


    const char *params = arch_param.c_str();

    if (params == NULL ) {
        throw std::runtime_error("[ARCHSIM] arch param file is not set");
    }

    arch_file_path = params;
    current_arch = "6.0";  // default arch version, can be changed later
    
    read_csv_file(arch_file_path);
    
    std::cout << "[ARCHSIM] Read params from " << arch_file_path << std::endl;
}

void ArchParams::read_csv_file(std::string file_path) {
    std::ifstream file(file_path);
    if (!file.is_open()) {
        std::cerr << "[ARCHSIM] Error: Unable to open param file " << file_path;
        return;
    }
    std::string line;

    //std::getline(file, line);   // skip first line(there is no header line in new params.csv file)
    std::getline(file, line);   // process the second line
    // process the second line to find the column number of this arch version
    auto split_line = split(line, ',');
    for (int i = 0; i < split_line.size(); i++) {
        if (split_line[i] == current_arch) {
            col_num = i;
            break;
        }
    }
    // if cannot find the arch version, throw an error
    assert(col_num != -1 && "arch version not found in csv file");

    while (std::getline(file, line)) {
        split_line = split(line, ',');
        param_map[split_line[6]] = split_line[col_num];
    }
}


std::string ArchParams::get_current_arch() const {
    return current_arch;
} 

void ArchParams::set_current_arch(const std::string& arch) {
    current_arch = arch;
}

std::string ArchParams::get_file_path() const {
    return arch_file_path;
}