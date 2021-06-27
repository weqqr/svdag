#include <chrono>
#include <iostream>
#include "dag.h"

int main()
{
    std::cout << "precompute-dag" << std::endl;

    Map map;

    auto start = std::chrono::high_resolution_clock::now();
    DAG dag(map, 7);
    auto end = std::chrono::high_resolution_clock::now();
    auto dt = end - start;
    std::cout << "dt=" << std::chrono::duration_cast<std::chrono::milliseconds>(dt) << std::endl;

    for (int z = 108; z < 109; z++) {
        for (int y = 0; y < 128; y++) {
            for (int x = 64; x < 128; x++) {
                bool map_value = map.get(x, y, z);
                bool dag_value = dag.get(x, y, z);
                if (map_value != dag_value) {
                    std::cout << "error at " << x << ", " << y << ", " << z << " expected: " << map_value << std::endl;
                }
            }
        }
    }

    return 0;
}
