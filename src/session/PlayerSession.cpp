#include "PlayerSession.h"

PlayerSession PlayerSession::Guest() {
    return PlayerSession{};
}

PlayerSession PlayerSession::Authenticated(std::string user_id, std::string username) {
    PlayerSession session;
    session.user_id_ = std::move(user_id);
    session.username_ = std::move(username);
    return session;
}

bool PlayerSession::IsGuest() const {
    return user_id_.empty();
}

bool PlayerSession::IsAuthenticated() const {
    return !user_id_.empty();
}

const std::string& PlayerSession::GetUserId() const {
    return user_id_;
}

const std::string& PlayerSession::GetUsername() const {
    return username_;
}
