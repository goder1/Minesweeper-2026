#include "GameController.h"

#include <json/json.h>

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
#include "db/GameRepository.h"
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
    std::chrono::steady_clock::time_point last_active;
};

constexpr std::chrono::minutes kGuestSessionTtl{30};

class GuestStore {
public:
    static GuestStore& Instance() {
        static GuestStore instance;
        return instance;
    }

    std::string Create(GameConfig config) {
        std::string id = NewSessionId();
        const auto now = std::chrono::steady_clock::now();
        std::lock_guard<std::mutex> lock(mutex_);
        SweepExpired(now);
        games_.emplace(id, GuestGame{GameSession(config, "guest"), now, now});
        return id;
    }

    std::string Restore(GameSession session, int elapsed_secs) {
        std::string id = NewSessionId();
        const auto now = std::chrono::steady_clock::now();
        const auto started_at = now - std::chrono::seconds(elapsed_secs);
        std::lock_guard<std::mutex> lock(mutex_);
        SweepExpired(now);
        games_.emplace(id, GuestGame{std::move(session), started_at, now});
        return id;
    }

    GuestGame* Find(const std::string& id) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto game = games_.find(id);
        if (game == games_.end()) {
            return nullptr;
        }
        game->second.last_active = std::chrono::steady_clock::now();
        return &game->second;
    }

    void Remove(const std::string& id) {
        std::lock_guard<std::mutex> lock(mutex_);
        games_.erase(id);
    }

private:
    void SweepExpired(std::chrono::steady_clock::time_point now) {
        for (auto it = games_.begin(); it != games_.end();) {
            if (now - it->second.last_active > kGuestSessionTtl) {
                it = games_.erase(it);
            } else {
                ++it;
            }
        }
    }

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

std::optional<Difficulty> StringToDifficulty(const std::string& name) {
    if (name == "beginner") {
        return Difficulty::Beginner;
    }
    if (name == "intermediate") {
        return Difficulty::Intermediate;
    }
    if (name == "expert") {
        return Difficulty::Expert;
    }
    if (name == "custom") {
        return Difficulty::Custom;
    }
    return std::nullopt;
}

std::string SerializeBoardState(const Board::SavedState& state) {
    auto to_json_array = [](const std::vector<std::size_t>& indices) {
        Json::Value array(Json::arrayValue);
        for (const std::size_t index : indices) {
            array.append(static_cast<Json::UInt64>(index));
        }
        return array;
    };

    Json::Value root;
    root["mines"] = to_json_array(state.mines);
    root["revealed"] = to_json_array(state.revealed);
    root["flagged"] = to_json_array(state.flagged);
    root["mistake_count"] = state.mistake_count;

    Json::StreamWriterBuilder writer_builder;
    writer_builder["indentation"] = "";
    return Json::writeString(writer_builder, root);
}

std::optional<Board::SavedState> ParseSavedState(const std::string& json_text) {
    Json::CharReaderBuilder reader_builder;
    Json::Value root;
    std::string errors;
    std::istringstream stream(json_text);
    if (!Json::parseFromStream(reader_builder, stream, &root, &errors) || !root.isObject() ||
        !root["mines"].isArray() || !root["revealed"].isArray() || !root["flagged"].isArray()) {
        return std::nullopt;
    }

    auto to_indices = [](const Json::Value& array) {
        std::vector<std::size_t> indices;
        indices.reserve(array.size());
        for (const auto& value : array) {
            indices.push_back(value.asUInt64());
        }
        return indices;
    };

    Board::SavedState state;
    state.mines = to_indices(root["mines"]);
    state.revealed = to_indices(root["revealed"]);
    state.flagged = to_indices(root["flagged"]);
    state.mistake_count = root["mistake_count"].asInt();
    return state;
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
        constexpr std::size_t kMaxDimension = 100;

        const std::uint64_t width = body["width"].asUInt64();
        const std::uint64_t height = body["height"].asUInt64();
        const std::uint64_t mine_cnt = body["mine_cnt"].asUInt64();
        if (width == 0 || height == 0 || width > kMaxDimension || height > kMaxDimension ||
            mine_cnt == 0 || mine_cnt >= width * height) {
            return std::nullopt;
        }

        GameConfig config;
        config.width = static_cast<std::size_t>(width);
        config.height = static_cast<std::size_t>(height);
        config.mine_cnt = static_cast<std::size_t>(mine_cnt);
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

void GameController::Save(const drogon::HttpRequestPtr& req,
                          std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    auto claims = TryAuthenticate(req);
    if (!claims) {
        Json::Value err;
        err["error"] = "authentication required";
        callback(JsonResp(err, drogon::k401Unauthorized));
        return;
    }

    const std::string session_id = req->getHeader("X-Session-Id");
    if (session_id.empty()) {
        Json::Value err;
        err["error"] = "missing X-Session-Id header";
        callback(JsonResp(err, drogon::k400BadRequest));
        return;
    }

    GuestGame* game = GuestStore::Instance().Find(session_id);
    if (game == nullptr) {
        callback(JsonResp({}, drogon::k200OK));
        return;
    }

    const Board& board = game->session.GetBoard();
    if (board.Status() != GameStatus::Ongoing) {
        GuestStore::Instance().Remove(session_id);
        callback(JsonResp({}, drogon::k200OK));
        return;
    }

    int elapsed_secs;
    auto body = req->getJsonObject();
    if (body && body->isMember("elapsed_secs") && (*body)["elapsed_secs"].isInt()) {
        elapsed_secs = std::max(0, (*body)["elapsed_secs"].asInt());
    } else {
        const auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::steady_clock::now() - game->started_at);
        elapsed_secs = static_cast<int>(std::max<long long>(0, elapsed.count()));
    }

    GameRow row;
    row.user_id = claims->user_id;
    row.difficulty = DifficultyToString(game->session.GetConfig().difficulty);
    row.width = static_cast<int>(board.Width());
    row.height = static_cast<int>(board.Height());
    row.mine_cnt = static_cast<int>(game->session.GetConfig().mine_cnt);
    row.state_json = SerializeBoardState(board.Serialize());
    row.elapsed_secs = elapsed_secs;
    row.mistake_count = board.MistakeCount();

    GuestStore::Instance().Remove(session_id);

    auto repo = std::make_shared<GameRepository>(db::Get());
    repo->CreateOrUpdate(
        row, [callback, repo]() { callback(JsonResp({}, drogon::k200OK)); },
        [callback, repo](const drogon::orm::DrogonDbException&) {
            callback(JsonResp({}, drogon::k500InternalServerError));
        });
}

void GameController::Resume(const drogon::HttpRequestPtr& req,
                            std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    auto claims = TryAuthenticate(req);
    if (!claims) {
        Json::Value err;
        err["error"] = "authentication required";
        callback(JsonResp(err, drogon::k401Unauthorized));
        return;
    }

    auto repo = std::make_shared<GameRepository>(db::Get());
    repo->FindByUserId(
        claims->user_id,
        [callback, user_id = claims->user_id, repo](const GameRow& row) {
            auto difficulty = StringToDifficulty(row.difficulty);
            auto state = ParseSavedState(row.state_json);
            if (!difficulty || !state) {
                callback(JsonResp({}, drogon::k500InternalServerError));
                return;
            }

            GameConfig config;
            config.width = static_cast<std::size_t>(row.width);
            config.height = static_cast<std::size_t>(row.height);
            config.mine_cnt = static_cast<std::size_t>(row.mine_cnt);
            config.difficulty = *difficulty;

            const std::string session_id = GuestStore::Instance().Restore(
                GameSession(config, user_id, *state), row.elapsed_secs);
            GuestGame* game = GuestStore::Instance().Find(session_id);

            Json::Value resp;
            resp["session_id"] = session_id;
            resp["elapsed_secs"] = row.elapsed_secs;
            resp["board"] = BoardToJson(game->session.GetBoard());
            callback(JsonResp(resp, drogon::k200OK));
        },
        [callback]() {
            Json::Value err;
            err["error"] = "no saved game";
            callback(JsonResp(err, drogon::k404NotFound));
        },
        [callback, repo](const drogon::orm::DrogonDbException&) {
            callback(JsonResp({}, drogon::k500InternalServerError));
        });
}
