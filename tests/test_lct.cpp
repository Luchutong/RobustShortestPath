#include "rsp/exhaustive_search.hpp"
#include "rsp/io.hpp"
#include "rsp/runner.hpp"
#include "rsp/value_iteration.hpp"

#include <cassert>
#include <cmath>
#include <vector>

namespace {

void expect_close(double actual, double expected) {
    assert(std::abs(actual - expected) < 1e-8);
}

void expect_vectors_close(
    const std::vector<double>& actual,
    const std::vector<double>& expected
) {
    assert(actual.size() == expected.size());
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

void test_toy_vi_matches_exhaustive() {
    const rsp::RobustGraph graph = rsp::read_graph_txt("data/toy_graph.txt");
    const auto vi = rsp::value_iteration(graph, 1e-9, 1000, true, false);
    const auto exhaustive = rsp::exhaustive_search(graph);

    assert(vi.converged);
    assert(exhaustive.success);
    expect_vectors_close(vi.value, exhaustive.value);
    assert(vi.policy[0] == exhaustive.policy[0]);
}

void test_small_layered_vi_matches_exhaustive() {
    const rsp::RobustGraph graph = make_small_layered_graph();
    const auto vi = rsp::value_iteration(graph, 1e-9, 1000, true, false);
    const auto exhaustive = rsp::exhaustive_search(graph);

    assert(vi.converged);
    assert(exhaustive.success);
    expect_vectors_close(vi.value, exhaustive.value);
    assert(vi.policy[0] == 1);
    expect_close(vi.value[0], 3.0);
    expect_close(vi.value[1], 3.0);
    expect_close(vi.value[2], 1.0);
    expect_close(vi.value[3], 0.0);
}

void test_value_iteration_reports_not_converged() {
    const rsp::RobustGraph graph = rsp::read_graph_txt("data/toy_graph.txt");
    const auto vi = rsp::value_iteration(graph, 1e-9, 1, true, false);
    assert(!vi.converged);
    assert(vi.iterations == 1);
}

void test_runner_vi_success_requires_convergence() {
    const rsp::RobustGraph graph = rsp::read_graph_txt("data/toy_graph.txt");
    rsp::AlgorithmOptions options;
    options.max_iter = 1;
    options.epsilon = 1e-9;
    options.init_with_inf = true;
    options.save_history = false;

    const auto run = rsp::run_algorithm(graph, "vi", options);
    assert(!run.converged);
    assert(!run.success);
}

}  // namespace

int main() {
    test_toy_vi_matches_exhaustive();
    test_small_layered_vi_matches_exhaustive();
    test_value_iteration_reports_not_converged();
    test_runner_vi_success_requires_convergence();
    return 0;
}
