#pragma once

#include <cstddef>

enum class Difficulty {
    Beginner,
    Intermediate,
    Expert,
    Custom,
};

struct GameConfig {
    std::size_t width = 0;
    std::size_t height = 0;
    std::size_t mine_cnt = 0;
    Difficulty difficulty = Difficulty::Custom;
};

GameConfig PresetConfig(Difficulty difficulty);
