#include "rsp/io.hpp"

#include "rsp/utils.hpp"

#include <filesystem>
#include <fstream>
#include <iomanip>
#include <cmath>
#include <stdexcept>

namespace rsp {
namespace {

bool file_is_empty_or_missing(const std::string& path) {
    std::error_code ec;
    if (!std::filesystem::exists(path, ec)) {
        return true;
    }
    return std::filesystem::file_size(path, ec) == 0;
}

void ensure_parent_dir(const std::string& path) {
    const auto parent = std::filesystem::path(path).parent_path();
    if (!parent.empty()) {
        std::filesystem::create_directories(parent);
    }
}

}  // namespace

RobustGraph read_graph_txt(const std::string& path) {
    std::ifstream in(path);
    if (!in) {
        throw std::runtime_error("failed to open input graph: " + path);
    }

    RobustGraph graph;
    in >> graph.n >> graph.terminal;
    if (!in) {
        throw std::runtime_error("failed to read graph header");
    }

    graph.nodes.resize(graph.n);
    for (int x = 0; x < graph.n; ++x) {
        graph.nodes[x].id = x;
        int action_count = 0;
        in >> action_count;
        if (!in) {
            throw std::runtime_error("failed to read action count");
        }
        if (action_count < 0) {
            throw std::runtime_error("negative action count");
        }

        graph.nodes[x].actions.resize(action_count);
        for (int a = 0; a < action_count; ++a) {
            Action action;
            int successor_count = 0;
            in >> action.action_id >> successor_count;
            if (!in) {
                throw std::runtime_error("failed to read action header");
            }
            if (successor_count < 0) {
                throw std::runtime_error("negative successor count");
            }
            for (int k = 0; k < successor_count; ++k) {
                Transition tr;
                in >> tr.to >> tr.cost;
                if (!in) {
                    throw std::runtime_error("failed to read transition");
                }
                if (!std::isfinite(tr.cost)) {
                    throw std::runtime_error("transition cost must be finite");
                }
                action.trans.push_back(tr);
            }
            graph.nodes[x].actions[a] = std::move(action);
        }
    }

    graph.validate();
    return graph;
}

void write_graph_json(const RobustGraph& graph, const std::string& path) {
    graph.validate();
    ensure_parent_dir(path);
    std::ofstream out(path);
    if (!out) {
        throw std::runtime_error("failed to open json output: " + path);
    }

    out << "{\n";
    out << "  \"n\": " << graph.n << ",\n";
    out << "  \"terminal\": " << graph.terminal << ",\n";
    out << "  \"nodes\": [\n";
    for (int x = 0; x < graph.n; ++x) {
        out << "    {\"id\": " << x << ", \"actions\": [";
        for (int a = 0; a < static_cast<int>(graph.nodes[x].actions.size()); ++a) {
            const auto& action = graph.nodes[x].actions[a];
            if (a > 0) out << ", ";
            out << "{\"action_id\": " << action.action_id << ", \"successors\": [";
            for (int k = 0; k < static_cast<int>(action.trans.size()); ++k) {
                const auto& tr = action.trans[k];
                if (k > 0) out << ", ";
                out << "{\"to\": " << tr.to << ", \"cost\": " << tr.cost << "}";
            }
            out << "]}";
        }
        out << "]}";
        if (x + 1 < graph.n) out << ",";
        out << "\n";
    }
    out << "  ]\n";
    out << "}\n";
}

void append_values_csv(
    const std::string& path,
    const std::string& graph_id,
    const std::string& algorithm,
    const std::vector<double>& value
) {
    ensure_parent_dir(path);
    const bool header = file_is_empty_or_missing(path);
    std::ofstream out(path, std::ios::app);
    if (!out) throw std::runtime_error("failed to open values csv");
    if (header) out << "graph_id,algorithm,node,value\n";
    for (int x = 0; x < static_cast<int>(value.size()); ++x) {
        out << graph_id << ',' << algorithm << ',' << x << ',' << format_value(value[x]) << '\n';
    }
}

void append_policies_csv(
    const std::string& path,
    const std::string& graph_id,
    const std::string& algorithm,
    const RobustGraph& graph,
    const std::vector<int>& policy
) {
    ensure_parent_dir(path);
    const bool header = file_is_empty_or_missing(path);
    std::ofstream out(path, std::ios::app);
    if (!out) throw std::runtime_error("failed to open policies csv");
    if (header) out << "graph_id,algorithm,node,action_index,action_id\n";
    for (int x = 0; x < static_cast<int>(policy.size()); ++x) {
        const int action_index = policy[x];
        int action_id = -1;
        if (!graph.is_terminal(x) && action_index >= 0 &&
            action_index < static_cast<int>(graph.nodes[x].actions.size())) {
            action_id = graph.nodes[x].actions[action_index].action_id;
        }
        out << graph_id << ',' << algorithm << ',' << x << ','
            << action_index << ',' << action_id << '\n';
    }
}

void append_residual_history_csv(
    const std::string& path,
    const std::string& graph_id,
    const std::string& algorithm,
    const std::vector<double>& residual_history
) {
    ensure_parent_dir(path);
    const bool header = file_is_empty_or_missing(path);
    std::ofstream out(path, std::ios::app);
    if (!out) throw std::runtime_error("failed to open residual csv");
    if (header) out << "graph_id,algorithm,iteration,residual\n";
    for (int i = 0; i < static_cast<int>(residual_history.size()); ++i) {
        out << graph_id << ',' << algorithm << ',' << i + 1 << ','
            << format_value(residual_history[i]) << '\n';
    }
}

void append_runtime_csv(
    const std::string& path,
    const std::string& graph_id,
    const RobustGraph& graph,
    const std::string& algorithm,
    double runtime_ms,
    int iterations,
    bool converged,
    const std::vector<double>& value,
    bool success
) {
    ensure_parent_dir(path);
    const bool header = file_is_empty_or_missing(path);
    std::ofstream out(path, std::ios::app);
    if (!out) throw std::runtime_error("failed to open runtime csv");
    if (header) {
        out << "graph_id,n,total_actions,total_transitions,algorithm,runtime_ms,"
               "iterations,converged,avg_value,success\n";
    }

    double sum = 0.0;
    int count = 0;
    for (int x = 0; x < graph.n; ++x) {
        if (!graph.is_terminal(x) && !is_inf(value[x])) {
            sum += value[x];
            ++count;
        }
    }
    const double avg = count == 0 ? INF : sum / count;

    out << graph_id << ',' << graph.n << ',' << graph.total_actions() << ','
        << graph.total_transitions() << ',' << algorithm << ','
        << std::fixed << std::setprecision(6) << runtime_ms << ','
        << iterations << ',' << (converged ? 1 : 0) << ','
        << format_value(avg) << ',' << (success ? 1 : 0) << '\n';
}

}  // namespace rsp
