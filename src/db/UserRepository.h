#pragma once

#include <drogon/orm/DbClient.h>

#include <functional>
#include <string>

struct UserRow {
    std::string id;
    std::string nickname;
    std::string password_hash;
    std::string avatar;
};

class UserRepository {
public:
    explicit UserRepository(drogon::orm::DbClientPtr database);

    using FoundCb = std::function<void(const UserRow&)>;
    using NotFoundCb = std::function<void()>;
    using CreatedCb = std::function<void(const std::string& new_id)>;
    using DoneCb = std::function<void()>;
    using ErrorCb = std::function<void(const drogon::orm::DrogonDbException&)>;

    void FindByNickname(const std::string& nickname, FoundCb on_found, NotFoundCb on_not_found,
                        ErrorCb on_error);
    void FindById(const std::string& user_id, FoundCb on_found, NotFoundCb on_not_found,
                  ErrorCb on_error);
    void Create(const std::string& nickname, const std::string& password_hash, CreatedCb on_created,
                ErrorCb on_error);

    void UpdateNickname(const std::string& user_id, const std::string& nickname, DoneCb on_done,
                        ErrorCb on_error);
    void UpdateAvatar(const std::string& user_id, const std::string& avatar, DoneCb on_done,
                      ErrorCb on_error);

private:
    drogon::orm::DbClientPtr db_;
};
