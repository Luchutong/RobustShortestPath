#include "rsp/baseline.hpp"
#include "rsp/io.hpp"
#include "rsp/proper_policy.hpp"
#include "rsp/runner.hpp"
#include "rsp/utils.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <set>
#include <map>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

int parse_int_strict(const std::string& text, const std::string& name) {
    std::size_t pos = 0;
    int value = std::stoi(text, &pos);
    if (pos != text.size()) {
        throw std::invalid_argument("invalid integer for " + name + ": " + text);
    }
    return value;
}

struct Args {
    std::string input;
    std::string input_dir;
    std::string output = "results";
    int start = 0;
    int max_steps = 1000;
    int successor_set_size = 0;
    bool has_successor_set_size = false;
};

struct RobustnessRow {
    std::string graph_id;
    int successor_set_size = 0;
    std::string policy_type;
    int start_node = 0;
    bool policy_valid = false;
    std::string status;
    double worst_cost = rsp::INF;
    bool terminated = false;
    int steps = 0;
};

struct Summary {
    int cases = 0;
    int valid_count = 0;
    int terminated_count = 0;
    double terminated_cost_sum = 0.0;
    int terminated_cost_count = 0;
    double terminated_steps_sum = 0.0;
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

        if (key == "--input") {
            args.input = require_value(key);
        } else if (key == "--input-dir") {
            args.input_dir = require_value(key);
        } else if (key == "--output") {
            args.output = require_value(key);
        } else if (key == "--start") {
            args.start = parse_int_strict(require_value(key), key);
        } else if (key == "--max-steps") {
            args.max_steps = parse_int_strict(require_value(key), key);
        } else if (key == "--s") {
            args.successor_set_size = parse_int_strict(require_value(key), key);
            args.has_successor_set_size = true;
        } else if (key == "--help") {
            std::cout
                << "Usage: run_robustness (--input data/toy_graph.txt | "
                << "--input-dir experiment_data/.../graphs) "
                << "--output results --start 0 --max-steps 20 [--s 2]\n";
            std::exit(0);
        } else {
            throw std::invalid_argument("unknown argument: " + key);
        }
    }
    if (args.input.empty() == args.input_dir.empty()) {
        throw std::invalid_argument("exactly one of --input or --input-dir is required");
    }
    if (args.max_steps < 0) {
        throw std::invalid_argument("--max-steps must be non-negative");
    }
    if (args.has_successor_set_size && args.successor_set_size <= 0) {
        throw std::invalid_argument("--s must be positive when provided");
    }
    return args;
}

std::vector<std::filesystem::path> list_graph_files(const Args& args) {
    if (!args.input.empty()) {
        return {std::filesystem::path(args.input)};
    }

    std::vector<std::filesystem::path> files;
    std::error_code ec;
    if (!std::filesystem::is_directory(args.input_dir, ec)) {
        throw std::invalid_argument("input directory does not exist: " + args.input_dir);
    }
    for (const auto& entry : std::filesystem::recursive_directory_iterator(args.input_dir)) {
        if (entry.is_regular_file() && entry.path().extension() == ".txt") {
            files.push_back(entry.path());
        }
    }
    std::sort(files.begin(), files.end());
    if (files.empty()) {
        throw std::invalid_argument("input directory contains no .txt graphs");
    }
    return files;
}

std::string graph_id_from_path(
    const std::filesystem::path& path,
    const std::string& input_dir
) {
    if (input_dir.empty()) {
        return path.stem().string();
    }
    std::string value = std::filesystem::relative(path, input_dir).string();
    for (char& ch : value) {
        if (ch == '/' || ch == '\\') {
            ch = '_';
        }
    }
    const auto txt_pos = value.rfind(".txt");
    if (txt_pos != std::string::npos && txt_pos + 4 == value.size()) {
        value.erase(txt_pos);
    }
    return value;
}

int max_successor_set_size(const rsp::RobustGraph& graph) {
    int max_size = 0;
    for (const auto& node : graph.nodes) {
        for (const auto& action : node.actions) {
            std::set<int> successors;
            for (const auto& tr : action.trans) {
                successors.insert(tr.to);
            }
            const int size = static_cast<int>(successors.size());
            if (size > max_size) {
                max_size = size;
            }
        }
    }
    return max_size;
}

std::string invalid_reason_for_run(
    const rsp::RobustGraph& graph,
    const rsp::AlgorithmRunResult& run
) {
    if (!run.converged) {
        return "algorithm_not_converged";
    }
    if (static_cast<int>(run.policy.size()) != graph.n) {
        return "policy_size_mismatch";
    }
    if (!rsp::check_policy_proper(graph, run.policy).proper) {
        return "policy_not_proper";
    }
    if (static_cast<int>(run.value.size()) != graph.n) {
        return "value_size_mismatch";
    }
    for (int x = 0; x < graph.n; ++x) {
        if (!graph.is_terminal(x) && rsp::is_inf(run.value[x])) {
            return "value_not_finite";
        }
    }
    if (!run.success) {
        return "algorithm_not_successful";
    }
    return "";
}

double safe_average(double sum, int count) {
    return count == 0 ? rsp::INF : sum / count;
}

void write_raw_header(std::ofstream& out) {
    out << "graph_id,s,policy_type,start_node,policy_valid,status,"
           "worst_cost,terminated,steps\n";
}

void write_raw_row(std::ofstream& out, const RobustnessRow& row) {
    out << row.graph_id << ','
        << row.successor_set_size << ','
        << row.policy_type << ','
        << row.start_node << ','
        << (row.policy_valid ? 1 : 0) << ','
        << row.status << ','
        << rsp::format_value(row.worst_cost) << ','
        << (row.terminated ? 1 : 0) << ','
        << row.steps << '\n';
}

void update_summary(Summary& summary, const RobustnessRow& row) {
    ++summary.cases;
    if (!row.policy_valid) {
        return;
    }
    ++summary.valid_count;
    if (!row.terminated) {
        return;
    }
    ++summary.terminated_count;
    summary.terminated_steps_sum += row.steps;
    if (!rsp::is_inf(row.worst_cost)) {
        summary.terminated_cost_sum += row.worst_cost;
        ++summary.terminated_cost_count;
    }
}

void write_summary(
    const std::string& path,
    const std::map<std::pair<int, std::string>, Summary>& summaries
) {
    std::ofstream out(path);
    if (!out) {
        throw std::runtime_error("failed to open robustness summary csv");
    }
    out << "s,policy_type,cases,valid_count,valid_rate,terminated_count,terminated_rate,"
           "avg_worst_cost,avg_steps\n";
    for (const auto& item : summaries) {
        const int successor_set_size = item.first.first;
        const std::string& policy_type = item.first.second;
        const Summary& summary = item.second;
        const double valid_rate = summary.cases == 0
            ? 0.0
            : static_cast<double>(summary.valid_count) / summary.cases;
        const double terminated_rate = summary.cases == 0
            ? 0.0
            : static_cast<double>(summary.terminated_count) / summary.cases;
        const double avg_worst_cost =
            safe_average(summary.terminated_cost_sum, summary.terminated_cost_count);
        const double avg_steps =
            safe_average(summary.terminated_steps_sum, summary.terminated_count);

        out << successor_set_size << ','
            << policy_type << ','
            << summary.cases << ','
            << summary.valid_count << ','
            << rsp::format_value(valid_rate) << ','
            << summary.terminated_count << ','
            << rsp::format_value(terminated_rate) << ','
            << rsp::format_value(avg_worst_cost) << ','
            << rsp::format_value(avg_steps) << '\n';
    }
}

RobustnessRow run_one_algorithm(
    const rsp::RobustGraph& graph,
    const std::string& graph_id,
    int successor_set_size,
    const std::string& algorithm,
    int start,
    int max_steps
) {
    RobustnessRow row;
    row.graph_id = graph_id;
    row.successor_set_size = successor_set_size;
    row.policy_type = algorithm;
    row.start_node = start;

    try {
        const auto run = rsp::run_algorithm(graph, algorithm);
        row.status = invalid_reason_for_run(graph, run);
        row.policy_valid = row.status.empty();
        if (!row.policy_valid) {
            return row;
        }

        const rsp::RolloutResult rollout = rsp::adversarial_rollout(
            graph, run.policy, run.value, start, max_steps);
        row.worst_cost = rollout.cost;
        row.terminated = rollout.terminated;
        row.steps = rollout.steps;
        // A policy that passed invalid_reason_for_run is already valid here; a
        // proper policy always terminates, so "rollout_timeout" only means
        // --max-steps was too small, not that the policy became invalid.
        row.status = row.terminated ? "ok" : "rollout_timeout";
    } catch (const std::exception& e) {
        row.status = "algorithm_exception";
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
        const auto files = list_graph_files(args);
        if (!args.input_dir.empty() && args.has_successor_set_size) {
            throw std::invalid_argument("--s cannot be used together with --input-dir");
        }
        std::filesystem::create_directories(args.output);
        const std::string raw_path = args.output + "/robustness.csv";
        const std::string summary_path = args.output + "/robustness_summary.csv";
        std::ofstream raw(raw_path);
        if (!raw) {
            throw std::runtime_error("failed to open robustness csv");
        }
        write_raw_header(raw);

        const std::vector<std::string> algorithms = {
            "baseline_nominal",
            "baseline_bestcase",
            "baseline_worst_immediate",
            "vi",
        };
        std::map<std::pair<int, std::string>, Summary> summaries;

        for (const auto& file : files) {
            const std::string graph_id = graph_id_from_path(file, args.input_dir);
            try {
                const rsp::RobustGraph graph = rsp::read_graph_txt(file.string());
                if (args.start < 0 || args.start >= graph.n) {
                    throw std::invalid_argument(
                        "--start is out of range for graph " + file.string());
                }

                const int successor_set_size = args.has_successor_set_size
                    ? args.successor_set_size
                    : max_successor_set_size(graph);

                for (const auto& algorithm : algorithms) {
                    const RobustnessRow row = run_one_algorithm(
                        graph, graph_id, successor_set_size, algorithm, args.start, args.max_steps);
                    write_raw_row(raw, row);
                    update_summary(summaries[{row.successor_set_size, row.policy_type}], row);
                }
            } catch (const std::exception& e) {
                // One unreadable/invalid graph should not abort the whole batch.
                std::cerr << "skipping graph: " << file.string()
                          << " error=" << e.what() << '\n';
                continue;
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
