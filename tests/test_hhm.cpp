#include "rsp/baseline.hpp"
#include "rsp/dijkstra_like.hpp"
#include "rsp/io.hpp"
#include "rsp/value_iteration.hpp"

#include <cassert>
#include <cmath>
#include <vector>

namespace {

void expect_close(double actual, double expected) {
    assert(std::abs(actual - expected) < 1e-8);
}

rsp::RobustGraph make_deadlock_graph() {
    rsp::RobustGraph graph;
    graph.n = 3;
    graph.terminal = 2;
    graph.nodes.resize(graph.n);
    for (int i = 0; i < graph.n; ++i) {
        graph.nodes[i].id = i;
    }
    graph.nodes[0].actions.push_back({0, "", {{1, 1.0}}});
    graph.nodes[1].actions.push_back({0, "", {{0, 1.0}}});
    return graph;
}

rsp::RobustGraph make_negative_graph() {
    rsp::RobustGraph graph;
    graph.n = 2;
    graph.terminal = 1;
    graph.nodes.resize(graph.n);
    graph.nodes[0].id = 0;
    graph.nodes[1].id = 1;
    graph.nodes[0].actions.push_back({0, "", {{1, -1.0}}});
    return graph;
}

rsp::RobustGraph make_deterministic_planning_graph() {
    rsp::RobustGraph graph;
    graph.n = 4;
    graph.terminal = 3;
    graph.nodes.resize(graph.n);
    for (int i = 0; i < graph.n; ++i) {
        graph.nodes[i].id = i;
    }
    graph.nodes[0].actions.push_back({0, "", {{1, 1.0}}});
    graph.nodes[0].actions.push_back({1, "", {{2, 5.0}}});
    graph.nodes[1].actions.push_back({0, "", {{3, 100.0}}});
    graph.nodes[2].actions.push_back({0, "", {{3, 1.0}}});
    return graph;
}

rsp::RobustGraph make_mode_separation_graph() {
    rsp::RobustGraph graph;
    graph.n = 5;
    graph.terminal = 4;
    graph.nodes.resize(graph.n);
    for (int i = 0; i < graph.n; ++i) {
        graph.nodes[i].id = i;
    }

    graph.nodes[0].actions.push_back({0, "", {{1, 1.0}, {4, 100.0}}});
    graph.nodes[0].actions.push_back({1, "", {{2, 10.0}, {4, 0.5}}});
    graph.nodes[0].actions.push_back({2, "", {{3, 3.0}}});
    graph.nodes[1].actions.push_back({0, "", {{4, 1.0}}});
    graph.nodes[2].actions.push_back({0, "", {{4, 1.0}}});
    graph.nodes[3].actions.push_back({0, "", {{4, 1.0}}});
    return graph;
}

void test_toy_dijkstra_like() {
    const rsp::RobustGraph graph = rsp::read_graph_txt("data/toy_graph.txt");
    const auto result = rsp::dijkstra_like(graph);
    assert(result.success);
    assert(result.finalized_count == graph.n);
    const std::vector<int> expected_order = {5, 1, 3, 0, 4, 2};
    assert(result.finalize_order == expected_order);
    expect_close(result.value[0], 7.0);
    assert(result.policy[0] == 1);
}

void test_dijkstra_like_failure_modes() {
    const auto deadlock = rsp::dijkstra_like(make_deadlock_graph());
    assert(!deadlock.success);
    assert(deadlock.finalized_count == 1);

    const auto negative = rsp::dijkstra_like(make_negative_graph());
    assert(!negative.success);
    assert(negative.finalized_count == 1);
}

void test_toy_baselines_and_rollout() {
    const rsp::RobustGraph graph = rsp::read_graph_txt("data/toy_graph.txt");
    const auto nominal = rsp::deterministic_dijkstra_baseline(
        graph, rsp::DeterministicMode::Nominal);
    const auto bestcase = rsp::deterministic_dijkstra_baseline(
        graph, rsp::DeterministicMode::BestCase);
    const auto worst_immediate = rsp::deterministic_dijkstra_baseline(
        graph, rsp::DeterministicMode::WorstImmediate);

    assert(nominal.success);
    assert(bestcase.success);
    assert(worst_immediate.success);
    assert(nominal.policy[0] == 0);
    assert(bestcase.policy[0] == 0);
    assert(worst_immediate.policy[0] == 0);

    const auto nominal_rollout = rsp::adversarial_rollout(
        graph, nominal.policy, nominal.value, 0, 20);
    assert(nominal_rollout.terminated);
    expect_close(nominal_rollout.cost, 102.0);

    const auto robust = rsp::value_iteration(graph, 1e-9, 1000, true, true);
    const auto robust_rollout = rsp::adversarial_rollout(
        graph, robust.policy, robust.value, 0, 20);
    assert(robust_rollout.terminated);
    expect_close(robust_rollout.cost, 7.0);
}

void test_baseline_rejects_negative_costs() {
    const rsp::RobustGraph graph = make_negative_graph();
    const auto nominal = rsp::deterministic_dijkstra_baseline(
        graph, rsp::DeterministicMode::Nominal);
    const auto bestcase = rsp::deterministic_dijkstra_baseline(
        graph, rsp::DeterministicMode::BestCase);
    const auto worst_immediate = rsp::deterministic_dijkstra_baseline(
        graph, rsp::DeterministicMode::WorstImmediate);

    assert(!nominal.success);
    assert(!bestcase.success);
    assert(!worst_immediate.success);
}

void test_baseline_modes_can_choose_different_actions() {
    const rsp::RobustGraph graph = make_mode_separation_graph();
    const auto nominal = rsp::deterministic_dijkstra_baseline(
        graph, rsp::DeterministicMode::Nominal);
    const auto bestcase = rsp::deterministic_dijkstra_baseline(
        graph, rsp::DeterministicMode::BestCase);
    const auto worst_immediate = rsp::deterministic_dijkstra_baseline(
        graph, rsp::DeterministicMode::WorstImmediate);

    assert(nominal.success);
    assert(bestcase.success);
    assert(worst_immediate.success);
    assert(nominal.policy[0] == 0);
    assert(bestcase.policy[0] == 1);
    assert(worst_immediate.policy[0] == 2);
}

void test_baseline_plans_over_full_deterministic_distance() {
    const rsp::RobustGraph graph = make_deterministic_planning_graph();
    const auto nominal = rsp::deterministic_dijkstra_baseline(
        graph, rsp::DeterministicMode::Nominal);
    const auto bestcase = rsp::deterministic_dijkstra_baseline(
        graph, rsp::DeterministicMode::BestCase);
    const auto worst_immediate = rsp::deterministic_dijkstra_baseline(
        graph, rsp::DeterministicMode::WorstImmediate);

    assert(nominal.success);
    assert(bestcase.success);
    assert(worst_immediate.success);
    assert(nominal.policy[0] == 1);
    assert(bestcase.policy[0] == 1);
    assert(worst_immediate.policy[0] == 1);
    expect_close(nominal.value[0], 6.0);
    expect_close(bestcase.value[0], 6.0);
    expect_close(worst_immediate.value[0], 6.0);
}

}  // namespace

int main() {
    test_toy_dijkstra_like();
    test_dijkstra_like_failure_modes();
    test_toy_baselines_and_rollout();
    test_baseline_rejects_negative_costs();
    test_baseline_modes_can_choose_different_actions();
    test_baseline_plans_over_full_deterministic_distance();
    return 0;
}
