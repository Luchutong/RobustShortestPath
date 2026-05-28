#include "rsp/io.hpp"
#include "rsp/runner.hpp"
#include "rsp/utils.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <regex>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

struct Args {
    std::string input_dir = "data/random_graphs";
    std::string output = "results";
    int max_iter = 100000;
    double epsilon = 1e-9;
};

struct RunRow {
    std::string graph_id;
    int n = 0;
    int total_actions = 0;
    int total_transitions = 0;
    std::string algorithm;
    double runtime_ms = 0.0;
    int iterations = 0;
    bool converged = false;
    bool success = false;
    double avg_value = rsp::INF;
};

struct SummaryKey {
    int n = 0;
    int requested_s = 0;
    int total_actions = 0;
    std::string algorithm;

    bool operator<(const SummaryKey& other) const {
        if (n != other.n) return n < other.n;
        if (requested_s != other.requested_s) return requested_s < other.requested_s;
        if (total_actions != other.total_actions) return total_actions < other.total_actions;
        return algorithm < other.algorithm;
    }
};

struct Summary {
    int cases = 0;
    int success_count = 0;
    double runtime_sum = 0.0;
    double success_iterations_sum = 0.0;
    double success_value_sum = 0.0;
    int success_value_count = 0;
};

Args parse_args(int argc, char** argv) {
    Args args;
    for (int i = 1; i < argc; ++i) {
        const std::string key = argv[i];
        auto require_value = [&](const std::string& name) -> std::string {
            if (i + 1 >= argc) {
                throw std::invalid_argument("missing value for " + name);
            }
            return argv[++i];
        };

        if (key == "--input-dir") {
            args.input_dir = require_value(key);
        } else if (key == "--output") {
            args.output = require_value(key);
        } else if (key == "--max-iter") {
            args.max_iter = std::stoi(require_value(key));
        } else if (key == "--epsilon") {
            args.epsilon = std::stod(require_value(key));
        } else if (key == "--help") {
            std::cout
                << "Usage: run_runtime --input-dir data/random_graphs "
                << "--output results --epsilon 1e-9 --max-iter 100000\n";
            std::exit(0);
        } else {
            throw std::invalid_argument("unknown argument: " + key);
        }
    }
    if (args.max_iter < 0) {
        throw std::invalid_argument("--max-iter must be non-negative");
    }
    if (!(args.epsilon > 0.0) || !std::isfinite(args.epsilon)) {
        throw std::invalid_argument("--epsilon must be positive and finite");
    }
    return args;
}

std::vector<std::filesystem::path> list_graph_files(const std::string& input_dir) {
    std::vector<std::filesystem::path> files;
    std::error_code ec;
    if (!std::filesystem::is_directory(input_dir, ec)) {
        throw std::invalid_argument("input directory does not exist: " + input_dir);
    }
    for (const auto& entry : std::filesystem::directory_iterator(input_dir)) {
        if (entry.is_regular_file() && entry.path().extension() == ".txt") {
            files.push_back(entry.path());
        }
    }
    std::sort(files.begin(), files.end());
    return files;
}

std::string graph_id_from_path(const std::filesystem::path& path) {
    return path.stem().string();
}

void parse_graph_params(const std::string& graph_id, int& requested_s, int& actions) {
    requested_s = 0;
    actions = 0;
    std::regex s_re("_s(\\d+)_");
    std::regex a_re("_a(\\d+)_");
    std::smatch match;
    if (std::regex_search(graph_id, match, s_re) && match.size() > 1) {
        requested_s = std::stoi(match[1].str());
    }
    if (std::regex_search(graph_id, match, a_re) && match.size() > 1) {
        actions = std::stoi(match[1].str());
    }
}

double average_value(const rsp::RobustGraph& graph, const std::vector<double>& value) {
    if (static_cast<int>(value.size()) != graph.n) {
        return rsp::INF;
    }
    double sum = 0.0;
    int count = 0;
    for (int x = 0; x < graph.n; ++x) {
        if (!graph.is_terminal(x) && !rsp::is_inf(value[x])) {
            sum += value[x];
            ++count;
        }
    }
    return count == 0 ? rsp::INF : sum / count;
}

void write_raw_header(std::ofstream& out) {
    out << "graph_id,n,total_actions,total_transitions,algorithm,runtime_ms,"
           "iterations,converged,success,avg_value\n";
}

void write_raw_row(std::ofstream& out, const RunRow& row) {
    out << row.graph_id << ','
        << row.n << ','
        << row.total_actions << ','
        << row.total_transitions << ','
        << row.algorithm << ','
        << std::fixed << std::setprecision(6) << row.runtime_ms << ','
        << row.iterations << ','
        << (row.converged ? 1 : 0) << ','
        << (row.success ? 1 : 0) << ','
        << rsp::format_value(row.avg_value) << '\n';
}

void update_summary(Summary& summary, const RunRow& row) {
    ++summary.cases;
    summary.runtime_sum += row.runtime_ms;
    if (!row.success) {
        return;
    }
    ++summary.success_count;
    summary.success_iterations_sum += row.iterations;
    if (!rsp::is_inf(row.avg_value)) {
        summary.success_value_sum += row.avg_value;
        ++summary.success_value_count;
    }
}

double safe_average(double sum, int count) {
    return count == 0 ? rsp::INF : sum / count;
}

void write_summary(
    const std::string& path,
    const std::map<SummaryKey, Summary>& summaries
) {
    std::ofstream out(path);
    if (!out) {
        throw std::runtime_error("failed to open runtime summary csv");
    }
    out << "n,requested_s,actions,algorithm,cases,success_count,success_rate,"
           "avg_runtime_ms,avg_iterations,avg_value\n";
    for (const auto& item : summaries) {
        const SummaryKey& key = item.first;
        const Summary& summary = item.second;
        const double success_rate = summary.cases == 0
            ? 0.0
            : static_cast<double>(summary.success_count) / summary.cases;
        const double avg_runtime = safe_average(summary.runtime_sum, summary.cases);
        const double avg_iterations =
            safe_average(summary.success_iterations_sum, summary.success_count);
        const double avg_value =
            safe_average(summary.success_value_sum, summary.success_value_count);

        out << key.n << ','
            << key.requested_s << ','
            << key.total_actions << ','
            << key.algorithm << ','
            << summary.cases << ','
            << summary.success_count << ','
            << std::fixed << std::setprecision(6) << success_rate << ','
            << rsp::format_value(avg_runtime) << ','
            << rsp::format_value(avg_iterations) << ','
            << rsp::format_value(avg_value) << '\n';
    }
}

RunRow run_one_algorithm(
    const rsp::RobustGraph& graph,
    const std::string& graph_id,
    const std::string& algorithm,
    const rsp::AlgorithmOptions& options
) {
    RunRow row;
    row.graph_id = graph_id;
    row.n = graph.n;
    row.total_actions = graph.total_actions();
    row.total_transitions = graph.total_transitions();
    row.algorithm = algorithm;

    const auto start = std::chrono::steady_clock::now();
    try {
        const rsp::AlgorithmRunResult result = rsp::run_algorithm(graph, algorithm, options);
        const auto stop = std::chrono::steady_clock::now();
        row.runtime_ms = std::chrono::duration<double, std::milli>(stop - start).count();
        row.iterations = result.iterations;
        row.converged = result.converged;
        row.success = result.success;
        row.avg_value = average_value(graph, result.value);
    } catch (const std::exception& e) {
        const auto stop = std::chrono::steady_clock::now();
        row.runtime_ms = std::chrono::duration<double, std::milli>(stop - start).count();
        row.iterations = 0;
        row.converged = false;
        row.success = false;
        row.avg_value = rsp::INF;
        std::cerr << "algorithm failed: graph=" << graph_id
                  << " algorithm=" << algorithm
                  << " error=" << e.what() << '\n';
    }

    return row;
}

}  // namespace

int main(int argc, char** argv) {
    try {
        const Args args = parse_args(argc, argv);
        const auto files = list_graph_files(args.input_dir);
        if (files.empty()) {
            throw std::invalid_argument("input directory contains no .txt graphs");
        }

        std::filesystem::create_directories(args.output);
        const std::string raw_path = args.output + "/runtime_experiment.csv";
        const std::string summary_path = args.output + "/runtime_summary.csv";

        std::ofstream raw(raw_path);
        if (!raw) {
            throw std::runtime_error("failed to open runtime experiment csv");
        }
        write_raw_header(raw);

        rsp::AlgorithmOptions options;
        options.epsilon = args.epsilon;
        options.max_iter = args.max_iter;
        options.init_with_inf = true;
        options.save_history = false;

        const std::vector<std::string> algorithms = {"vi", "pi", "dijkstra"};
        std::map<SummaryKey, Summary> summaries;

        for (const auto& file : files) {
            rsp::RobustGraph graph = rsp::read_graph_txt(file.string());
            const std::string graph_id = graph_id_from_path(file);
            int requested_s = 0;
            int actions_parsed = 0;
            parse_graph_params(graph_id, requested_s, actions_parsed);
            for (const auto& algorithm : algorithms) {
                const RunRow row = run_one_algorithm(graph, graph_id, algorithm, options);
                write_raw_row(raw, row);
                SummaryKey key;
                key.n = row.n;
                key.requested_s = requested_s;
                key.total_actions = actions_parsed;
                key.algorithm = row.algorithm;
                update_summary(summaries[key], row);
            }
        }

        write_summary(summary_path, summaries);
        std::cout << "wrote " << raw_path << " and " << summary_path << '\n';
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "error: " << e.what() << '\n';
        return 1;
    }
}
