/**************************************************************************
 * Parser definition file.
 * This file defines the Tracer class used as a profiling tool.
 *
 */

#include "GPUType.h"

#include <map>
#include <vector>
#include <set>
#include <cmath>
#include <regex>
#include <algorithm>
#include <fstream>
#include <filesystem>
#include <cassert>

using namespace std;

#ifndef _PROFILER_
#define _PROFILER_

// Macros for use within code so it gets compiled out

// Function prototypes
namespace cg1gpu
{

class Tracer
{

private:

    struct TraceRegion
    {
        U64 ticks;
        U64 visits;
        U64 ticksTotal;
        U64 visitsTotal;
        string regionName;
        string className;
        string functionName;
    };

    map<string, TraceRegion *> traceRegions;
    vector<TraceRegion *> regionStack;
    U32 regionStackSize;
    U64 lastTickSample;
    U64 allTicks;
    U64 allTicksTotal;
    TraceRegion *currentTraceRegion;
    static U64 sampleTickCounter();
    U32 lastProcessor;
    bool differentProcessors;
    U32 processorChanges;
    
public:
    Tracer();
    ~Tracer();
    void reset();
    void enterRegion(char *regionName, char *className, char *functionName);
    void exitRegion();
    void generateReport(char *fileName);
};



//=============================================================================
// Forward declarations
//=============================================================================
class TestBench;

//=============================================================================
// MetricType - Classification flags for metrics
//=============================================================================
enum class MetricType : uint32_t {
    NONE = 0,
    CYCLE = 1 << 0,
    CYCLE_DETAIL = 1 << 1,
    CYCLE_ACTIVE = 1 << 2,
    CYCLE_BUSY = 1 << 3,
    WORKLOAD = 1 << 4,
    WORKLOAD_TRAFFIC = 1 << 5,
    WORKLOAD_INSTR_NUM = 1 << 6,
    RATIO = 1 << 7
};

inline MetricType operator|(MetricType a, MetricType b) {
    return static_cast<MetricType>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

inline MetricType operator&(MetricType a, MetricType b) {
    return static_cast<MetricType>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
}

inline bool has_flag(MetricType flags, MetricType flag) {
    return (static_cast<uint32_t>(flags) & static_cast<uint32_t>(flag)) != 0;
}

//=============================================================================
// MetricGroup - Groups metrics by tags
//=============================================================================
class MetricGroup {
public:
    void add(const std::string& name, const std::set<MetricType>& tags) {
        for (auto tag : tags) {
            group_[tag].insert(name);
        }
    }

    std::set<std::string> get(MetricType tag) const {
        auto it = group_.find(tag);
        return it != group_.end() ? it->second : std::set<std::string>{};
    }

    std::set<MetricType> get_tags(const std::string& key) const {
        std::set<MetricType> tags;
        for (const auto& [tag, keys] : group_) {
            if (keys.count(key)) {
                tags.insert(tag);
            }
        }
        return tags;
    }

private:
    std::map<MetricType, std::set<std::string>> group_;
};

//=============================================================================
// Metric - Metric collector
//=============================================================================
class Metric {
public:
    void reset() {
        signals_.clear();
    }

    void profiling(const std::string& key, int64_t value = 1) {
        signals_[key] += value;
    }

    void add_metric(const std::set<MetricType>& tags, const std::string& key, int64_t value) {
        groups_.add(key, tags);
        signals_[key] = value;
    }

    int64_t get_data(const std::string& key) const {
        auto it = signals_.find(key);
        return it != signals_.end() ? it->second : 0;
    }

    std::set<MetricType> get_tags(const std::string& key) const {
        return groups_.get_tags(key);
    }

    const std::map<std::string, int64_t>& signals() const { return signals_; }
    std::map<std::string, int64_t>& signals() { return signals_; }

    void add_metrics(const std::map<std::string, int64_t>& data, std::set<MetricType> tags) {
        // Add CYCLE tag if any cycle-related tag is present
        if (tags.count(MetricType::CYCLE_ACTIVE) || 
            tags.count(MetricType::CYCLE_BUSY) || 
            tags.count(MetricType::CYCLE_DETAIL)) {
            tags.insert(MetricType::CYCLE);
        }
        for (const auto& [key, value] : data) {
            add_metric(tags, key, value);
        }
    }

private:
    std::map<std::string, int64_t> signals_;
    MetricGroup groups_;
};

//=============================================================================
// Profiler - Main profiler class
//=============================================================================
class Profiler {
public:
    Profiler(TestBench* tb, const std::string& output_file = "")
        : tb_(tb)
        , output_file_(output_file)
        , cycle_(0)
    {
        clean_legacy_profilings();
    }

    void reset_per_cycle() {
        status_custom_.clear();
    }

    /**
     * @brief Set custom status flag
     * When using this API, call profiling_statistic() at end of each cycle
     */
    void set_statistic(const std::string& key, int64_t value = 1) {
        if (profiling_enabled()) {
            status_custom_[key] = value;
        }
    }

    /**
     * @brief Profile custom status/counters
     */
    void profiling_statistic() {
        if (profiling_enabled()) {
            for (const auto& [key, value] : status_custom_) {
                profiling(key, value);
            }
            reset_per_cycle();
        }
    }

    void profiling(const std::string& key, int64_t value = 1) {
        if (profiling_enabled()) {
            metric_sample_.profiling(key, value);
        }
    }

    int64_t get_metric_value(const std::string& key) const {
        return metric_sample_.get_data(key);
    }

    void set_current_cycle(uint64_t cycle) {
        cycle_ = cycle;
    }

    /**
     * @brief Collect metric samples for current profiler
     */
    void collect_samples(const std::string& module) {
        if (!profiling_enabled()) return;

        uint64_t sample_duration = get_sample_duration();
        size_t exist_samples = static_cast<size_t>(std::ceil(
            static_cast<double>(cycle_) / sample_duration));

        // Clean module name (remove "GPU." prefix, replace dots with underscores)
        std::string clean_module = std::regex_replace(module, std::regex(R"(^GPU\.)"), "");
        std::replace(clean_module.begin(), clean_module.end(), '.', '_');

        for (const auto& [key, value] : metric_sample_.signals()) {
            std::string full_key = clean_module + "_" + key;

            if (key.find("ACTIVE_CYCLE") != std::string::npos) {
                // Validate: value should not exceed sample duration
                assert(value <= sample_duration);
            }

            if (metric_collection_.count(full_key)) {
                metric_collection_[full_key].push_back(value);
            } else {
                // Initialize with zeros for previous samples
                metric_collection_[full_key] = std::vector<int64_t>(exist_samples - 1, 0);
                metric_collection_[full_key].push_back(value);
            }
        }

        // Pad all metrics to same length
        for (auto& [key, values] : metric_collection_) {
            while (values.size() < exist_samples) {
                values.push_back(0);
            }
        }

        metric_sample_.reset();
    }

    void sample_key_check() {
        // Check that all sample keys are defined in counter definition
        for (const auto& [key, _] : metric_sample_.signals()) {
            if (!counter_def_check(key)) {
                // Warning: key not in counter_definition.csv
            }
        }
    }

    bool counter_def_check(const std::string& key) const {
        // Check if key matches counter definitions
        // Simplified - actual implementation would check against counter_df
        if (key.find("INSTR_EXEC_NUM_") != std::string::npos ||
            key.find("INSTR_DISP_NUM_") != std::string::npos) {
            return true;
        }
        return true; // Simplified
    }

    void clean_legacy_profilings() {
        if (!output_file_.empty() && std::filesystem::exists(output_file_)) {
            std::filesystem::remove(output_file_);
        }
    }

    void dump_profilings(const std::string& name) {
        if (metric_collection_.empty() || output_file_.empty()) return;

        std::ofstream file(output_file_, std::ios::app);
        if (!file.is_open()) return;

        for (const auto& [key, values] : metric_collection_) {
            file << key;
            for (const auto& v : values) {
                file << "," << v;
            }
            file << "\n";
        }
    }

private:
    bool profiling_enabled() const {
        return tb_ != nullptr;  // Simplified - could check tb_->profiling_enable()
    }
    
    uint64_t get_sample_duration() const {
        return 1000;  // Default sample duration
    }

    TestBench* tb_;
    std::string output_file_;
    uint64_t cycle_;
    Metric metric_sample_;
    std::map<std::string, std::vector<int64_t>> metric_collection_;
    std::map<std::string, int64_t> status_custom_;
};



//=============================================================================
// GlobalProfiler macros using Tracer singleton
//=============================================================================
#undef TRACING_ENTER_REGION
#undef TRACING_EXIT_REGION
#undef TRACING_GENERATE_REPORT

//#define ENABLE_TRACING
#ifdef ENABLE_TRACING
    #define TRACING_ENTER_REGION(a, b, c) cg1gpu::getTracer().enterRegion((a), (b), (c));
    #define TRACING_EXIT_REGION()         cg1gpu::getTracer().exitRegion();
    #define TRACING_GENERATE_REPORT(a)    cg1gpu::getTracer().generateReport((a));
#else
    #define TRACING_ENTER_REGION(a, b, c)
    #define TRACING_EXIT_REGION()
    #define TRACING_GENERATE_REPORT(a)
#endif

//  Get the global profiler instance
Tracer& getTracer();

}   // namespace cg1gpu


#endif    // _PROFILER_
