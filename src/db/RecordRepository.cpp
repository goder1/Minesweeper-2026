#include "RecordRepository.h"

RecordRepository::RecordRepository(drogon::orm::DbClientPtr database) : db_(std::move(database)) {}

void RecordRepository::FindByUserId(const std::string& user_id, RowsCb on_rows, ErrorCb on_error) {
    db_->execSqlAsync(
        "SELECT r.user_id::text, u.nickname, u.avatar, r.difficulty, r.time_seconds, "
        "       r.mistake_count, r.achieved_at::text, r.ip_address::text "
        "FROM records r "
        "JOIN users u ON u.id = r.user_id "
        "WHERE r.user_id = $1::uuid",
        [on_rows = std::move(on_rows)](const drogon::orm::Result& result) {
            std::vector<RecordRow> rows;
            rows.reserve(result.size());
            for (const auto& row : result) {
                rows.push_back({row["user_id"].as<std::string>(), row["nickname"].as<std::string>(),
                                row["avatar"].as<std::string>(),
                                row["difficulty"].as<std::string>(), row["time_seconds"].as<int>(),
                                row["mistake_count"].as<int>(),
                                row["achieved_at"].as<std::string>(),
                                row["ip_address"].isNull()                       // <— защита от NULL
                                ? std::string{}
                                : row["ip_address"].as<std::string>()});
            }
            on_rows(rows);
        },
        [on_error = std::move(on_error)](const drogon::orm::DrogonDbException& exception) {
            on_error(exception);
        },
        user_id);
}

void RecordRepository::FindTopByDifficulty(const std::string& difficulty, int limit, RowsCb on_rows,
                                           ErrorCb on_error) {
    db_->execSqlAsync(
        "SELECT r.user_id::text, u.nickname, u.avatar, r.difficulty, r.time_seconds, "
        "       r.mistake_count, r.achieved_at::textь r.ip_address::text "
        "FROM records r "
        "JOIN users u ON u.id = r.user_id "
        "WHERE r.difficulty = $1 "
        "ORDER BY r.time_seconds ASC, r.mistake_count ASC "
        "LIMIT $2::int",
        [on_rows = std::move(on_rows)](const drogon::orm::Result& result) {
            std::vector<RecordRow> rows;
            rows.reserve(result.size());
            for (const auto& row : result) {
                rows.push_back({row["user_id"].as<std::string>(), row["nickname"].as<std::string>(),
                                row["avatar"].as<std::string>(),
                                row["difficulty"].as<std::string>(), row["time_seconds"].as<int>(),
                                row["mistake_count"].as<int>(),
                                row["achieved_at"].as<std::string>(),
                                row["ip_address"].isNull()                       // <— защита от NULL
                                ? std::string{}
                                : row["ip_address"].as<std::string>()});
            }
            on_rows(rows);
        },
        [on_error = std::move(on_error)](const drogon::orm::DrogonDbException& exception) {
            on_error(exception);
        },
        difficulty, limit);
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - order matches the SQL column order and
// call sites.
void RecordRepository::CreateOrUpdate(const std::string& user_id, const std::string& difficulty,
                                      int time_seconds, int mistake_count, const std::string& ip_address, DoneCb on_done,
                                      ErrorCb on_error) {
    db_->execSqlAsync(
        "INSERT INTO records (user_id, difficulty, time_seconds, mistake_count, ip_address) "
        "VALUES ($1, $2, $3, $4, $5) "
        "ON CONFLICT (user_id, difficulty) DO UPDATE "
        "SET time_seconds  = LEAST(records.time_seconds, EXCLUDED.time_seconds), "
        "    mistake_count = CASE WHEN EXCLUDED.time_seconds < records.time_seconds "
        "                        THEN EXCLUDED.mistake_count "
        "                        ELSE records.mistake_count END, "
        "    ip_address    = CASE WHEN EXCLUDED.time_seconds < records.time_seconds "
        "                        THEN EXCLUDED.ip_address "
        "                        ELSE records.ip_address END, "
        "    achieved_at   = CASE WHEN EXCLUDED.time_seconds < records.time_seconds "
        "                        THEN NOW() "
        "                        ELSE records.achieved_at END",
        [on_done = std::move(on_done)](const drogon::orm::Result&) { on_done(); },
        [on_error = std::move(on_error)](const drogon::orm::DrogonDbException& exception) {
            on_error(exception);
        },
        user_id, difficulty, time_seconds, mistake_count, ip_address);
}
