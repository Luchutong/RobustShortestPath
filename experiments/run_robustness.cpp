#include "rsp/baseline.hpp"
#include "rsp/io.hpp"
#include "rsp/proper_policy.hpp"
#include "rsp/runner.hpp"
#include "rsp/utils.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

struct Args {
    std::string input;
    std::string output = "results";
    int start = 0;
    int max_steps = 1000;
    int successor_set_size = -1;
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
        } else if (key == "--output") {
            args.output = require_value(key);
        } else if (key == "--start") {
            args.start = std::stoi(require_value(key));
        } else if (key == "--max-steps") {
            args.max_steps = std::stoi(require_value(key));
        } else if (key == "--s") {
            args.successor_set_size = std::stoi(require_value(key));
        } else if (key == "--help") {
            std::cout
                << "Usage: run_robustness --input data/toy_graph.txt "
                << "--output results --start 0 --max-steps 20 [--s 2]\n";
            std::exit(0);
        } else {
            throw std::invalid_argument("unknown argument: " + key);
        }
    }
    if (args.input.empty()) {
        throw std::invalid_argument("--input is required");
    }
    return args;
}

bool file_is_empty_or_missing(const std::string& path) {
    std::error_code ec;
    if (!std::filesystem::exists(path, ec)) {
        return true;
    }
    return std::filesystem::file_size(path, ec) == 0;
}

std::string graph_id_from_path(const std::string& input) {
    return std::filesystem::path(input).stem().string();
}

int max_successor_set_size(const rsp::RobustGraph& graph) {
    int max_size = 0;
    for (const auto& node : graph.nodes) {
        for (const auto& action : node.actions) {
            const int size = static_cast<int>(action.trans.size());
            if (size > max_size) {
                max_size = size;
            }
        }
    }
    return max_size;
}

bool valid_rollout_policy(const rsp::RobustGraph& graph, const rsp::AlgorithmRunResult& run) {
    if (!run.success || !run.converged || static_cast<int>(run.policy.size()) != graph.n) {
        return false;
    }
    return rsp::check_policy_proper(graph, run.policy).proper;
}

void append_rollout_row(
    std::ofstream& out,
    const std::string& graph_id,
    int successor_set_size,
    const std::string& policy_type,
    int start_node,
    const rsp::RolloutResult& rollout
) {
    out << graph_id << ','
        << successor_set_size << ','
        << policy_type << ','
        << start_node << ','
        << rsp::format_value(rollout.cost) << ','
        << (rollout.terminated ? 1 : 0) << ','
        << rollout.steps << '\n';
}

}  // namespace

int main(int argc, char** argv) {
    try {
        const Args args = parse_args(argc, argv);
        const rsp::RobustGraph graph = rsp::read_graph_txt(args.input);
        if (args.start < 0 || args.start >= graph.n) {
            throw std::invalid_argument("--start is out of range");
        }

        std::filesystem::create_directories(args.output);
        const std::string csv_path = args.output + "/robustness.csv";
        const bool header = file_is_empty_or_missing(csv_path);
        std::ofstream out(csv_path, std::ios::app);
        if (!out) {
            throw std::runtime_error("failed to open robustness csv");
        }
        if (header) {
            out << "graph_id,s,policy_type,start_node,worst_cost,terminated,steps\n";
        }

        const std::string graph_id = graph_id_from_path(args.input);
        const int successor_set_size = args.successor_set_size >= 0
            ? args.successor_set_size
            : max_successor_set_size(graph);
        const std::vector<std::string> algorithms = {
            "baseline_nominal",
            "baseline_bestcase",
            "baseline_worst_immediate",
            "vi",
        };

        for (const auto& algorithm : algorithms) {
            const auto run = rsp::run_algorithm(graph, algorithm);
            rsp::RolloutResult rollout;
            if (valid_rollout_policy(graph, run)) {
                rollout = rsp::adversarial_rollout(
                    graph, run.policy, run.value, args.start, args.max_steps);
            } else {
                rollout.cost = rsp::INF;
            }
            append_rollout_row(out, graph_id, successor_set_size, algorithm, args.start, rollout);
        }

        std::cout << "wrote " << csv_path << '\n';
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "error: " << e.what() << '\n';
        return 1;
    }
}
