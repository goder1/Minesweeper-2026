#include "GameRepository.h"

GameRepository::GameRepository(drogon::orm::DbClientPtr database) : db_(std::move(database)) {}

void GameRepository::FindByUserId(const std::string& user_id, FoundCb on_found,
                                  NotFoundCb on_not_found, ErrorCb on_error) {
    db_->execSqlAsync(
        "SELECT id::text, user_id::text, difficulty, width, height, mine_cnt, "
        "       state::text, elapsed_secs, mistake_count "
        "FROM games WHERE user_id = $1",
        [on_found = std::move(on_found),
         on_not_found = std::move(on_not_found)](const drogon::orm::Result& result) {
            if (result.empty()) {
                on_not_found();
            } else {
                on_found({result[0]["id"].as<std::string>(), result[0]["user_id"].as<std::string>(),
                          result[0]["difficulty"].as<std::string>(), result[0]["width"].as<int>(),
                          result[0]["height"].as<int>(), result[0]["mine_cnt"].as<int>(),
                          result[0]["state"].as<std::string>(), result[0]["elapsed_secs"].as<int>(),
                          result[0]["mistake_count"].as<int>()});
            }
        },
        [on_error = std::move(on_error)](const drogon::orm::DrogonDbException& exception) {
            on_error(exception);
        },
        user_id);
}

void GameRepository::CreateOrUpdate(const GameRow& row, DoneCb on_done, ErrorCb on_error) {
    db_->execSqlAsync(
        "INSERT INTO games (user_id, difficulty, width, height, mine_cnt, state, elapsed_secs, "
        "mistake_count) "
        "VALUES ($1, $2, $3, $4, $5, $6::jsonb, $7, $8) "
        "ON CONFLICT (user_id) DO UPDATE "
        "SET difficulty    = EXCLUDED.difficulty, "
        "    width         = EXCLUDED.width, "
        "    height        = EXCLUDED.height, "
        "    mine_cnt      = EXCLUDED.mine_cnt, "
        "    state         = EXCLUDED.state, "
        "    elapsed_secs  = EXCLUDED.elapsed_secs, "
        "    mistake_count = EXCLUDED.mistake_count, "
        "    last_updated  = NOW()",
        [on_done = std::move(on_done)](const drogon::orm::Result&) { on_done(); },
        [on_error = std::move(on_error)](const drogon::orm::DrogonDbException& exception) {
            on_error(exception);
        },
        row.user_id, row.difficulty, row.width, row.height, row.mine_cnt, row.state_json,
        row.elapsed_secs, row.mistake_count);
}

void GameRepository::DeleteByUserId(const std::string& user_id, DoneCb on_done, ErrorCb on_error) {
    db_->execSqlAsync(
        "DELETE FROM games WHERE user_id = $1",
        [on_done = std::move(on_done)](const drogon::orm::Result&) { on_done(); },
        [on_error = std::move(on_error)](const drogon::orm::DrogonDbException& exception) {
            on_error(exception);
        },
        user_id);
}
