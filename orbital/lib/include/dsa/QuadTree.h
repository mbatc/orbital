#pragma once

#include "../core/Pool.h"
#include "../math/MathTypes.h"

class QuadTree {
public:
  struct Node {
    bfc::Vec3u64 coord;
    uint64_t     children[4];
    bool         split = false;

    int64_t      layerDimensions() const;
    double       size() const;
    bfc::Vec3u64 parent() const;
    bfc::Vec3u64 child(uint8_t index) const;
  };

  QuadTree();

  static uint64_t layerFromSize(double sz);
  static uint64_t tilesAtLayer(uint64_t layer);

  void trySplit(std::function<bool(Node const &)> const & predicate, uint64_t root = 0, bool joinOnFail = false);
  bool insert(bfc::Vec3u64 const & coord);
  bool join(uint64_t node);
  bool split(uint64_t node);
  void forLeaves(std::function<void(Node const &)> const & cb);

private:
  bool isSplit(uint64_t node);
  bool insert(bfc::Vec3u64 const & coord, uint64_t root);

  bfc::Pool<Node> nodes;
};
