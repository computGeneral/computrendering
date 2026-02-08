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
// Filename        : ./bhavsim/simulator/common/read_params.h
// Last Revision   : 0.0.1
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
#include <assert.h>
#include <sstream>

template<typename T>
struct str2value;

#define __DEFINE_TRANSFORM(type_name, operation)        \
    template<>                                          \
    struct str2value<type_name> {                       \
        static type_name map(const char* value) {       \
            return (operation);                         \
        }                                               \
    };

__DEFINE_TRANSFORM(std::string, std::string(value))
__DEFINE_TRANSFORM(bool, bool(std::stoi(value)))
__DEFINE_TRANSFORM(std::int32_t, std::stoi(value, nullptr, 0))
__DEFINE_TRANSFORM(std::uint32_t, (std::uint32_t)std::stoull(value, nullptr, 0))
__DEFINE_TRANSFORM(std::int64_t, std::stoll(value, nullptr, 0))

static inline std::vector<std::string> split(const std::string& s, char delimiter) {
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(s);
    while (std::getline(tokenStream, token, delimiter)) {
        tokens.push_back(token);
    }
    return tokens;
}

/*!
 * \brief C++11 style thread-safe lazy initialization singleton class (**NOT** safe in C++98)
 *        refer to: ISO/IEC 14882:2011 6.7 footnote #4
 * \tparam T class type
 */
template<typename T, int NumInstance = 1>
class Singleton {
public:
    template<typename... Args>
    static T& instance(Args&&... args) {
        static T obj(std::forward<Args>(args)...);
        return obj;
    }

    static T& instance(int index = 0) {
        static T obj[NumInstance];
        if (index >= NumInstance) {
            std::cerr << "index out of range:" << index;
            std::abort();
        }
        return obj[index];
    }

    virtual ~Singleton() = default;

    inline explicit Singleton() = default;

public:
    Singleton(const Singleton&) = delete;
    Singleton& operator=(const Singleton&) = delete;
};

/**
 * @brief Read the CSV file and store the data in a map. 
 * 
 * You can directly use ArchParams::get<type>("para_name") to get typed value of the param without declaring object. 
*/
class ArchParams : public Singleton<ArchParams> {
    
    public:
        friend class Singleton<ArchParams>;

        void read_csv_file(std::string file_path);

        inline const char* operator[](const std::string& param_name) const {
            auto it = param_map.find(param_name);
            if (it != param_map.end()) {
                return it->second.c_str();  
            } else {
                return nullptr;  
            }
        }

        std::string get_current_arch() const;
        void set_current_arch(const std::string& arch);

        std::string get_file_path() const;

        inline const std::map<std::string, std::string>& get_param_map() const {
            return param_map;
        }

        template<typename T>
        inline static const T get(const std::string& param_name) {
            ArchParams& instance = ArchParams::instance();
            std::string val_str = instance.param_map[param_name];
            std::istringstream iss(val_str);

            if constexpr (std::is_same_v<T, double>) {
                double val_double;
                if (iss >> val_double && iss.eof()) {
                    return val_double;
                }
            } else if constexpr (std::is_same_v<T, uint32_t>) {
                uint32_t val_uint;
                if (iss >> val_uint && iss.eof()) {
                    return val_uint;
                }
            } else if constexpr (std::is_same_v<T, int64_t>) {
                int64_t val_int;
                if (iss >> val_int && iss.eof()) {
                    return val_int;
                }
            } else if constexpr (std::is_same_v<T, std::string>) {
                return val_str; 
            }
            // you can add more types here if needed

            throw std::invalid_argument("Conversion failed or type not supported.");
        }

    private:
        ArchParams();
        std::map<std::string, std::string> param_map;   // key: param name, value: param value
        int col_num;    // the column number of the current arch
        std::string current_arch;
        std::string arch_file_path;
};
