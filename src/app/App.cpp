#include "App.h"

#include <stdexcept>

App::App() : session_(PlayerSession::Guest()) {}

void App::Login(std::string user_id, std::string username) {
    session_ = PlayerSession::Authenticated(std::move(user_id), std::move(username));
}

void App::Logout() {
    active_game_.reset();
    session_ = PlayerSession::Guest();
    screen_ = AppScreen::MainMenu;
}

bool App::IsAuthenticated() const {
    return session_.IsAuthenticated();
}

void App::StartGame(GameConfig config) {
    std::string player_id = session_.GetUserId();
    active_game_.emplace(config, std::move(player_id));
    screen_ = AppScreen::Playing;
}

bool App::CanResumeGame() const {
    if (!session_.IsAuthenticated()) {
        return false;
    }
    return active_game_.has_value() && active_game_->GetBoard().Status() == GameStatus::Ongoing;
}

void App::ResumeGame() {
    if (!CanResumeGame()) {
        throw std::logic_error("No game available to resume");
    }
    screen_ = AppScreen::Playing;
}

bool App::HasActiveGame() const {
    return active_game_.has_value();
}

GameSession& App::GetActiveGame() {
    if (!active_game_.has_value()) {
        throw std::logic_error("No active game");
    }
    return *active_game_;
}

AppScreen App::GetCurrentScreen() const {
    return screen_;
}

void App::SwitchScreen(AppScreen screen) {
    screen_ = screen;
}
