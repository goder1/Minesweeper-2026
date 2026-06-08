#include "GameSession.h"

GameSession::GameSession(GameConfig config, std::string player_id)
    : board_(config.width, config.height, config.mine_cnt),
      config_(config),
      player_id_(std::move(player_id)) {}

GameSession::GameSession(GameConfig config, std::string player_id,
                         const Board::SavedState& saved_state)
    : board_(config.width, config.height, config.mine_cnt, saved_state),
      config_(config),
      player_id_(std::move(player_id)) {}

Board& GameSession::GetBoard() {
    return board_;
}

const Board& GameSession::GetBoard() const {
    return board_;
}

const GameConfig& GameSession::GetConfig() const {
    return config_;
}

const std::string& GameSession::GetPlayerId() const {
    return player_id_;
}

bool GameSession::CanSaveRecord() const {
    return !player_id_.empty();
}

bool GameSession::CanResume() const {
    return !player_id_.empty();
}
