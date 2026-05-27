#include "rsp/baseline.hpp"
#include "rsp/dijkstra_like.hpp"
#include "rsp/io.hpp"
#include "rsp/value_iteration.hpp"

#include <cassert>
#include <cmath>
#include <cstdlib>
#include <iostream>
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

rsp::RobustGraph make_rollout_tie_graph() {
    rsp::RobustGraph graph;
    graph.n = 4;
    graph.terminal = 3;
    graph.nodes.resize(graph.n);
    for (int i = 0; i < graph.n; ++i) {
        graph.nodes[i].id = i;
    }
    graph.nodes[0].actions.push_back({0, "", {{2, 1.0}, {1, 1.0}}});
    graph.nodes[1].actions.push_back({0, "", {{3, 0.0}}});
    graph.nodes[2].actions.push_back({0, "", {{3, 0.0}}});
    return graph;
}

void test_toy_dijkstra_like() {
    const rsp::RobustGraph graph = rsp::read_graph_txt("data/toy_graph.txt");
    const auto result = rsp::dijkstra_like(graph);
    CHECK(result.success);
    CHECK(result.finalized_count == graph.n);
    const std::vector<int> expected_order = {5, 1, 3, 0, 4, 2};
    CHECK(result.finalize_order == expected_order);
    expect_close(result.value[0], 7.0);
    CHECK(result.policy[0] == 1);
}

void test_dijkstra_like_failure_modes() {
    const auto deadlock = rsp::dijkstra_like(make_deadlock_graph());
    CHECK(!deadlock.success);
    CHECK(deadlock.finalized_count == 1);
}

void test_toy_baselines_and_rollout() {
    const rsp::RobustGraph graph = rsp::read_graph_txt("data/toy_graph.txt");
    const auto nominal = rsp::deterministic_dijkstra_baseline(
        graph, rsp::DeterministicMode::Nominal);
    const auto bestcase = rsp::deterministic_dijkstra_baseline(
        graph, rsp::DeterministicMode::BestCase);
    const auto worst_immediate = rsp::deterministic_dijkstra_baseline(
        graph, rsp::DeterministicMode::WorstImmediate);

    CHECK(nominal.success);
    CHECK(bestcase.success);
    CHECK(worst_immediate.success);
    CHECK(nominal.policy[0] == 0);
    CHECK(bestcase.policy[0] == 0);
    CHECK(worst_immediate.policy[0] == 0);

    const auto nominal_rollout = rsp::adversarial_rollout(
        graph, nominal.policy, nominal.value, 0, 20);
    CHECK(nominal_rollout.terminated);
    expect_close(nominal_rollout.cost, 102.0);

    const auto robust = rsp::value_iteration(graph, 1e-9, 1000, true, true);
    const auto robust_rollout = rsp::adversarial_rollout(
        graph, robust.policy, robust.value, 0, 20);
    CHECK(robust_rollout.terminated);
    expect_close(robust_rollout.cost, 7.0);
}

void test_baseline_rejects_negative_costs() {
    const rsp::RobustGraph graph = make_negative_graph();
    bool threw_nominal = false;
    bool threw_bestcase = false;
    bool threw_worst = false;
    try {
        (void)rsp::deterministic_dijkstra_baseline(
            graph, rsp::DeterministicMode::Nominal);
    } catch (const std::invalid_argument&) {
        threw_nominal = true;
    }
    try {
        (void)rsp::deterministic_dijkstra_baseline(
            graph, rsp::DeterministicMode::BestCase);
    } catch (const std::invalid_argument&) {
        threw_bestcase = true;
    }
    try {
        (void)rsp::deterministic_dijkstra_baseline(
            graph, rsp::DeterministicMode::WorstImmediate);
    } catch (const std::invalid_argument&) {
        threw_worst = true;
    }

    CHECK(threw_nominal);
    CHECK(threw_bestcase);
    CHECK(threw_worst);
}

void test_negative_costs_are_rejected_by_graph_contract() {
    bool threw = false;
    try {
        (void)make_negative_graph().validate();
    } catch (const std::invalid_argument&) {
        threw = true;
    }
    CHECK(threw);
}

void test_baseline_modes_can_choose_different_actions() {
    const rsp::RobustGraph graph = make_mode_separation_graph();
    const auto nominal = rsp::deterministic_dijkstra_baseline(
        graph, rsp::DeterministicMode::Nominal);
    const auto bestcase = rsp::deterministic_dijkstra_baseline(
        graph, rsp::DeterministicMode::BestCase);
    const auto worst_immediate = rsp::deterministic_dijkstra_baseline(
        graph, rsp::DeterministicMode::WorstImmediate);

    CHECK(nominal.success);
    CHECK(bestcase.success);
    CHECK(worst_immediate.success);
    CHECK(nominal.policy[0] == 0);
    CHECK(bestcase.policy[0] == 1);
    CHECK(worst_immediate.policy[0] == 2);
}

void test_baseline_plans_over_full_deterministic_distance() {
    const rsp::RobustGraph graph = make_deterministic_planning_graph();
    const auto nominal = rsp::deterministic_dijkstra_baseline(
        graph, rsp::DeterministicMode::Nominal);
    const auto bestcase = rsp::deterministic_dijkstra_baseline(
        graph, rsp::DeterministicMode::BestCase);
    const auto worst_immediate = rsp::deterministic_dijkstra_baseline(
        graph, rsp::DeterministicMode::WorstImmediate);

    CHECK(nominal.success);
    CHECK(bestcase.success);
    CHECK(worst_immediate.success);
    CHECK(nominal.policy[0] == 1);
    CHECK(bestcase.policy[0] == 1);
    CHECK(worst_immediate.policy[0] == 1);
    expect_close(nominal.value[0], 6.0);
    expect_close(bestcase.value[0], 6.0);
    expect_close(worst_immediate.value[0], 6.0);
}

void test_rollout_rejects_mismatched_sizes() {
    const rsp::RobustGraph graph = rsp::read_graph_txt("data/toy_graph.txt");
    const auto bad_policy_rollout = rsp::adversarial_rollout(
        graph, std::vector<int>{0, 0}, std::vector<double>(graph.n, 0.0), 0, 20);
    CHECK(!bad_policy_rollout.terminated);
    CHECK(bad_policy_rollout.path.empty());

    const auto bad_value_rollout = rsp::adversarial_rollout(
        graph, std::vector<int>(graph.n, 0), std::vector<double>{0.0, 0.0}, 0, 20);
    CHECK(!bad_value_rollout.terminated);
    CHECK(bad_value_rollout.path.empty());
}

void test_rollout_tie_breaks_by_smallest_successor_id() {
    const rsp::RobustGraph graph = make_rollout_tie_graph();
    const std::vector<int> policy = {0, 0, 0, -1};
    const std::vector<double> value = {0.0, 0.0, 0.0, 0.0};
    const auto rollout = rsp::adversarial_rollout(graph, policy, value, 0, 10);
    CHECK(rollout.terminated);
    CHECK(rollout.path.size() == 3);
    CHECK(rollout.path[0] == 0);
    CHECK(rollout.path[1] == 1);
    CHECK(rollout.path[2] == 3);
}

}  // namespace

int main() {
    test_toy_dijkstra_like();
    test_dijkstra_like_failure_modes();
    test_toy_baselines_and_rollout();
    test_baseline_rejects_negative_costs();
    test_negative_costs_are_rejected_by_graph_contract();
    test_baseline_modes_can_choose_different_actions();
    test_baseline_plans_over_full_deterministic_distance();
    test_rollout_rejects_mismatched_sizes();
    test_rollout_tie_breaks_by_smallest_successor_id();
    return 0;
}
