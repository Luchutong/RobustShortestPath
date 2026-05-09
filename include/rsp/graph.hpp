#pragma once

#include <stdexcept>
#include <string>
#include <vector>

namespace rsp {

struct Transition {
    int to = -1;
    double cost = 0.0;
};

struct Action {
    int action_id = -1;
    std::string name;
    std::vector<Transition> trans;
};

struct Node {
    int id = -1;
    std::vector<Action> actions;
};

class RobustGraph {
public:
    int n = 0;
    int terminal = -1;
    std::vector<Node> nodes;

    bool is_terminal(int x) const {
        return x == terminal;
    }

    int total_actions() const;
    int total_transitions() const;
    void validate() const;
};

}  // namespace rsp

