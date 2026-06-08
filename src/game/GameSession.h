#pragma once

#include <string>

#include "Board.h"
#include "Difficulty.h"

class GameSession {
public:
    GameSession(GameConfig config, std::string player_id);
    GameSession(GameConfig config, std::string player_id, const Board::SavedState& saved_state);

    [[nodiscard]] Board& GetBoard();
    [[nodiscard]] const Board& GetBoard() const;
    [[nodiscard]] const GameConfig& GetConfig() const;
    [[nodiscard]] const std::string& GetPlayerId() const;
    [[nodiscard]] bool CanSaveRecord() const;
    [[nodiscard]] bool CanResume() const;

private:
    Board board_;
    GameConfig config_;
    std::string player_id_;
};
