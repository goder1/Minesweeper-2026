#include "LeaderboardController.h"

#include "db/DbClient.h"
#include "db/RecordRepository.h"

static drogon::HttpResponsePtr JsonResp(Json::Value body, drogon::HttpStatusCode code) {
    auto resp = drogon::HttpResponse::newHttpJsonResponse(std::move(body));
    resp->setStatusCode(code);
    return resp;
}

void LeaderboardController::Get(const drogon::HttpRequestPtr& req,
                                std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    const std::string difficulty = req->getParameter("difficulty");
    const std::string limit_str = req->getParameter("limit");

    const std::set<std::string> valid = {"beginner", "intermediate", "expert", "custom"};
    if (valid.find(difficulty) == valid.end()) {
        Json::Value err;
        err["error"] = "difficulty must be beginner, intermediate, expert, or custom";
        callback(JsonResp(err, drogon::k400BadRequest));
        return;
    }

    int limit = 20;
    if (!limit_str.empty()) {
        try {
            limit = std::stoi(limit_str);
        } catch (...) {
            limit = 20;
        }
        if (limit < 1 || limit > 100) {
            limit = 20;
        }
    }

    RecordRepository repo(db::Get());
    repo.FindTopByDifficulty(
        difficulty, limit,
        [callback](const std::vector<RecordRow>& rows) {
            Json::Value arr(Json::arrayValue);
            for (const auto& row : rows) {
                Json::Value entry;
                entry["user_id"] = row.user_id;
                entry["nickname"] = row.nickname;
                entry["avatar"] = row.avatar;
                entry["difficulty"] = row.difficulty;
                entry["time_seconds"] = row.time_seconds;
                entry["mistakes"] = row.mistake_count;
                entry["achieved_at"] = row.achieved_at;
                entry["ip"] = row.ip_address;
                arr.append(entry);
            }
            callback(JsonResp(arr, drogon::k200OK));
        },
        [callback](const drogon::orm::DrogonDbException&) {
            callback(JsonResp({}, drogon::k500InternalServerError));
        });
}
