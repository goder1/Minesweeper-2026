#pragma once

#include <drogon/orm/DbClient.h>

#include <functional>
#include <optional>
#include <string>

struct GameRow {
    std::string id;
    std::string user_id;
    std::string difficulty;
    int width;
    int height;
    int mine_cnt;
    std::string state_json;  // JSONB
    int elapsed_secs;
    int mistake_count;
};

class GameRepository {
public:
    explicit GameRepository(drogon::orm::DbClientPtr database);

    using FoundCb = std::function<void(const GameRow&)>;
    using NotFoundCb = std::function<void()>;
    using DoneCb = std::function<void()>;
    using ErrorCb = std::function<void(const drogon::orm::DrogonDbException&)>;

    void FindByUserId(const std::string& user_id, FoundCb on_found, NotFoundCb on_not_found,
                      ErrorCb on_error);

    void CreateOrUpdate(const GameRow& row, DoneCb on_done, ErrorCb on_error);

    void DeleteByUserId(const std::string& user_id, DoneCb on_done, ErrorCb on_error);

private:
    drogon::orm::DbClientPtr db_;
};
