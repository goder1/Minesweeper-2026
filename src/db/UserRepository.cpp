#include "UserRepository.h"

UserRepository::UserRepository(drogon::orm::DbClientPtr database) : db_(std::move(database)) {}

void UserRepository::FindByNickname(const std::string& nickname, FoundCb on_found,
                                    NotFoundCb on_not_found, ErrorCb on_error) {
    db_->execSqlAsync(
        "SELECT id::text, nickname, password_hash, avatar FROM users WHERE nickname = $1",
        [on_found = std::move(on_found),
         on_not_found = std::move(on_not_found)](const drogon::orm::Result& result) {
            if (result.empty()) {
                on_not_found();
            } else {
                on_found({result[0]["id"].as<std::string>(),
                          result[0]["nickname"].as<std::string>(),
                          result[0]["password_hash"].as<std::string>(),
                          result[0]["avatar"].as<std::string>()});
            }
        },
        [on_error = std::move(on_error)](const drogon::orm::DrogonDbException& exception) {
            on_error(exception);
        },
        nickname);
}

void UserRepository::FindById(const std::string& user_id, FoundCb on_found, NotFoundCb on_not_found,
                              ErrorCb on_error) {
    db_->execSqlAsync(
        "SELECT id::text, nickname, password_hash, avatar FROM users WHERE id = $1::uuid",
        [on_found = std::move(on_found),
         on_not_found = std::move(on_not_found)](const drogon::orm::Result& result) {
            if (result.empty()) {
                on_not_found();
            } else {
                on_found({result[0]["id"].as<std::string>(),
                          result[0]["nickname"].as<std::string>(),
                          result[0]["password_hash"].as<std::string>(),
                          result[0]["avatar"].as<std::string>()});
            }
        },
        [on_error = std::move(on_error)](const drogon::orm::DrogonDbException& exception) {
            on_error(exception);
        },
        user_id);
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - order matches the SQL column order and
// call sites.
void UserRepository::Create(const std::string& nickname, const std::string& password_hash,
                            CreatedCb on_created, ErrorCb on_error) {
    db_->execSqlAsync(
        "INSERT INTO users (nickname, password_hash) VALUES ($1, $2) RETURNING id::text",
        [on_created = std::move(on_created)](const drogon::orm::Result& result) {
            on_created(result[0]["id"].as<std::string>());
        },
        [on_error = std::move(on_error)](const drogon::orm::DrogonDbException& exception) {
            on_error(exception);
        },
        nickname, password_hash);
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - order matches the SQL column order and
// call sites.
void UserRepository::UpdateNickname(const std::string& user_id, const std::string& nickname,
                                    DoneCb on_done, ErrorCb on_error) {
    db_->execSqlAsync(
        "UPDATE users SET nickname = $2 WHERE id = $1::uuid",
        [on_done = std::move(on_done)](const drogon::orm::Result&) { on_done(); },
        [on_error = std::move(on_error)](const drogon::orm::DrogonDbException& exception) {
            on_error(exception);
        },
        user_id, nickname);
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - order matches the SQL column order and
// call sites.
void UserRepository::UpdateAvatar(const std::string& user_id, const std::string& avatar,
                                  DoneCb on_done, ErrorCb on_error) {
    db_->execSqlAsync(
        "UPDATE users SET avatar = $2 WHERE id = $1::uuid",
        [on_done = std::move(on_done)](const drogon::orm::Result&) { on_done(); },
        [on_error = std::move(on_error)](const drogon::orm::DrogonDbException& exception) {
            on_error(exception);
        },
        user_id, avatar);
}
