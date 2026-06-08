#pragma once

#include <vector>

#include "Cell.h"

enum class GameStatus {
    Ongoing,
    Won,
};

class Board {
public:
    struct SavedState {
        std::vector<std::size_t> mines;
        std::vector<std::size_t> revealed;
        std::vector<std::size_t> flagged;
        int mistake_count = 0;
    };

    Board(std::size_t width, std::size_t height, std::size_t mine_cnt);
    Board(std::size_t width, std::size_t height, std::size_t mine_cnt, const SavedState& state);

    [[nodiscard]] SavedState Serialize() const;

    void RevealCell(std::size_t x_coord, std::size_t y_coord);
    void ToggleFlag(std::size_t x_coord, std::size_t y_coord);
    void RevertMove();

    [[nodiscard]] GameStatus Status() const;
    [[nodiscard]] const Cell& GetCellInfo(std::size_t x_coord, std::size_t y_coord) const;
    [[nodiscard]] bool HasPendingRevert() const;
    [[nodiscard]] int MistakeCount() const;

    [[nodiscard]] std::size_t Width() const { return width_; }
    [[nodiscard]] std::size_t Height() const { return height_; }

private:
    void PlaceMines(std::size_t x_coord, std::size_t y_coord);
    void CalculateAdjacent();
    void RevealAllAround(std::size_t x_coord, std::size_t y_coord);
    void ChordReveal(std::size_t x_coord, std::size_t y_coord);
    void CheckWin();
    [[nodiscard]] bool InBounds(std::size_t x_coord, std::size_t y_coord) const;
    Cell& GetCell(std::size_t x_coord, std::size_t y_coord);

    std::size_t width_;
    std::size_t height_;
    std::size_t mine_cnt_;
    bool initialized_ = false;
    bool pending_revert_ = false;
    int mistake_count_ = 0;
    GameStatus status_ = GameStatus::Ongoing;
    std::vector<Cell> cells_;
};
