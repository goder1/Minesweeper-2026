#include "Board.h"

#include <algorithm>
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

Board::Board(std::size_t width, std::size_t height, std::size_t mine_cnt, const SavedState& state)
    : Board(width, height, mine_cnt) {
    for (const std::size_t index : state.mines) {
        cells_[index].is_mine = true;
    }
    CalculateAdjacent();

    for (const std::size_t index : state.revealed) {
        cells_[index].state = CellState::Revealed;
    }
    for (const std::size_t index : state.flagged) {
        cells_[index].state = CellState::Flagged;
    }

    mistake_count_ = state.mistake_count;
    initialized_ = true;
    CheckWin();
}

Board::SavedState Board::Serialize() const {
    SavedState state;
    state.mistake_count = mistake_count_;
    for (std::size_t index = 0; index < cells_.size(); ++index) {
        const Cell& cell = cells_[index];
        if (cell.is_mine) {
            state.mines.push_back(index);
        } else if (cell.state == CellState::Revealed) {
            state.revealed.push_back(index);
        }
        if (cell.state == CellState::Flagged) {
            state.flagged.push_back(index);
        }
    }
    return state;
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
    const std::size_t clicked_index = y_coord * width_ + x_coord;

    std::vector<bool> in_opening(cells_.size(), false);
    in_opening[clicked_index] = true;
    for (int j = -1; j <= 1; ++j) {
        for (int i = -1; i <= 1; ++i) {
            if (i == 0 && j == 0) {
                continue;
            }
            if (InBounds(x_coord + i, y_coord + j)) {
                in_opening[(y_coord + j) * width_ + (x_coord + i)] = true;
            }
        }
    }

    // Cells outside the clicked cell's neighborhood are placed first; its neighbors (but
    // never the clicked cell itself) are only used as a fallback for boards too small to
    // keep the whole opening mine-free.
    std::vector<std::size_t> candidates;
    std::vector<std::size_t> opening_neighbors;
    for (std::size_t index = 0; index < cells_.size(); ++index) {
        if (index == clicked_index) {
            continue;
        }
        (in_opening[index] ? opening_neighbors : candidates).push_back(index);
    }

    std::mt19937 rng(std::random_device{}());
    std::shuffle(candidates.begin(), candidates.end(), rng);

    std::size_t placed = std::min(mine_cnt_, candidates.size());
    for (std::size_t i = 0; i < placed; ++i) {
        cells_[candidates[i]].is_mine = true;
    }

    if (placed < mine_cnt_) {
        std::shuffle(opening_neighbors.begin(), opening_neighbors.end(), rng);
        for (std::size_t i = 0; placed < mine_cnt_ && i < opening_neighbors.size();
             ++i, ++placed) {
            cells_[opening_neighbors[i]].is_mine = true;
        }
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
                    if (InBounds(x_coord + i, y_coord + j) &&
                        GetCell(x_coord + i, y_coord + j).is_mine) {
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
    if (cell.state == CellState::Revealed) {
        ChordReveal(x_coord, y_coord);
        return;
    }
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
    CheckWin();
}

void Board::CheckWin() {
    int hidden_safe = 0;
    for (const auto& cell : cells_) {
        if (!cell.is_mine &&
            (cell.state == CellState::Hidden || cell.state == CellState::Flagged)) {
            ++hidden_safe;
        }
    }
    if (hidden_safe == 0) {
        status_ = GameStatus::Won;
    }
}

void Board::ChordReveal(std::size_t x_coord, std::size_t y_coord) {
    Cell& cell = GetCell(x_coord, y_coord);
    if (cell.adjacent_mines == 0) {
        return;
    }

    int flagged = 0;
    for (int j = -1; j <= 1; ++j) {
        for (int i = -1; i <= 1; ++i) {
            if (i == 0 && j == 0) {
                continue;
            }
            if (InBounds(x_coord + i, y_coord + j) &&
                GetCell(x_coord + i, y_coord + j).state == CellState::Flagged) {
                ++flagged;
            }
        }
    }
    if (flagged != cell.adjacent_mines) {
        return;
    }

    for (int j = -1; j <= 1; ++j) {
        for (int i = -1; i <= 1; ++i) {
            if (i == 0 && j == 0) {
                continue;
            }
            if (!InBounds(x_coord + i, y_coord + j)) {
                continue;
            }
            Cell& neighbor = GetCell(x_coord + i, y_coord + j);
            if (neighbor.state != CellState::Hidden) {
                continue;
            }
            if (neighbor.is_mine) {
                neighbor.state = CellState::HitMine;
                ++mistake_count_;
                pending_revert_ = true;
                return;
            }
            RevealAllAround(x_coord + i, y_coord + j);
        }
    }

    CheckWin();
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
