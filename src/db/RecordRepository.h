#pragma once

#include <drogon/orm/DbClient.h>

#include <functional>
#include <string>
#include <vector>

struct RecordRow {
    std::string user_id;
    std::string nickname;
    std::string avatar;
    std::string difficulty;
    int time_seconds;
    int mistake_count;
    std::string achieved_at;
    std::string ip_address;
};

class RecordRepository {
public:
    explicit RecordRepository(drogon::orm::DbClientPtr database);

    using RowsCb = std::function<void(const std::vector<RecordRow>&)>;
    using DoneCb = std::function<void()>;
    using ErrorCb = std::function<void(const drogon::orm::DrogonDbException&)>;

    void FindByUserId(const std::string& user_id, RowsCb on_rows, ErrorCb on_error);

    void FindTopByDifficulty(const std::string& difficulty, int limit, RowsCb on_rows,
                             ErrorCb on_error);

    void CreateOrUpdate(const std::string& user_id, const std::string& difficulty, int time_seconds,
                        int mistake_count, const std::string& ip_address, DoneCb on_done, ErrorCb on_error);

private:
    drogon::orm::DbClientPtr db_;
};
