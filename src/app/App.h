#pragma once

#include <optional>

#include "game/GameSession.h"
#include "session/PlayerSession.h"

enum class AppScreen {
    MainMenu,
    Playing,
    Leaderboard,
    Settings,
    Login,
    Register,
};

class App {
public:
    App();

    void Login(std::string user_id, std::string username);
    void Logout();
    [[nodiscard]] bool IsAuthenticated() const;

    void StartGame(GameConfig config);
    [[nodiscard]] bool CanResumeGame() const;
    void ResumeGame();
    [[nodiscard]] bool HasActiveGame() const;
    [[nodiscard]] GameSession& GetActiveGame();

    [[nodiscard]] AppScreen GetCurrentScreen() const;
    void SwitchScreen(AppScreen screen);

private:
    AppScreen screen_ = AppScreen::MainMenu;
    PlayerSession session_;
    std::optional<GameSession> active_game_;
};
