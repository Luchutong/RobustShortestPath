#include "rsp/dijkstra_like.hpp"
#include "rsp/exhaustive_search.hpp"
#include "rsp/io.hpp"
#include "rsp/policy_iteration.hpp"
#include "rsp/value_iteration.hpp"

#include <cassert>
#include <cmath>
#include <vector>

namespace {

void expect_close(double actual, double expected) {
    assert(std::abs(actual - expected) < 1e-8);
}

void check_toy_values(const std::vector<double>& value) {
    expect_close(value[0], 7.0);
    expect_close(value[1], 1.0);
    expect_close(value[2], 101.0);
    expect_close(value[3], 4.0);
    expect_close(value[4], 100.0);
    expect_close(value[5], 0.0);
}

}  // namespace

int main() {
    rsp::RobustGraph graph = rsp::read_graph_txt("data/toy_graph.txt");

    auto vi = rsp::value_iteration(graph, 1e-9, 1000, true, true);
    check_toy_values(vi.value);
    assert(vi.policy[0] == 1);

    auto pi = rsp::policy_iteration(graph);
    check_toy_values(pi.value);
    assert(pi.policy[0] == 1);

    auto dij = rsp::dijkstra_like(graph);
    assert(dij.success);
    check_toy_values(dij.value);
    assert(dij.policy[0] == 1);

    auto exhaustive = rsp::exhaustive_search(graph);
    assert(exhaustive.success);
    check_toy_values(exhaustive.optimal_value_by_state);

    return 0;
}
