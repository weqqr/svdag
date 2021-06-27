#include <algorithm>
#include <bitset>
#include <cstdint>
#include <iostream>
#include <numeric>
#include <vector>
#include "dag.h"

std::ostream& operator<<(std::ostream& ostream, const DAGNode& node) {
    ostream << "[" << std::bitset<8>(node.children) << ": ";

    ostream << node.ptr[0];
    for (int i = 1; i < 8; i++)
        ostream << ", " << node.ptr[i];

    ostream << "]";

    return ostream;
}

uint32_t make_leaf(const Map& map, int x0, int y0, int z0)
{
    uint32_t children = 0;

    children |= map.get(x0 + 0, y0 + 0, z0 + 0) << 0;
    children |= map.get(x0 + 1, y0 + 0, z0 + 0) << 1;
    children |= map.get(x0 + 0, y0 + 1, z0 + 0) << 2;
    children |= map.get(x0 + 1, y0 + 1, z0 + 0) << 3;
    children |= map.get(x0 + 0, y0 + 0, z0 + 1) << 4;
    children |= map.get(x0 + 1, y0 + 0, z0 + 1) << 5;
    children |= map.get(x0 + 0, y0 + 1, z0 + 1) << 6;
    children |= map.get(x0 + 1, y0 + 1, z0 + 1) << 7;

    return children;
}

void build_svdag(const Map& map, std::vector<std::vector<DAGNode>>& levels, uint32_t level_count, int x0, int y0, int z0)
{
    for (int level = level_count - 2; level >= 0; level--) {
        uint32_t size = 1 << level;

        auto& bottom_level = levels[level + 1];
        auto& current_level = levels[level];

        // Preallocating saves a *lot* of time
        current_level.reserve(static_cast<size_t>(pow(2, 3*level)));

        // Build level
        for (uint32_t z = 0; z < size; z++) {
            for (uint32_t y = 0; y < size; y++) {
                for (uint32_t x = 0; x < size; x++) {
                    uint32_t bs = 1 << (level + 1);

                    DAGNode node;

                    for (uint32_t i = 0; i < 8; i++) {
                        bool bx = (i & 1) != 0;
                        bool by = (i & 2) != 0;
                        bool bz = (i & 4) != 0;

                        // L-2 is a special case: pointers are bit-packed values from L-1
                        if (level == level_count - 2) {
                            auto leaf = make_leaf(map, x0 + 4*x + 2*bx, y0 + 4*y + 2*by, z0 + 4*z + 2*bz);
                            node.children |= 1 << i;
                            node.ptr[i] = leaf;
                        } else {
                            uint32_t index = (2*z+bz)*bs*bs+(2*y+by)*bs+(2*x+bx);
                            auto& bottom_node = bottom_level[index];
                            node.children |= 1 << i;
                            node.ptr[i] = index;
                        }
                    }

                    current_level.push_back(node);
                }
            }
        }

#if REDUCE_SVO_TO_DAG
        // L-1 is already merged into L-2
        if (level_count - level <= 2)
            continue;

        std::vector<uint32_t> pattern(bottom_level.size());
        std::vector<uint32_t> mapped_pointers(bottom_level.size());
        std::iota(pattern.begin(), pattern.end(), 0);

        std::sort(pattern.begin(), pattern.end(), [&](auto& lhs, auto& rhs) {
            return bottom_level[lhs] < bottom_level[rhs];
        });

        std::vector<DAGNode> sorted(pattern.size());
        std::transform(pattern.begin(), pattern.end(), sorted.begin(), [&](uint32_t i) {
            return bottom_level[i];
        });

        uint32_t first = 0;
        uint32_t last = static_cast<uint32_t>(bottom_level.size());
        uint32_t result = first;
        pattern[first] = result;
        while (++first != last) {
            if (!(sorted[result] == sorted[first]) && ++result != first) {
                sorted[result] = sorted[first];
            }
            mapped_pointers[pattern[first]] = result;
        }

        for (auto& i : current_level) {
            for (auto& ptr : i.ptr) {
                auto new_location = mapped_pointers[ptr];
                ptr = new_location;
            }
        }

        sorted.erase(sorted.begin() + result + 1, sorted.end());

        bottom_level = std::move(sorted);
#endif
    }
}

DAG::DAG(const Map& map, uint32_t levels)
{
    m_level_count = levels;
    m_levels.resize(levels);
    build_svdag(map, m_levels, levels, 0, 0, 0);
}

bool DAG::get(uint32_t x, uint32_t y, uint32_t z) const {
    uint32_t pointer = 0;

    uint32_t bx;
    uint32_t by;
    uint32_t bz;

    for (uint32_t level = 0; level < m_level_count - 1; level++) {
        auto& node = m_levels[level][pointer];

        uint32_t size = 1 << (m_level_count - level - 1);
        bx = x / size;
        by = y / size;
        bz = z / size;

        x -= bx * size;
        y -= by * size;
        z -= bz * size;

        pointer = node.ptr[bx + 2*by + 4*bz];
    }

    return (pointer & (1 << (x + 2*y + 4*z))) != 0;
}

size_t DAG::total_size() const
{
    size_t total_size = 0;
    for (const auto& level : m_levels) {
        total_size += level.size() * sizeof(DAGNode);
    }
    return total_size;
}

std::vector<uint32_t> DAG::flatten() const
{
    std::vector<uint32_t> output;
}
