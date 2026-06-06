#pragma once

#include <cstdint>

enum class CellState : uint8_t {
    Hidden,
    Revealed,
    Flagged,
    HitMine,
};

struct Cell {
    bool is_mine = false;
    std::uint8_t adjacent_mines = 0;
    CellState state = CellState::Hidden;
};
