#include "Board.h"

#include <algorithm>
#include <numeric>
#include <random>
#include <stdexcept>

#include "Cell.h"

Board::Board(std::size_t width, std::size_t height, std::size_t mine_cnt)
    : width_(width), height_(height), mine_cnt_(mine_cnt), cells_(width * height) {
    if (width == 0 || height == 0) {
        throw std::invalid_argument("Board dimensions must be positive");
    }
    if (mine_cnt >= width * height) {
        throw std::invalid_argument("Mine count must be less than total cells");
    }
}

bool Board::InBounds(std::size_t x_coord, std::size_t y_coord) const {
    return x_coord < width_ && y_coord < height_;
}

Cell& Board::GetCell(std::size_t x_coord, std::size_t y_coord) {
    return cells_[y_coord * width_ + x_coord];
}

const Cell& Board::GetCellInfo(std::size_t x_coord, std::size_t y_coord) const {
    return cells_[y_coord * width_ + x_coord];
}

void Board::PlaceMines(std::size_t x_coord, std::size_t y_coord) {
    std::vector<std::size_t> indices(width_ * height_);
    std::iota(indices.begin(), indices.end(), 0);

    indices.erase(indices.begin() + static_cast<std::ptrdiff_t>(y_coord * width_ + x_coord));

    std::mt19937 rng(std::random_device{}());
    std::shuffle(indices.begin(), indices.end(), rng);

    for (std::size_t i = 0; i < mine_cnt_; ++i) {
        cells_[indices[i]].is_mine = true;
    }

    CalculateAdjacent();
}

void Board::CalculateAdjacent() {
    for (std::size_t y_coord = 0; y_coord < height_; ++y_coord) {
        for (std::size_t x_coord = 0; x_coord < width_; ++x_coord) {
            if (GetCell(x_coord, y_coord).is_mine) {
                continue;
            }
            std::uint8_t count = 0;
            for (int j = -1; j <= 1; ++j) {
                for (int i = -1; i <= 1; ++i) {
                    if (i == 0 && j == 0) {
                        continue;
                    }
                    if (InBounds(x_coord + i, y_coord + j) && GetCell(x_coord + i, y_coord + j).is_mine) {
                        ++count;
                    }
                }
            }
            GetCell(x_coord, y_coord).adjacent_mines = count;
        }
    }
}

void Board::RevealCell(std::size_t x_coord, std::size_t y_coord) {
    if (!InBounds(x_coord, y_coord) || status_ != GameStatus::Ongoing || pending_revert_) {
        return;
    }

    if (!initialized_) {
        PlaceMines(x_coord, y_coord);
        initialized_ = true;
    }

    Cell& cell = GetCell(x_coord, y_coord);
    if (cell.state != CellState::Hidden) {
        return;
    }

    if (cell.is_mine) {
        cell.state = CellState::HitMine;
        ++mistake_count_;
        pending_revert_ = true;
        return;
    }

    RevealAllAround(x_coord, y_coord);

    int hidden_safe = 0;
    for (const auto& c : cells_) {
        if (!c.is_mine && (c.state == CellState::Hidden || c.state == CellState::Flagged)) {
            ++hidden_safe;
        }
    }
    if (hidden_safe == 0) {
        status_ = GameStatus::Won;
    }
}

void Board::RevertMove() {
    if (!pending_revert_) {
        return;
    }
    for (auto& cell : cells_) {
        if (cell.state == CellState::HitMine) {
            cell.state = CellState::Hidden;
            break;
        }
    }
    pending_revert_ = false;
}

void Board::RevealAllAround(std::size_t x_coord, std::size_t y_coord) {
    if (!InBounds(x_coord, y_coord)) {
        return;
    }
    Cell& cell = GetCell(x_coord, y_coord);
    if (cell.state != CellState::Hidden || cell.is_mine) {
        return;
    }

    cell.state = CellState::Revealed;

    if (cell.adjacent_mines == 0) {
        for (int j = -1; j <= 1; ++j) {
            for (int i = -1; i <= 1; ++i) {
                if (i == 0 && j == 0) {
                    continue;
                }
                RevealAllAround(x_coord + i, y_coord + j);
            }
        }
    }
}

void Board::ToggleFlag(std::size_t x_coord, std::size_t y_coord) {
    if (!InBounds(x_coord, y_coord) || status_ != GameStatus::Ongoing || pending_revert_) {
        return;
    }
    Cell& cell = GetCell(x_coord, y_coord);
    if (cell.state == CellState::Hidden) {
        cell.state = CellState::Flagged;
    } else if (cell.state == CellState::Flagged) {
        cell.state = CellState::Hidden;
    }
}

GameStatus Board::Status() const {
    return status_;
}

bool Board::HasPendingRevert() const {
    return pending_revert_;
}

int Board::MistakeCount() const {
    return mistake_count_;
}
