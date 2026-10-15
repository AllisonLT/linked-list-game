#include "render/CardHitTest.hpp"
#include "model/SinglyLinkedList.hpp"
#include "render/NodeLayout.hpp"
#include "view/NodeViewState.hpp"

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {

void require(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "Test failed: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

void testCreateAndConnectNodes() {
    game::model::SinglyLinkedList<int> list;
    const auto first = list.emplaceNode(10);
    const auto second = list.emplaceNode(20);

    list.setHead(first);
    list.setNext(first, second);

    require(list.size() == 2, "two nodes are stored");
    require(list.head() == first, "head points to the first node");
    require(list.node(first).value == 10, "first node preserves its value");
    require(list.node(first).next == second, "first node points to second node");
    require(list.isConsistent(), "connected list is consistent");
}

void testNullLinksAndDeletion() {
    game::model::SinglyLinkedList<int> list;
    const auto first = list.emplaceNode(10);
    const auto second = list.emplaceNode(20);

    list.setHead(first);
    list.setNext(first, second);
    list.setNext(first, std::nullopt);

    require(!list.node(first).next, "links can be cleared explicitly");

    list.setNext(first, second);
    list.eraseNode(second);

    require(!list.node(first).next, "incoming links are cleared when a node is deleted");
    require(list.head() == first, "unrelated head is retained");
    require(list.isConsistent(), "deletion cannot leave dangling IDs");

    list.eraseNode(first);
    require(!list.head(), "deleting the head clears the head");
    require(list.empty(), "all nodes are deleted");
}

void testRejectsUnknownNodes() {
    game::model::SinglyLinkedList<int> list;
    const auto first = list.emplaceNode(10);

    bool rejectedDestination = false;
    try {
        list.setNext(first, game::model::NodeId{999});
    } catch (const std::out_of_range&) {
        rejectedDestination = true;
    }

    bool rejectedSource = false;
    try {
        list.setNext(game::model::NodeId{999}, first);
    } catch (const std::out_of_range&) {
        rejectedSource = true;
    }

    require(rejectedDestination, "links to unknown destination nodes are rejected");
    require(rejectedSource, "links from unknown source nodes are rejected");
    require(list.isConsistent(), "failed edits leave the list consistent");
}

void testSupportsUnicodePayloads() {
    game::model::SinglyLinkedList<std::u32string> list;
    const auto node = list.emplaceNode(U"\U0001F680");

    require(list.node(node).value == U"\U0001F680", "generic node values preserve Unicode data");
}

void testThreeNodeExample() {
    game::model::SinglyLinkedList<std::string> list;
    const auto nodeA = list.emplaceNode("A");
    const auto nodeB = list.emplaceNode("B");
    const auto nodeC = list.emplaceNode("C");

    list.setHead(nodeA);
    list.setNext(nodeA, nodeB);
    list.setNext(nodeB, nodeC);

    require(list.head() == nodeA, "head points to node A");
    require(list.node(nodeA).next == nodeB, "node A points to node B");
    require(list.node(nodeB).next == nodeC, "node B points to node C");
    require(!list.node(nodeC).next, "node C has a null endpoint");
    require(list.isConsistent(), "three-node example is consistent");
}

void testDisconnectedThreeNodeExample() {
    game::model::SinglyLinkedList<std::string> list;
    const auto nodeA = list.emplaceNode("A");
    const auto nodeB = list.emplaceNode("B");
    const auto nodeC = list.emplaceNode("C");

    list.setHead(nodeA);
    list.setNext(nodeA, nodeB);

    require(list.head() == nodeA, "head points to node A in the incorrect example");
    require(!list.node(nodeB).next, "node B incorrectly has a null endpoint");
    require(!list.node(nodeC).next, "disconnected node C has a null endpoint");
    require(list.isConsistent(), "disconnected nodes are structurally consistent");
}

void testNodeViewStateIsSeparateFromListData() {
    game::model::SinglyLinkedList<int> list;
    const auto node = list.emplaceNode(42);
    game::view::NodeViewState viewState{.id = node, .x = 100.0f, .y = 200.0f};

    viewState.x = 350.0f;
    viewState.y = 475.0f;

    require(list.node(viewState.id).value == 42, "moving a card does not change its node value");
    require(list.node(viewState.id).id == node, "view state retains the node identity");
    require(viewState.x == 350.0f && viewState.y == 475.0f, "view state stores card position");
}

void testCardHitTesting() {
    const game::view::NodeViewState node{
        .id = game::model::NodeId{1},
        .x = 100.0f,
        .y = 200.0f,
    };

    require(game::render::isPointInsideCard(node, 100.0f, 200.0f), "card includes its top-left edge");
    require(
        game::render::isPointInsideCard(
            node,
            100.0f + game::render::NODE_CARD_WIDTH,
            200.0f + game::render::NODE_CARD_HEIGHT),
        "card includes its bottom-right edge");
    require(game::render::isPointInsideCard(node, 200.0f, 300.0f), "card includes interior points");
    require(!game::render::isPointInsideCard(node, 99.0f, 200.0f), "card excludes points left of its body");
    require(!game::render::isPointInsideCard(node, 100.0f, 401.0f), "card excludes points below its body");
    require(
        !game::render::isPointInsideCard(
            node,
            100.0f + game::render::NODE_CARD_WIDTH + 10.0f,
            300.0f),
        "card excludes the next pointer");
    require(
        !game::render::isPointInsideCard(node, 200.0f, 200.0f),
        "card excludes the external-reference dock");
}

void testDefinesNodeLayout() {
    const auto layout = game::render::makeNodeLayout(100.0f, 200.0f);

    require(layout.card.width == layout.card.height, "node body is square");
    require(
        layout.value.x > layout.card.x && layout.value.y > layout.card.y &&
            layout.value.x + layout.value.width < layout.card.x + layout.card.width &&
            layout.value.y + layout.value.height < layout.card.y + layout.card.height,
        "value region is contained within the card");
    require(
        layout.nextLabel.x > layout.card.x && layout.nextLabel.y > layout.card.y &&
            layout.nextLabel.y + layout.nextLabel.height < layout.card.y + layout.card.height,
        "next label region is contained within the card");
    require(
        layout.referenceDock.centerX == layout.card.x + layout.card.width / 2.0f &&
            layout.referenceDock.centerY > layout.card.y &&
            layout.referenceDock.centerY - layout.referenceDock.radius < layout.card.y &&
            layout.referenceDock.radius > 0.0f,
        "reference dock is a more-than-semicircular top concavity");
    require(
        layout.outputSocket.centerX > layout.card.x + layout.card.width &&
            layout.outputSocket.centerX - layout.outputSocket.radius < layout.card.x + layout.card.width &&
            layout.outputSocket.centerY > layout.card.y &&
            layout.outputSocket.centerY < layout.card.y + layout.card.height,
        "output socket protrudes from the card edge");
}

} // namespace

int main() {
    testCreateAndConnectNodes();
    testNullLinksAndDeletion();
    testRejectsUnknownNodes();
    testSupportsUnicodePayloads();
    testThreeNodeExample();
    testDisconnectedThreeNodeExample();
    testNodeViewStateIsSeparateFromListData();
    testCardHitTesting();
    testDefinesNodeLayout();
}
