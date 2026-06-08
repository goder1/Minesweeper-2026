#include <gtest/gtest.h>

#include <stdexcept>

#include "../../src/game/Board.h"

TEST(BoardTest, ConstructsWithCorrectDimensions) {
    Board board(9, 9, 10);
    EXPECT_EQ(board.Width(), 9);
    EXPECT_EQ(board.Height(), 9);
}

TEST(BoardTest, ThrowsOnTooManyMines) {
    EXPECT_THROW(Board(5, 5, 25), std::invalid_argument);
}

TEST(BoardTest, ThrowsOnZeroDimension) {
    EXPECT_THROW(Board(0, 9, 5), std::invalid_argument);
    EXPECT_THROW(Board(9, 0, 5), std::invalid_argument);
}

TEST(BoardTest, FirstRevealIsNeverMine) {
    for (int i = 0; i < 50; ++i) {
        Board board(9, 9, 70);
        board.RevealCell(4, 4);
        EXPECT_FALSE(board.HasPendingRevert());
    }
}

TEST(BoardTest, RevealedCellChangesState) {
    Board board(9, 9, 10);
    board.RevealCell(4, 4);
    EXPECT_EQ(board.GetCellInfo(4, 4).state, CellState::Revealed);
}

TEST(BoardTest, RevealOutOfBoundsDoesNothing) {
    Board board(9, 9, 10);
    EXPECT_NO_THROW(board.RevealCell(-1, 0));
    EXPECT_NO_THROW(board.RevealCell(9, 9));
}

TEST(BoardTest, RevealAlreadyRevealedCellDoesNothing) {
    Board board(9, 9, 10);
    board.RevealCell(4, 4);
    EXPECT_NO_THROW(board.RevealCell(4, 4));
    EXPECT_EQ(board.GetCellInfo(4, 4).state, CellState::Revealed);
}

TEST(BoardTest, FlagHiddenCellSetsFlag) {
    Board board(9, 9, 10);
    board.ToggleFlag(0, 0);
    EXPECT_EQ(board.GetCellInfo(0, 0).state, CellState::Flagged);
}

TEST(BoardTest, FlagTogglesBackToHidden) {
    Board board(9, 9, 10);
    board.ToggleFlag(0, 0);
    board.ToggleFlag(0, 0);
    EXPECT_EQ(board.GetCellInfo(0, 0).state, CellState::Hidden);
}

TEST(BoardTest, CannotFlagRevealedCell) {
    Board board(9, 9, 10);
    board.RevealCell(4, 4);
    board.ToggleFlag(4, 4);
    EXPECT_EQ(board.GetCellInfo(4, 4).state, CellState::Revealed);
}

TEST(BoardTest, CannotRevealFlaggedCell) {
    Board board(9, 9, 10);
    board.ToggleFlag(0, 0);
    board.RevealCell(0, 0);
    EXPECT_EQ(board.GetCellInfo(0, 0).state, CellState::Flagged);
}

namespace {
// Builds a 2x1 board with a known mine at (1, 0), bypassing PlaceMines so the
// "safe first click" guarantee can't turn the only other cell into the mine
// (which would instantly win the game before these tests get to reveal it).
Board BoardWithMineAtOne() {
    Board::SavedState state;
    state.mines = {1};
    return Board(2, 1, 1, state);
}
}  // namespace

TEST(BoardTest, HittingMineSetsHitMineState) {
    Board board = BoardWithMineAtOne();
    board.RevealCell(1, 0);
    EXPECT_EQ(board.GetCellInfo(1, 0).state, CellState::HitMine);
}

TEST(BoardTest, HittingMineSetsPendingRevert) {
    Board board = BoardWithMineAtOne();
    board.RevealCell(1, 0);
    EXPECT_TRUE(board.HasPendingRevert());
}

TEST(BoardTest, HittingMineIncrementsMistakeCount) {
    Board board = BoardWithMineAtOne();
    board.RevealCell(1, 0);
    EXPECT_EQ(board.MistakeCount(), 1);
}

TEST(BoardTest, RevertMoveRestoresHiddenState) {
    Board board = BoardWithMineAtOne();
    board.RevealCell(1, 0);
    board.RevertMove();
    EXPECT_EQ(board.GetCellInfo(1, 0).state, CellState::Hidden);
    EXPECT_FALSE(board.HasPendingRevert());
}

TEST(BoardTest, NoMovesAllowedWhilePendingRevert) {
    Board board = BoardWithMineAtOne();
    board.RevealCell(1, 0);
    board.ToggleFlag(0, 0);
    EXPECT_EQ(board.GetCellInfo(0, 0).state, CellState::Hidden);
}

TEST(BoardTest, MistakeCountAccumulatesAcrossReverts) {
    Board board = BoardWithMineAtOne();
    board.RevealCell(1, 0);
    board.RevertMove();
    board.RevealCell(1, 0);
    EXPECT_EQ(board.MistakeCount(), 2);
}

TEST(BoardTest, InitialStatusIsOngoing) {
    Board board(9, 9, 10);
    EXPECT_EQ(board.Status(), GameStatus::Ongoing);
}
