#include "rsp/exhaustive_search.hpp"
#include "rsp/io.hpp"
#include "rsp/policy_iteration.hpp"
#include "rsp/proper_policy.hpp"
#include "rsp/runner.hpp"
#include "rsp/value_iteration.hpp"

#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

#define CHECK(cond)                                                            \
    do {                                                                       \
        if (!(cond)) {                                                         \
            std::cerr << "CHECK failed: " #cond                                \
                      << " at " << __FILE__ << ":" << __LINE__ << std::endl;   \
            std::exit(1);                                                      \
        }                                                                      \
    } while (0)

void expect_close(double actual, double expected) {
    CHECK(std::abs(actual - expected) < 1e-8);
}

void expect_vectors_close(
    const std::vector<double>& actual,
    const std::vector<double>& expected
) {
    CHECK(actual.size() == expected.size());
    for (int i = 0; i < static_cast<int>(actual.size()); ++i) {
        expect_close(actual[i], expected[i]);
    }
}

rsp::RobustGraph make_small_layered_graph() {
    rsp::RobustGraph graph;
    graph.n = 4;
    graph.terminal = 3;
    graph.nodes.resize(graph.n);
    for (int i = 0; i < graph.n; ++i) {
        graph.nodes[i].id = i;
    }

    graph.nodes[0].actions.push_back({0, "", {{1, 1.0}, {2, 5.0}}});
    graph.nodes[0].actions.push_back({1, "", {{2, 2.0}}});
    graph.nodes[1].actions.push_back({0, "", {{3, 3.0}}});
    graph.nodes[1].actions.push_back({1, "", {{3, 4.0}}});
    graph.nodes[2].actions.push_back({0, "", {{3, 1.0}}});
    graph.nodes[2].actions.push_back({1, "", {{3, 2.0}}});
    graph.validate();
    return graph;
}

rsp::RobustGraph make_no_proper_policy_graph() {
    rsp::RobustGraph graph;
    graph.n = 3;
    graph.terminal = 2;
    graph.nodes.resize(graph.n);
    for (int i = 0; i < graph.n; ++i) {
        graph.nodes[i].id = i;
    }
    graph.nodes[0].actions.push_back({0, "", {{1, 1.0}}});
    graph.nodes[1].actions.push_back({0, "", {{0, 1.0}}});
    graph.validate();
    return graph;
}

std::string write_temp_graph_file(const std::string& body, const std::string& suffix) {
    const std::string path = "/tmp/rsp_" + suffix + ".txt";
    std::ofstream out(path);
    if (!out) {
        throw std::runtime_error("failed to open temp graph file");
    }
    out << body;
    return path;
}

std::string read_text_file(const std::string& path) {
    std::ifstream in(path);
    CHECK(static_cast<bool>(in));
    return std::string(
        std::istreambuf_iterator<char>(in),
        std::istreambuf_iterator<char>());
}

std::string shell_quote(const std::string& s) {
    std::string out = "'";
    for (char c : s) {
        if (c == '\'') {
            out += "'\\''";
        } else {
            out += c;
        }
    }
    out += "'";
    return out;
}

void test_toy_vi_matches_exhaustive() {
    const rsp::RobustGraph graph = rsp::read_graph_txt("data/toy_graph.txt");
    const auto vi = rsp::value_iteration(graph, 1e-9, 1000, true, false);
    const auto exhaustive = rsp::exhaustive_search(graph);

    CHECK(vi.converged);
    CHECK(exhaustive.success);
    expect_vectors_close(vi.value, exhaustive.optimal_value_by_state);
    CHECK(vi.policy[0] == exhaustive.best_policy_for_start[0]);
}

void test_small_layered_vi_matches_exhaustive() {
    const rsp::RobustGraph graph = make_small_layered_graph();
    const auto vi = rsp::value_iteration(graph, 1e-9, 1000, true, false);
    const auto exhaustive = rsp::exhaustive_search(graph);

    CHECK(vi.converged);
    CHECK(exhaustive.success);
    expect_vectors_close(vi.value, exhaustive.optimal_value_by_state);
    CHECK(vi.policy[0] == 1);
    expect_close(vi.value[0], 3.0);
    expect_close(vi.value[1], 3.0);
    expect_close(vi.value[2], 1.0);
    expect_close(vi.value[3], 0.0);
}

void test_runner_exhaustive_uses_start_optimal_policy_and_statewise_values() {
    const rsp::RobustGraph graph = make_small_layered_graph();
    const auto exhaustive = rsp::exhaustive_search(graph);
    const auto run = rsp::run_algorithm(graph, "exhaustive");

    CHECK(exhaustive.success);
    CHECK(run.success);
    expect_vectors_close(run.value, exhaustive.optimal_value_by_state);
    CHECK(run.policy == exhaustive.best_policy_for_start);
}

void test_value_iteration_reports_not_converged() {
    const rsp::RobustGraph graph = rsp::read_graph_txt("data/toy_graph.txt");
    const auto vi = rsp::value_iteration(graph, 1e-9, 1, true, false);
    CHECK(!vi.converged);
    CHECK(vi.iterations == 1);
}

void test_runner_vi_success_requires_convergence() {
    const rsp::RobustGraph graph = rsp::read_graph_txt("data/toy_graph.txt");
    rsp::AlgorithmOptions options;
    options.max_iter = 1;
    options.epsilon = 1e-9;
    options.init_with_inf = true;
    options.save_history = false;

    const auto run = rsp::run_algorithm(graph, "vi", options);
    CHECK(!run.converged);
    CHECK(!run.success);
}

void test_runner_vi_rejects_no_proper_policy_graph() {
    const rsp::RobustGraph graph = make_no_proper_policy_graph();
    rsp::AlgorithmOptions options;
    options.max_iter = 20;
    options.epsilon = 1e-9;
    options.init_with_inf = true;
    options.save_history = false;

    const auto run = rsp::run_algorithm(graph, "vi", options);
    CHECK(run.converged);
    CHECK(!run.success);
}

void test_runner_pi_requires_convergence() {
    const rsp::RobustGraph graph = rsp::read_graph_txt("data/toy_graph.txt");
    rsp::AlgorithmOptions options;
    options.max_iter = 0;
    options.epsilon = 1e-9;

    const auto run = rsp::run_algorithm(graph, "pi", options);
    CHECK(!run.converged);
    CHECK(!run.success);
}

void test_policy_iteration_accepts_max_iter_only() {
    const rsp::RobustGraph graph = rsp::read_graph_txt("data/toy_graph.txt");
    const auto result = rsp::policy_iteration(graph, 100);
    CHECK(result.converged);
    CHECK(result.final_policy_proper);
}

void test_terminal_actions_are_rejected() {
    rsp::RobustGraph graph;
    graph.n = 2;
    graph.terminal = 1;
    graph.nodes.resize(graph.n);
    graph.nodes[0].id = 0;
    graph.nodes[1].id = 1;
    graph.nodes[0].actions.push_back({0, "", {{1, 1.0}}});
    graph.nodes[1].actions.push_back({0, "", {{1, 0.0}}});

    bool threw = false;
    try {
        graph.validate();
    } catch (const std::invalid_argument&) {
        threw = true;
    }
    CHECK(threw);
}

void test_negative_action_count_rejected() {
    const std::string path = write_temp_graph_file("2 1\n-1\n0\n", "negative_action_count");
    bool threw = false;
    try {
        (void)rsp::read_graph_txt(path);
    } catch (const std::runtime_error&) {
        threw = true;
    }
    CHECK(threw);
    std::remove(path.c_str());
}

void test_negative_node_count_rejected_before_resize() {
    const std::string path = write_temp_graph_file("-1 0\n", "negative_node_count");
    bool threw = false;
    try {
        (void)rsp::read_graph_txt(path);
    } catch (const std::runtime_error&) {
        threw = true;
    }
    CHECK(threw);
    std::remove(path.c_str());
}

void test_terminal_index_rejected_before_resize() {
    const std::string path = write_temp_graph_file("2 2\n0\n0\n", "bad_terminal_index");
    bool threw = false;
    try {
        (void)rsp::read_graph_txt(path);
    } catch (const std::runtime_error&) {
        threw = true;
    }
    CHECK(threw);
    std::remove(path.c_str());
}

void test_negative_successor_count_rejected() {
    const std::string path = write_temp_graph_file("2 1\n1\n0 -1\n0\n", "negative_successor_count");
    bool threw = false;
    try {
        (void)rsp::read_graph_txt(path);
    } catch (const std::runtime_error&) {
        threw = true;
    }
    CHECK(threw);
    std::remove(path.c_str());
}

void test_terminal_policy_must_be_minus_one() {
    const rsp::RobustGraph graph = rsp::read_graph_txt("data/toy_graph.txt");
    std::vector<int> policy = {1, 0, 0, 0, 0, 0};
    const auto check = rsp::check_policy_proper(graph, policy);
    CHECK(!check.proper);
}

void test_generator_uses_distinct_filenames() {
    const std::string out_dir = "/tmp/rsp_generator_unique_names";
    const std::string cleanup = "rm -rf " + out_dir;
    CHECK(std::system(cleanup.c_str()) == 0);

    const std::string cmd1 =
        "python3 experiments/generate_medium_graphs.py --output " + out_dir +
        " --sizes 20 --cases 2 --actions 3 --successors 1 --seed 42";
    const std::string cmd2 =
        "python3 experiments/generate_medium_graphs.py --output " + out_dir +
        " --sizes 20 --cases 2 --actions 3 --successors 5 --seed 42";

    CHECK(std::system(cmd1.c_str()) == 0);
    CHECK(std::system(cmd2.c_str()) == 0);

    std::vector<std::string> names;
    for (const auto& entry : std::filesystem::directory_iterator(out_dir)) {
        if (entry.is_regular_file() && entry.path().extension() == ".txt") {
            names.push_back(entry.path().filename().string());
        }
    }
    CHECK(names.size() == 4);
    bool saw_s1 = false;
    bool saw_s5 = false;
    for (const auto& name : names) {
        if (name.find("_s1_") != std::string::npos) {
            saw_s1 = true;
        }
        if (name.find("_s5_") != std::string::npos) {
            saw_s5 = true;
        }
    }
    CHECK(saw_s1);
    CHECK(saw_s5);
    CHECK(std::system(cleanup.c_str()) == 0);
}

void test_generator_avoids_duplicate_successors() {
    const std::string out_dir = "/tmp/rsp_generator_unique_successors";
    const std::string cleanup = "rm -rf " + out_dir;
    CHECK(std::system(cleanup.c_str()) == 0);

    const std::string cmd =
        "python3 experiments/generate_medium_graphs.py --output " + out_dir +
        " --sizes 20 --cases 1 --actions 3 --successors 5 --seed 42";
    CHECK(std::system(cmd.c_str()) == 0);

    std::string path;
    for (const auto& entry : std::filesystem::directory_iterator(out_dir)) {
        if (entry.is_regular_file() && entry.path().extension() == ".txt") {
            path = entry.path().string();
            break;
        }
    }
    CHECK(!path.empty());

    std::ifstream in(path);
    CHECK(static_cast<bool>(in));
    int n = 0;
    int terminal = -1;
    in >> n >> terminal;
    CHECK(static_cast<bool>(in));
    for (int node = 0; node < n; ++node) {
        int action_count = 0;
        in >> action_count;
        CHECK(static_cast<bool>(in));
        for (int action = 0; action < action_count; ++action) {
            int action_id = -1;
            int successor_count = 0;
            in >> action_id >> successor_count;
            CHECK(static_cast<bool>(in));
            std::set<int> successors;
            for (int k = 0; k < successor_count; ++k) {
                int to = -1;
                double cost = 0.0;
                in >> to >> cost;
                CHECK(static_cast<bool>(in));
                successors.insert(to);
            }
            CHECK(static_cast<int>(successors.size()) == successor_count);
        }
    }
    CHECK(std::system(cleanup.c_str()) == 0);
}

void test_generator_writes_metadata_csv() {
    const std::string out_dir = "/tmp/rsp_generator_metadata";
    const std::string cleanup = "rm -rf " + out_dir;
    CHECK(std::system(cleanup.c_str()) == 0);

    const std::string cmd =
        "python3 experiments/generate_medium_graphs.py --output " + out_dir +
        " --sizes 20 --cases 2 --actions 3 --successors 5 --seed 42";
    CHECK(std::system(cmd.c_str()) == 0);

    std::ifstream in(out_dir + "/graph_metadata.csv");
    CHECK(static_cast<bool>(in));
    std::string header;
    std::getline(in, header);
    if (!header.empty() && static_cast<unsigned char>(header[0]) == 0xEF) {
        header = header.substr(3);
    }
    if (!header.empty() && header.back() == '\r') {
        header.pop_back();
    }
    std::vector<std::string> columns;
    std::size_t start = 0;
    while (true) {
        const std::size_t comma = header.find(',', start);
        if (comma == std::string::npos) {
            columns.push_back(header.substr(start));
            break;
        }
        columns.push_back(header.substr(start, comma - start));
        start = comma + 1;
    }
    CHECK(columns.size() == 13);
    CHECK(columns[0] == "graph_id");
    CHECK(columns[1] == "n");
    CHECK(columns[2] == "actions");
    CHECK(columns[3] == "case");
    CHECK(columns[4] == "base_seed");
    CHECK(columns[5] == "display_seed");
    CHECK(columns[6] == "rng_seed");
    CHECK(columns[7] == "requested_s");
    CHECK(columns[8] == "min_actual_s");
    CHECK(columns[9] == "max_actual_s");
    CHECK(columns[10] == "avg_actual_s");
    CHECK(columns[11] == "min_cost");
    CHECK(columns[12] == "max_cost");

    std::string row;
    int rows = 0;
    while (std::getline(in, row)) {
        CHECK(!row.empty());
        ++rows;
    }
    CHECK(rows == 2);
    CHECK(std::system(cleanup.c_str()) == 0);
}

void test_generator_successors_values_mode() {
    const std::string out_dir = "/tmp/rsp_generator_successors_values";
    const std::string cleanup = "rm -rf " + out_dir;
    CHECK(std::system(cleanup.c_str()) == 0);

    const std::string cmd =
        "python3 experiments/generate_medium_graphs.py --output " + out_dir +
        " --sizes 20 --cases 2 --actions 3 --successors-values 1 2 5 --seed 42";
    CHECK(std::system(cmd.c_str()) == 0);

    std::vector<std::string> txt_files;
    for (const auto& entry : std::filesystem::directory_iterator(out_dir)) {
        if (entry.is_regular_file() && entry.path().extension() == ".txt") {
            txt_files.push_back(entry.path().filename().string());
        }
    }
    CHECK(txt_files.size() == 6);

    std::ifstream meta_in(out_dir + "/graph_metadata.csv");
    CHECK(static_cast<bool>(meta_in));
    std::string line;
    std::getline(meta_in, line);
    int meta_rows = 0;
    std::set<int> requested_s_values;
    while (std::getline(meta_in, line)) {
        CHECK(!line.empty());
        ++meta_rows;
        std::size_t pos = 0;
        for (int i = 0; i < 7; ++i) {
            pos = line.find(',', pos + 1);
        }
        const std::size_t next = line.find(',', pos + 1);
        const std::string s_field = line.substr(
            pos + 1, next == std::string::npos ? std::string::npos : next - pos - 1);
        requested_s_values.insert(std::stoi(s_field));
    }
    CHECK(meta_rows == 6);
    CHECK(requested_s_values == std::set<int>({1, 2, 5}));
    CHECK(std::system(cleanup.c_str()) == 0);
}

void test_run_robustness_rejects_mixed_input_dir_with_override_s() {
    const std::string out_dir = "/tmp/rsp_robustness_override_out";
    const std::string err_path = "/tmp/rsp_robustness_override_out.err";
    const std::string cmd =
        shell_quote(std::string(RSP_RUN_ROBUSTNESS)) + " --input-dir "
        "experiment_data/official_20260521_210335/exp4_robustness/graphs "
        "--output " + out_dir + " --start 0 --max-steps 1000 --s 3 "
        "2> " + shell_quote(err_path);
    CHECK(std::system(cmd.c_str()) != 0);
    CHECK(read_text_file(err_path).find("--s cannot be used together with --input-dir") != std::string::npos);
    CHECK(std::system(("rm -rf " + out_dir).c_str()) == 0);
    std::remove(err_path.c_str());
}

void test_run_robustness_rejects_negative_max_steps() {
    const std::string out_dir = "/tmp/rsp_robustness_negative_steps_out";
    const std::string err_path = "/tmp/rsp_robustness_negative_steps_out.err";
    const std::string cmd =
        shell_quote(std::string(RSP_RUN_ROBUSTNESS)) + " --input data/toy_graph.txt "
        "--output " + out_dir + " --start 0 --max-steps -1 "
        "2> " + shell_quote(err_path);
    CHECK(std::system(cmd.c_str()) != 0);
    CHECK(read_text_file(err_path).find("--max-steps must be non-negative") != std::string::npos);
    CHECK(std::system(("rm -rf " + out_dir).c_str()) == 0);
    std::remove(err_path.c_str());
}

void test_run_runtime_rejects_nonpositive_epsilon() {
    const std::string out_dir = "/tmp/rsp_runtime_bad_epsilon_out";
    const std::string err_path = "/tmp/rsp_runtime_bad_epsilon_out.err";
    const std::string cmd =
        shell_quote(std::string(RSP_RUN_RUNTIME)) +
        " --input-dir experiment_data/official_20260521_210335/exp3_runtime/graphs "
        "--output " + out_dir + " --epsilon 0 "
        "2> " + shell_quote(err_path);
    CHECK(std::system(cmd.c_str()) != 0);
    CHECK(read_text_file(err_path).find("--epsilon must be positive and finite") != std::string::npos);
    CHECK(std::system(("rm -rf " + out_dir).c_str()) == 0);
    std::remove(err_path.c_str());
}

void test_run_runtime_rejects_nonfinite_epsilon() {
    const std::string out_dir = "/tmp/rsp_runtime_bad_nonfinite_epsilon_out";
    const std::string err_path = "/tmp/rsp_runtime_bad_nonfinite_epsilon_out.err";
    const std::string cmd =
        shell_quote(std::string(RSP_RUN_RUNTIME)) +
        " --input-dir experiment_data/official_20260521_210335/exp3_runtime/graphs "
        "--output " + out_dir + " --epsilon inf "
        "2> " + shell_quote(err_path);
    CHECK(std::system(cmd.c_str()) != 0);
    CHECK(read_text_file(err_path).find("--epsilon must be positive and finite") != std::string::npos);
    CHECK(std::system(("rm -rf " + out_dir).c_str()) == 0);
    std::remove(err_path.c_str());
}

void test_rsp_main_rejects_nonfinite_epsilon() {
    const std::string out_dir = "/tmp/rsp_main_bad_nonfinite_epsilon_out";
    const std::string err_path = "/tmp/rsp_main_bad_nonfinite_epsilon_out.err";
    const std::string cmd =
        shell_quote(std::string(RSP_MAIN_BIN)) +
        " --input data/toy_graph.txt --algorithm vi "
        "--output " + out_dir + " --epsilon nan "
        "2> " + shell_quote(err_path);
    CHECK(std::system(cmd.c_str()) != 0);
    CHECK(read_text_file(err_path).find("--epsilon must be positive and finite") != std::string::npos);
    CHECK(std::system(("rm -rf " + out_dir).c_str()) == 0);
    std::remove(err_path.c_str());
}

void test_run_robustness_uses_unique_graph_ids_for_recursive_inputs() {
    const std::string out_dir = "/tmp/rsp_robustness_graph_id_out";
    CHECK(std::system(("rm -rf " + out_dir).c_str()) == 0);
    const std::string cmd =
        shell_quote(std::string(RSP_RUN_ROBUSTNESS)) + " --input-dir experiment_data/official_20260521_210335/exp4_robustness/graphs "
        "--output " + out_dir + " --start 0 --max-steps 50";
    CHECK(std::system(cmd.c_str()) == 0);

    std::ifstream in(out_dir + "/robustness.csv");
    CHECK(static_cast<bool>(in));
    std::string line;
    std::getline(in, line);
    std::set<std::string> graph_ids;
    while (std::getline(in, line)) {
        const std::size_t comma = line.find(',');
        CHECK(comma != std::string::npos);
        graph_ids.insert(line.substr(0, comma));
    }
    CHECK(graph_ids.size() == 100);
    CHECK(std::system(("rm -rf " + out_dir).c_str()) == 0);
}

void test_run_robustness_rejects_nonpositive_s() {
    const std::string out_dir = "/tmp/rsp_robustness_bad_s_out";
    const std::string zero_err = "/tmp/rsp_robustness_bad_s_zero.err";
    const std::string neg_one_err = "/tmp/rsp_robustness_bad_s_neg_one.err";
    const std::string neg_three_err = "/tmp/rsp_robustness_bad_s_neg_three.err";
    const std::string zero_cmd =
        shell_quote(std::string(RSP_RUN_ROBUSTNESS)) + " --input data/toy_graph.txt "
        "--output " + out_dir + " --start 0 --max-steps 20 --s 0 "
        "2> " + shell_quote(zero_err);
    const std::string neg_one_cmd =
        shell_quote(std::string(RSP_RUN_ROBUSTNESS)) + " --input data/toy_graph.txt "
        "--output " + out_dir + " --start 0 --max-steps 20 --s -1 "
        "2> " + shell_quote(neg_one_err);
    const std::string neg_three_cmd =
        shell_quote(std::string(RSP_RUN_ROBUSTNESS)) + " --input data/toy_graph.txt "
        "--output " + out_dir + " --start 0 --max-steps 20 --s -3 "
        "2> " + shell_quote(neg_three_err);
    CHECK(std::system(zero_cmd.c_str()) != 0);
    CHECK(std::system(neg_one_cmd.c_str()) != 0);
    CHECK(std::system(neg_three_cmd.c_str()) != 0);
    CHECK(read_text_file(zero_err).find("--s must be positive when provided") != std::string::npos);
    CHECK(read_text_file(neg_one_err).find("--s must be positive when provided") != std::string::npos);
    CHECK(read_text_file(neg_three_err).find("--s must be positive when provided") != std::string::npos);
    CHECK(std::system(("rm -rf " + out_dir).c_str()) == 0);
    std::remove(zero_err.c_str());
    std::remove(neg_one_err.c_str());
    std::remove(neg_three_err.c_str());
}

void test_run_robustness_uses_distinct_successor_count_for_default_s() {
    const std::string graph_path = write_temp_graph_file(
        "3 2\n"
        "1\n"
        "0 2 2 1.0 2 3.0\n"
        "1\n"
        "0 1 2 1.0\n"
        "0\n",
        "duplicate_successor_graph");
    const std::string out_dir = "/tmp/rsp_distinct_s_out";
    const std::string cmd =
        shell_quote(std::string(RSP_RUN_ROBUSTNESS)) + " --input " + graph_path +
        " --output " + out_dir + " --start 0 --max-steps 20";
    CHECK(std::system(cmd.c_str()) == 0);

    std::ifstream in(out_dir + "/robustness.csv");
    CHECK(static_cast<bool>(in));
    std::string line;
    std::getline(in, line);
    std::getline(in, line);
    const std::size_t first = line.find(',');
    const std::size_t second = line.find(',', first + 1);
    CHECK(first != std::string::npos);
    CHECK(second != std::string::npos);
    CHECK(line.substr(first + 1, second - first - 1) == "1");

    std::remove(graph_path.c_str());
    CHECK(std::system(("rm -rf " + out_dir).c_str()) == 0);
}

}  // namespace

int main() {
    test_toy_vi_matches_exhaustive();
    test_small_layered_vi_matches_exhaustive();
    test_runner_exhaustive_uses_start_optimal_policy_and_statewise_values();
    test_value_iteration_reports_not_converged();
    test_runner_vi_success_requires_convergence();
    test_runner_vi_rejects_no_proper_policy_graph();
    test_runner_pi_requires_convergence();
    test_policy_iteration_accepts_max_iter_only();
    test_terminal_actions_are_rejected();
    test_negative_action_count_rejected();
    test_negative_node_count_rejected_before_resize();
    test_terminal_index_rejected_before_resize();
    test_negative_successor_count_rejected();
    test_terminal_policy_must_be_minus_one();
    test_generator_uses_distinct_filenames();
    test_generator_avoids_duplicate_successors();
    test_generator_writes_metadata_csv();
    test_generator_successors_values_mode();
    test_run_robustness_rejects_mixed_input_dir_with_override_s();
    test_run_robustness_rejects_negative_max_steps();
    test_run_runtime_rejects_nonpositive_epsilon();
    test_run_runtime_rejects_nonfinite_epsilon();
    test_rsp_main_rejects_nonfinite_epsilon();
    test_run_robustness_uses_unique_graph_ids_for_recursive_inputs();
    test_run_robustness_rejects_nonpositive_s();
    test_run_robustness_uses_distinct_successor_count_for_default_s();
    return 0;
}
