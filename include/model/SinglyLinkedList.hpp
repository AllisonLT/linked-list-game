#pragma once

#include "model/NodeId.hpp"

#include <cstdint>
#include <optional>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace game::model {

template <typename T>
class SinglyLinkedList {
public:
    struct Node {
        NodeId id;
        T value;
        std::optional<NodeId> next;
    };

    [[nodiscard]] bool empty() const noexcept {
        return m_Nodes.empty();
    }

    [[nodiscard]] std::size_t size() const noexcept {
        return m_Nodes.size();
    }

    [[nodiscard]] bool contains(NodeId id) const noexcept {
        return m_Nodes.contains(id);
    }

    template <typename... Args>
    NodeId emplaceNode(Args&&... args) {
        if (m_NextId == 0) {
            throw std::overflow_error("No more node IDs are available");
        }

        const NodeId id{m_NextId++};
        m_Nodes.emplace(id, Node{id, T(std::forward<Args>(args)...), std::nullopt});
        return id;
    }

    [[nodiscard]] const Node& node(NodeId id) const {
        return findNode(id);
    }

    [[nodiscard]] std::optional<NodeId> head() const noexcept {
        return m_Head;
    }

    void setHead(std::optional<NodeId> id) {
        validateOptionalNodeId(id);
        m_Head = id;
    }

    void setNext(NodeId source, std::optional<NodeId> destination) {
        validateOptionalNodeId(destination);
        findNode(source).next = destination;
    }

    void eraseNode(NodeId id) {
        if (!contains(id)) {
            throw std::out_of_range("Unknown node ID");
        }

        for (auto& [otherId, otherNode] : m_Nodes) {
            if (otherId != id && otherNode.next == id) {
                otherNode.next.reset();
            }
        }

        if (m_Head == id) {
            m_Head.reset();
        }

        m_Nodes.erase(id);
    }

    void clear() noexcept {
        m_Nodes.clear();
        m_Head.reset();
    }

    [[nodiscard]] std::vector<NodeId> downstreamNodeIds(NodeId source) const {
        std::vector<NodeId> downstream;
        std::unordered_set<NodeId, NodeIdHash> visited;
        visited.insert(source);

        const Node* current = &findNode(source);
        while (current->next && visited.insert(*current->next).second) {
            downstream.push_back(*current->next);
            current = &findNode(*current->next);
        }

        return downstream;
    }

    [[nodiscard]] bool isConsistent() const noexcept {
        if (m_Head && !contains(*m_Head)) {
            return false;
        }

        for (const auto& [id, currentNode] : m_Nodes) {
            if (currentNode.id != id || (currentNode.next && !contains(*currentNode.next))) {
                return false;
            }
        }

        return true;
    }

private:
    Node& findNode(NodeId id) {
        const auto iterator = m_Nodes.find(id);
        if (iterator == m_Nodes.end()) {
            throw std::out_of_range("Unknown node ID");
        }
        return iterator->second;
    }

    const Node& findNode(NodeId id) const {
        const auto iterator = m_Nodes.find(id);
        if (iterator == m_Nodes.end()) {
            throw std::out_of_range("Unknown node ID");
        }
        return iterator->second;
    }

    void validateOptionalNodeId(std::optional<NodeId> id) const {
        if (id && !contains(*id)) {
            throw std::out_of_range("Unknown node ID");
        }
    }

    std::unordered_map<NodeId, Node, NodeIdHash> m_Nodes;
    std::optional<NodeId> m_Head;
    std::uint32_t m_NextId = 1;
};

} // namespace game::model
