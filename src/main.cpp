#include "rsp/io.hpp"
#include "rsp/runner.hpp"

#include <chrono>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {

int parse_int_strict(const std::string& text, const std::string& name) {
    std::size_t pos = 0;
    int value = std::stoi(text, &pos);
    if (pos != text.size()) {
        throw std::invalid_argument("invalid integer for " + name + ": " + text);
    }
    return value;
}

double parse_double_strict(const std::string& text, const std::string& name) {
    std::size_t pos = 0;
    double value = std::stod(text, &pos);
    if (pos != text.size()) {
        throw std::invalid_argument("invalid double for " + name + ": " + text);
    }
    return value;
}

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
            args.max_iter = parse_int_strict(require_value(key), key);
        } else if (key == "--epsilon") {
            args.epsilon = parse_double_strict(require_value(key), key);
        } else if (key == "--zero-init") {
            args.init_with_inf = false;
        } else if (key == "--help") {
            std::cout
                << "Usage: rsp_main --input data/toy_graph.txt "
                << "--algorithm vi|pi|dijkstra|exhaustive|baseline_nominal|baseline_bestcase|baseline_worst_immediate "
                << "--output results\n";
            std::exit(0);
        } else {
            throw std::invalid_argument("unknown argument: " + key);
        }
    }
    if (args.input.empty()) {
        throw std::invalid_argument("--input is required");
    }
    if (args.max_iter < 0) {
        throw std::invalid_argument("--max-iter must be non-negative");
    }
    if (!(args.epsilon > 0.0) || !std::isfinite(args.epsilon)) {
        throw std::invalid_argument("--epsilon must be positive and finite");
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

        const auto start = std::chrono::steady_clock::now();
        rsp::AlgorithmOptions options;
        options.epsilon = args.epsilon;
        options.max_iter = args.max_iter;
        options.init_with_inf = args.init_with_inf;
        options.save_history = true;
        rsp::AlgorithmRunResult result = rsp::run_algorithm(graph, args.algorithm, options);

        const auto stop = std::chrono::steady_clock::now();
        const double runtime_ms =
            std::chrono::duration<double, std::milli>(stop - start).count();

        std::filesystem::create_directories(args.output);
        rsp::append_values_csv(args.output + "/values.csv", graph_id, result.name, result.value);
        rsp::append_policies_csv(args.output + "/policies.csv", graph_id, result.name, graph, result.policy);
        rsp::append_residual_history_csv(
            args.output + "/residual_history.csv", graph_id, result.name, result.residual_history);
        rsp::append_runtime_csv(
            args.output + "/runtime.csv", graph_id, graph, result.name,
            runtime_ms, result.iterations, result.converged, result.value, result.success);

        std::cout << "algorithm=" << result.name
                  << " graph=" << graph_id
                  << " success=" << (result.success ? 1 : 0)
                  << " runtime_ms=" << runtime_ms << '\n';
        return result.success ? 0 : 2;
    } catch (const std::exception& e) {
        std::cerr << "error: " << e.what() << '\n';
        return 1;
    }
}
