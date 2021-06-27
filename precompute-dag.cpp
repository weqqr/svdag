#include <chrono>
#include <iostream>
#include <vector>
#include <bitset>
#include <algorithm>
#include <numeric>

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

void build_svdag_tiered(const Map& map, std::vector<std::vector<DAGNode>>& levels, uint32_t level_count, int x0, int y0, int z0)
{
    for (int level = level_count - 2; level >= 0; level--) {
        uint32_t size = 1 << level;

        auto& bottom_level = levels[level + 1];
        auto& current_level = levels[level];

        // Preallocating saves a *lot* of time
        current_level.reserve(pow(2, 3*level));

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

#define DO_COMPACTION 1
#if DO_COMPACTION
        // L-1 is already merged into L-2
        if (level_count - level <= 2)
            continue;

#define COMPACTION_METHOD 0
#if COMPACTION_METHOD == 0
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
        uint32_t last = bottom_level.size();
        uint32_t result = first;
        pattern[first] = result;
        while (++first != last) {
            if (!(sorted[result] == sorted[first]) && ++result != first) {
                sorted[result] = sorted[first];
            }
            mapped_pointers[pattern[first]] = result;
        }

        for (size_t i = 0; i < current_level.size(); i++) {
            for (size_t j = 0; j < 8; j++) {
                auto new_location = mapped_pointers[current_level[i].ptr[j]];
                current_level[i].ptr[j] = new_location;
            }
        }

        sorted.erase(sorted.begin() + result + 1, sorted.end());

        bottom_level = std::move(sorted);
#else
        std::vector<DAGNode> compacted;

        std::cout << current_level.size() << std::endl;
        for (size_t i = 0; i < bottom_level.size(); i++) {
            auto it = std::find(compacted.begin(), compacted.end(), bottom_level[i]);

            uint32_t index;
            if (it == compacted.end()) {
                index = compacted.size();
                compacted.push_back(bottom_level[i]);
            } else {
                index = it - compacted.begin();
            }

            // Fix pointers in current level
            for (auto& current_level_node : current_level) {
                for (uint32_t& pointer : current_level_node.ptr) {
                    if (pointer == i) {
                        pointer = index;
                    }
                }
            }
        }

        bottom_level = std::move(compacted);
#endif
#endif
    }
}

class DAG {
public:
    explicit DAG(const Map& map, uint32_t levels)
    {
        m_level_count = levels;
        m_levels.resize(levels);
        build_svdag_tiered(map, m_levels, levels, 0, 0, 0);

        /*size_t total_size = 0;
        for (size_t i = 0; i < m_levels.size(); i++) {
            std::cout << "Level " << i << " count=" << m_levels[i].size() << std::endl;
            total_size += m_levels[i].size() * sizeof(DAGNode);
        }
        std::cout << "DAG Size=" << float(total_size) / 1024 / 1024 << " MiB" << std::endl;*/

        /*for (size_t i = 0; i < m_levels.size(); i++) {
            std::cout << "Level " << i << std::endl;
            for (size_t j = 0; j < m_levels[i].size(); j++) {
                std::cout << "\t" << m_levels[i][j] << " " << std::endl;
            }
        }*/
    }

    bool get_tiered(uint32_t x, uint32_t y, uint32_t z) const {
        uint32_t pointer = 0;

        uint32_t bx;
        uint32_t by;
        uint32_t bz;

        for (int level = 0; level < m_level_count - 1; level++) {
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

    uint32_t m_level_count = 0;
    std::vector<std::vector<DAGNode>> m_levels;
};

int main()
{
    std::cout << "precompute-dag" << std::endl;

    Map map;

    auto start = std::chrono::high_resolution_clock::now();
    //for (int i = 0; i < 10; i++)
    DAG dag(map, 7);
    auto end = std::chrono::high_resolution_clock::now();
    auto dt = end - start;
    std::cout << "dt=" << std::chrono::duration_cast<std::chrono::milliseconds>(dt) << std::endl;

    for (int z = 108; z < 109; z++) {
        for (int y = 0; y < 128; y++) {
            for (int x = 64; x < 128; x++) {
                bool map_value = map.get(x, y, z);
                bool dag_value = dag.get_tiered(x, y, z);
                if (map_value != dag_value) {
                    std::cout << "error at " << x << ", " << y << ", " << z << " expected: " << map_value << std::endl;
                }
            }
        }
    }

    return 0;
}
