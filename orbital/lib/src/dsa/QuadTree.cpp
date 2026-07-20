#include "dsa/QuadTree.h"

QuadTree::QuadTree() {
  nodes.emplace(Node{{0, 0, 0}});
}

uint64_t QuadTree::layerFromSize(double sz) {
  return uint64_t(std::log2(1.0 / sz));
}

uint64_t QuadTree::tilesAtLayer(uint64_t layer) {
  return 1ull << layer;
}

void QuadTree::trySplit(std::function<bool(Node const &)> const & predicate, uint64_t root, bool joinOnFail) {
  if (!predicate(nodes[root])) {
    if (joinOnFail)
      join(root);
    return;
  }

  split(root);

  for (uint8_t i = 0; i < 4; ++i) {
    trySplit(predicate, nodes[root].children[i]);
  }
}

bool QuadTree::insert(bfc::Vec3u64 const & coord) {
  return insert(coord, 0);
}

bool QuadTree::join(uint64_t node) {
  if (!nodes.isUsed(node)) {
    return false;
  }

  if (!nodes[node].split) {
    return false;
  }

  for (uint8_t i = 0; i < 4; ++i) {
    join(nodes[node].children[i]);

    nodes.erase(nodes[node].children[i]);
    nodes[node].children[i] = -1;
  }

  nodes[node].split = false;
  return true;
}

bool QuadTree::split(uint64_t node) {
  if (!nodes.isUsed(node)) {
    return false;
  }

  if (nodes[node].split) {
    return false;
  }

  uint64_t childNodes[] = {
    (uint64_t)nodes.emplace(),
    (uint64_t)nodes.emplace(),
    (uint64_t)nodes.emplace(),
    (uint64_t)nodes.emplace(),
  };
  std::memcpy(nodes[node].children, childNodes, sizeof(childNodes));
  auto & parent = nodes[node];
  parent.split  = true;

  for (uint8_t i = 0; i < 4; ++i) {
    nodes[parent.children[i]].coord = parent.child(i);
  }
  return true;
}

void QuadTree::forLeaves(std::function<void(Node const &)> const & cb) {
  for (auto & node : nodes) {
    if (!node.split) {
      cb(node);
    }
  }
}

bool QuadTree::isSplit(uint64_t node) {
  return nodes[node].split;
}

bool QuadTree::insert(bfc::Vec3u64 const & coord, uint64_t root) {
  const int64_t diff = coord.z - nodes[root].coord.z;
  if (diff <= 0)
    return nodes[root].coord == coord;

  split(root);

  const auto    childCoord = bfc::Vec2u64(coord) / uint64_t(1ull << (diff - 1));
  const auto    localCoord = childCoord - bfc::Vec2u64(nodes[root].child(0));
  const int64_t childIndex = localCoord.y * 2 + localCoord.x;

  return insert(coord, nodes[root].children[childIndex]);
}

int64_t QuadTree::Node::layerDimensions() const {
  return tilesAtLayer(coord.z);
}

double QuadTree::Node::size() const {
  return 1.0f / layerDimensions();
}

bfc::Vec3u64 QuadTree::Node::parent() const {
  bfc::Vec3u64 parent = coord;
  parent.z -= 1;
  parent.x = parent.x / 2;
  parent.y = parent.y / 2;
  return parent;
}

bfc::Vec3u64 QuadTree::Node::child(uint8_t index) const {
  bfc::Vec3u64 child = coord;
  child.z += 1;
  child.x = child.x * 2 + (index % 2);
  child.y = child.y * 2 + (index / 2);
  return child;
}
