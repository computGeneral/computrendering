// The confidential and proprietary information contained in this file may 
// only be used by a person authorized under and to the extent permitted   
// by a subsisting licensing agreement from computGeneral Limited.                 
//                 (c) Copyright 2024-2029 computGeneral Limited.                  
//                     ALL RIGHTS RESERVED                                 
// This entire notice must be reproduced on all copies of this file        
// and copies of this file may only be made by a person if such person is  
// permitted to do so under the terms of a subsisting license agreement    
// from computGeneral Limited.                                                     
// 
// Filename        : param_loader.hpp
// Last Revision   : 0.2.0
// Author          : gene@computGeneral.com

#pragma once

#include <cstdlib>
#include <cstring>
#include <mutex>
#include <sstream>
#include <type_traits>
#include <unordered_map>
#include <functional>
#include <map>
#include <vector>
#include <variant>
#include <string>
#include <fstream>
#include <iostream>
#include <cassert>
#include <algorithm>

// Forward declaration so param_loader can populate the legacy config struct.
namespace arch { struct cgsArchConfig; }
using arch::cgsArchConfig;

static inline std::vector<std::string> split(const std::string& s, char delimiter) {
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(s);
    while (std::getline(tokenStream, token, delimiter)) {
        tokens.push_back(token);
    }
    return tokens;
}

/**
 * @brief Singleton ArchParams — reads CG1GPU.csv and provides typed parameter access.
 *
 * Usage:
 *   // Initialize once (e.g. in main / CG1SIM.cpp):
 *   ArchParams::init("path/to/CG1GPU.csv", "CG1GPU.ini");
 *
 *   // Then anywhere in the code:
 *   uint32_t frames = ArchParams::get<uint32_t>("SIMULATOR_SimFrames");
 *   bool     msaa   = ArchParams::get<bool>("SIMULATOR_ForceMSAA");
 *   std::string file = ArchParams::get<std::string>("SIMULATOR_InputFile");
 */
class ArchParams {
public:
    // ---- Singleton access ----
    static ArchParams& instance();

    // ---- Explicit initializer (call once before any get()) ----
    static void init(const std::string& csv_path, const std::string& arch_name = "CG1GPU.ini");

    // ---- Typed parameter accessor ----
    template<typename T>
    static T get(const std::string& param_name) {
        ArchParams& inst = instance();
        auto it = inst.param_map_.find(param_name);
        if (it == inst.param_map_.end() || it->second.empty()) {
            throw std::invalid_argument("[ArchParams] Parameter not found or empty: " + param_name);
        }
        const std::string& val = it->second;

        if constexpr (std::is_same_v<T, bool>) {
            // Handle TRUE/FALSE strings (from INI convention)
            std::string upper = val;
            std::transform(upper.begin(), upper.end(), upper.begin(), ::toupper);
            if (upper == "TRUE")  return true;
            if (upper == "FALSE") return false;
            return static_cast<bool>(std::stoi(val));
        } else if constexpr (std::is_same_v<T, uint32_t>) {
            return static_cast<uint32_t>(std::stoul(val, nullptr, 0));
        } else if constexpr (std::is_same_v<T, int32_t>) {
            return static_cast<int32_t>(std::stoi(val, nullptr, 0));
        } else if constexpr (std::is_same_v<T, uint64_t>) {
            return static_cast<uint64_t>(std::stoull(val, nullptr, 0));
        } else if constexpr (std::is_same_v<T, int64_t>) {
            return static_cast<int64_t>(std::stoll(val, nullptr, 0));
        } else if constexpr (std::is_same_v<T, float>) {
            return std::stof(val);
        } else if constexpr (std::is_same_v<T, double>) {
            return std::stod(val);
        } else if constexpr (std::is_same_v<T, std::string>) {
            return val;
        } else {
            static_assert(!sizeof(T), "ArchParams::get<T>: unsupported type");
        }
    }

    // ---- Convenience: get with default (returns default if key missing) ----
    template<typename T>
    static T get(const std::string& param_name, const T& default_val) {
        ArchParams& inst = instance();
        auto it = inst.param_map_.find(param_name);
        if (it == inst.param_map_.end() || it->second.empty()) {
            return default_val;
        }
        try { return get<T>(param_name); }
        catch (...) { return default_val; }
    }

    // ---- Raw string lookup ----
    const char* operator[](const std::string& param_name) const;

    // ---- Accessors ----
    const std::string& get_current_arch() const { return current_arch_; }
    const std::string& get_file_path()    const { return file_path_; }
    const std::map<std::string, std::string>& get_param_map() const { return param_map_; }
    bool is_initialized() const { return initialized_; }

    // ---- Populate legacy cgsArchConfig from CSV params ----
    void populateArchConfig(cgsArchConfig* arch) const;

    // ---- Override a param value at runtime (e.g. command-line) ----
    void set(const std::string& param_name, const std::string& value);

    // ---- Reverse-populate: build ArchParams singleton from a legacy cgsArchConfig struct ----
    // Call this after ConfigLoader fills ArchConf so the singleton is always available.
    static void initFromArchConfig(const cgsArchConfig& arch);

    // ---- Build a cgsArchConfig struct from the current singleton state ----
    // Useful for passing to sub-module constructors that still expect the struct.
    cgsArchConfig toArchConfig() const;

private:
    ArchParams();  // private: use init() / instance()
    void read_csv_file(const std::string& file_path, const std::string& arch_name);

    std::map<std::string, std::string> param_map_;
    int col_num_;
    std::string current_arch_;
    std::string file_path_;
    bool initialized_;
};
