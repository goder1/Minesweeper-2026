#pragma once

#include "Board.h"
#include "Difficulty.h"

#include <string>

class GameSession {
public:
    GameSession(GameConfig config, std::string player_id);

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
