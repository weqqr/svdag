#pragma once

#include <iosfwd>

#define REDUCE_SVO_TO_DAG 1

class Map {
public:
    Map() = default;

    bool get(int x, int y, int z) const
    {
        return ((x % (y+1)) ^ (y % 2)) == 0;
    }

private:
};

struct DAGNode {
    uint32_t children = 0;
    uint32_t ptr[8] = {0};

    DAGNode() {}
    explicit DAGNode(uint32_t children) : children(children) {}

    bool operator==(const DAGNode& rhs) const {
        return !memcmp(&this->ptr, &rhs.ptr, 8*sizeof(uint32_t));
    }

    bool operator<(const DAGNode& rhs) const {
        return memcmp(&this->ptr, &rhs.ptr, 8*sizeof(uint32_t)) < 0;
    }
};

std::ostream& operator<<(std::ostream& ostream, const DAGNode& node);

class DAG {
public:
    explicit DAG(const Map& map, uint32_t levels);
    bool get(uint32_t x, uint32_t y, uint32_t z) const;

    uint32_t m_level_count = 0;
    std::vector<std::vector<DAGNode>> m_levels;
};
