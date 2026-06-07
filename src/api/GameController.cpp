#include "GameController.h"

#include <algorithm>
#include <chrono>
#include <iomanip>
#include <mutex>
#include <optional>
#include <random>
#include <sstream>
#include <unordered_map>

#include "JwtHelper.h"
#include "db/DbClient.h"
#include "db/RecordRepository.h"
#include "game/Difficulty.h"
#include "game/GameSession.h"

namespace {

drogon::HttpResponsePtr JsonResp(Json::Value body, drogon::HttpStatusCode code) {
    auto resp = drogon::HttpResponse::newHttpJsonResponse(std::move(body));
    resp->setStatusCode(code);
    return resp;
}

std::string NewSessionId() {
    static std::mt19937_64 rng(std::random_device{}());
    std::uniform_int_distribution<std::uint64_t> dist;
    std::ostringstream oss;
    oss << std::hex << std::setfill('0') << std::setw(16) << dist(rng) << std::setw(16)
        << dist(rng);
    return oss.str();
}

struct GuestGame {
    GameSession session;
    std::chrono::steady_clock::time_point started_at;
};

class GuestStore {
public:
    static GuestStore& Instance() {
        static GuestStore instance;
        return instance;
    }

    std::string Create(GameConfig config) {
        std::string id = NewSessionId();
        std::lock_guard<std::mutex> lock(mutex_);
        games_.emplace(id,
                       GuestGame{GameSession(config, "guest"), std::chrono::steady_clock::now()});
        return id;
    }

    GuestGame* Find(const std::string& id) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto game = games_.find(id);
        return game == games_.end() ? nullptr : &game->second;
    }

private:
    std::mutex mutex_;
    std::unordered_map<std::string, GuestGame> games_;
};

std::string DifficultyToString(Difficulty difficulty) {
    switch (difficulty) {
        case Difficulty::Beginner:
            return "beginner";
        case Difficulty::Intermediate:
            return "intermediate";
        case Difficulty::Expert:
            return "expert";
        case Difficulty::Custom:
            return "custom";
    }
    return "custom";
}

std::optional<jwt_helper::Claims> TryAuthenticate(const drogon::HttpRequestPtr& req) {
    const std::string auth = req->getHeader("Authorization");
    if (auth.substr(0, 7) != "Bearer ") {
        return std::nullopt;
    }
    return jwt_helper::VerifyToken(auth.substr(7));
}

void MaybeSaveRecord(const drogon::HttpRequestPtr& req, const GuestGame& game) {
    if (game.session.GetBoard().Status() != GameStatus::Won) {
        return;
    }
    auto claims = TryAuthenticate(req);
    if (!claims) {
        return;
    }

    const auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::steady_clock::now() - game.started_at);
    const int time_seconds = static_cast<int>(std::max<long long>(1, elapsed.count()));

    RecordRepository repo(db::Get());
    repo.CreateOrUpdate(
        claims->user_id, DifficultyToString(game.session.GetConfig().difficulty), time_seconds,
        game.session.GetBoard().MistakeCount(), []() {},
        [](const drogon::orm::DrogonDbException&) {});
}

std::string CellStateToString(CellState state) {
    switch (state) {
        case CellState::Hidden:
            return "hidden";
        case CellState::Revealed:
            return "revealed";
        case CellState::Flagged:
            return "flagged";
        case CellState::HitMine:
            return "mine";
    }
    return "hidden";
}

Json::Value BoardToJson(const Board& board) {
    const bool reveal_mines = board.Status() == GameStatus::Won;

    Json::Value cells(Json::arrayValue);
    for (std::size_t y_coord = 0; y_coord < board.Height(); ++y_coord) {
        Json::Value row(Json::arrayValue);
        for (std::size_t x_coord = 0; x_coord < board.Width(); ++x_coord) {
            const Cell& cell = board.GetCellInfo(x_coord, y_coord);
            Json::Value json_cell;
            json_cell["state"] = CellStateToString(cell.state);
            if (cell.state == CellState::Revealed) {
                json_cell["adjacent_mines"] = cell.adjacent_mines;
            }
            if (cell.state == CellState::HitMine || (reveal_mines && cell.is_mine)) {
                json_cell["is_mine"] = true;
            }
            row.append(std::move(json_cell));
        }
        cells.append(std::move(row));
    }

    Json::Value resp;
    resp["width"] = static_cast<Json::UInt64>(board.Width());
    resp["height"] = static_cast<Json::UInt64>(board.Height());
    resp["status"] = board.Status() == GameStatus::Won ? "won" : "ongoing";
    resp["pending_revert"] = board.HasPendingRevert();
    resp["mistake_count"] = board.MistakeCount();
    resp["cells"] = std::move(cells);
    return resp;
}

std::optional<GameConfig> ParseGameConfig(const Json::Value& body) {
    if (body.isMember("difficulty") && body["difficulty"].isString()) {
        const std::string name = body["difficulty"].asString();
        if (name == "beginner") {
            return PresetConfig(Difficulty::Beginner);
        }
        if (name == "intermediate") {
            return PresetConfig(Difficulty::Intermediate);
        }
        if (name == "expert") {
            return PresetConfig(Difficulty::Expert);
        }
        return std::nullopt;
    }
    if (body.isMember("width") && body.isMember("height") && body.isMember("mine_cnt")) {
        GameConfig config;
        config.width = body["width"].asUInt64();
        config.height = body["height"].asUInt64();
        config.mine_cnt = body["mine_cnt"].asUInt64();
        config.difficulty = Difficulty::Custom;
        return config;
    }
    return std::nullopt;
}

GuestGame* FindGuestGame(const drogon::HttpRequestPtr& req,
                         const std::function<void(const drogon::HttpResponsePtr&)>& callback) {
    const std::string session_id = req->getHeader("X-Session-Id");
    if (session_id.empty()) {
        Json::Value err;
        err["error"] = "missing X-Session-Id header";
        callback(JsonResp(err, drogon::k400BadRequest));
        return nullptr;
    }
    GuestGame* game = GuestStore::Instance().Find(session_id);
    if (game == nullptr) {
        Json::Value err;
        err["error"] = "unknown game session";
        callback(JsonResp(err, drogon::k404NotFound));
        return nullptr;
    }
    return game;
}

}  // namespace

void GameController::Start(const drogon::HttpRequestPtr& req,
                           std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    auto body = req->getJsonObject();
    if (!body) {
        callback(JsonResp({}, drogon::k400BadRequest));
        return;
    }

    auto config = ParseGameConfig(*body);
    if (!config) {
        Json::Value err;
        err["error"] =
            "expected {\"difficulty\": \"beginner\"|\"intermediate\"|\"expert\"} "
            "or {\"width\": N, \"height\": N, \"mine_cnt\": N}";
        callback(JsonResp(err, drogon::k400BadRequest));
        return;
    }

    std::string session_id;
    try {
        session_id = GuestStore::Instance().Create(*config);
    } catch (const std::invalid_argument& e) {
        Json::Value err;
        err["error"] = e.what();
        callback(JsonResp(err, drogon::k400BadRequest));
        return;
    }

    GuestGame* game = GuestStore::Instance().Find(session_id);
    Json::Value resp;
    resp["session_id"] = session_id;
    resp["board"] = BoardToJson(game->session.GetBoard());
    callback(JsonResp(resp, drogon::k200OK));
}

void GameController::Move(const drogon::HttpRequestPtr& req,
                          std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    GuestGame* game = FindGuestGame(req, callback);
    if (game == nullptr) {
        return;
    }

    auto body = req->getJsonObject();
    if (!body || !body->isMember("x") || !body->isMember("y")) {
        callback(JsonResp({}, drogon::k400BadRequest));
        return;
    }

    game->session.GetBoard().RevealCell((*body)["x"].asUInt64(), (*body)["y"].asUInt64());
    MaybeSaveRecord(req, *game);
    callback(JsonResp(BoardToJson(game->session.GetBoard()), drogon::k200OK));
}

void GameController::Flag(const drogon::HttpRequestPtr& req,
                          std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    GuestGame* game = FindGuestGame(req, callback);
    if (game == nullptr) {
        return;
    }

    auto body = req->getJsonObject();
    if (!body || !body->isMember("x") || !body->isMember("y")) {
        callback(JsonResp({}, drogon::k400BadRequest));
        return;
    }

    game->session.GetBoard().ToggleFlag((*body)["x"].asUInt64(), (*body)["y"].asUInt64());
    callback(JsonResp(BoardToJson(game->session.GetBoard()), drogon::k200OK));
}

void GameController::Revert(const drogon::HttpRequestPtr& req,
                            std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    GuestGame* game = FindGuestGame(req, callback);
    if (game == nullptr) {
        return;
    }

    game->session.GetBoard().RevertMove();
    callback(JsonResp(BoardToJson(game->session.GetBoard()), drogon::k200OK));
}

void GameController::State(const drogon::HttpRequestPtr& req,
                           std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    GuestGame* game = FindGuestGame(req, callback);
    if (game == nullptr) {
        return;
    }

    callback(JsonResp(BoardToJson(game->session.GetBoard()), drogon::k200OK));
}
