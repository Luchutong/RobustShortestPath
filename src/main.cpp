#include "rsp/dijkstra_like.hpp"
#include "rsp/exhaustive_search.hpp"
#include "rsp/io.hpp"
#include "rsp/policy_iteration.hpp"
#include "rsp/value_iteration.hpp"

#include <chrono>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {

struct Args {
    std::string input;
    std::string algorithm = "vi";
    std::string output = "results";
    bool init_with_inf = true;
    int max_iter = 100000;
    double epsilon = 1e-9;
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
        } else if (key == "--algorithm") {
            args.algorithm = require_value(key);
        } else if (key == "--output") {
            args.output = require_value(key);
        } else if (key == "--max-iter") {
            args.max_iter = std::stoi(require_value(key));
        } else if (key == "--epsilon") {
            args.epsilon = std::stod(require_value(key));
        } else if (key == "--zero-init") {
            args.init_with_inf = false;
        } else if (key == "--help") {
            std::cout
                << "Usage: rsp_main --input data/toy_graph.txt "
                << "--algorithm vi|pi|dijkstra|exhaustive --output results\n";
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

std::string graph_id_from_path(const std::string& input) {
    return std::filesystem::path(input).stem().string();
}

}  // namespace

int main(int argc, char** argv) {
    try {
        const Args args = parse_args(argc, argv);
        rsp::RobustGraph graph = rsp::read_graph_txt(args.input);
        const std::string graph_id = graph_id_from_path(args.input);

        std::vector<double> value;
        std::vector<int> policy;
        std::vector<double> residual_history;
        int iterations = 0;
        bool converged = false;
        bool success = false;

        const auto start = std::chrono::steady_clock::now();

        if (args.algorithm == "vi") {
            auto result = rsp::value_iteration(
                graph, args.epsilon, args.max_iter, args.init_with_inf, true);
            value = result.value;
            policy = result.policy;
            residual_history = result.residual_history;
            iterations = result.iterations;
            converged = result.converged;
            success = true;
        } else if (args.algorithm == "pi") {
            auto result = rsp::policy_iteration(graph, args.max_iter, args.epsilon);
            value = result.value;
            policy = result.policy;
            residual_history = result.residual_history;
            iterations = result.outer_iterations;
            converged = result.converged;
            success = result.final_policy_proper;
        } else if (args.algorithm == "dijkstra") {
            auto result = rsp::dijkstra_like(graph);
            value = result.value;
            policy = result.policy;
            iterations = result.finalized_count;
            converged = result.success;
            success = result.success;
        } else if (args.algorithm == "exhaustive") {
            auto result = rsp::exhaustive_search(graph);
            value = result.value;
            policy = result.policy;
            iterations = result.checked_policies;
            converged = result.success;
            success = result.success;
        } else {
            throw std::invalid_argument("unknown algorithm: " + args.algorithm);
        }

        const auto stop = std::chrono::steady_clock::now();
        const double runtime_ms =
            std::chrono::duration<double, std::milli>(stop - start).count();

        std::filesystem::create_directories(args.output);
        rsp::append_values_csv(args.output + "/values.csv", graph_id, args.algorithm, value);
        rsp::append_policies_csv(args.output + "/policies.csv", graph_id, args.algorithm, graph, policy);
        rsp::append_residual_history_csv(
            args.output + "/residual_history.csv", graph_id, args.algorithm, residual_history);
        rsp::append_runtime_csv(
            args.output + "/runtime.csv", graph_id, graph, args.algorithm,
            runtime_ms, iterations, converged, value, success);

        std::cout << "algorithm=" << args.algorithm
                  << " graph=" << graph_id
                  << " success=" << (success ? 1 : 0)
                  << " runtime_ms=" << runtime_ms << '\n';
        return success ? 0 : 2;
    } catch (const std::exception& e) {
        std::cerr << "error: " << e.what() << '\n';
        return 1;
    }
}

