#pragma once

#include <string>

class PlayerSession {
public:
    static PlayerSession Guest();
    static PlayerSession Authenticated(std::string user_id, std::string username);

    [[nodiscard]] bool IsGuest() const;
    [[nodiscard]] bool IsAuthenticated() const;
    [[nodiscard]] const std::string& GetUserId() const;
    [[nodiscard]] const std::string& GetUsername() const;

private:
    PlayerSession() = default;

    std::string user_id_;
    std::string username_;
};
