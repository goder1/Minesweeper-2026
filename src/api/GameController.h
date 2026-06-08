#pragma once

#include <drogon/HttpController.h>

class GameController : public drogon::HttpController<GameController> {
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(GameController::Start, "/api/game/start", drogon::Post);
    ADD_METHOD_TO(GameController::Move, "/api/game/move", drogon::Post);
    ADD_METHOD_TO(GameController::Flag, "/api/game/flag", drogon::Post);
    ADD_METHOD_TO(GameController::Revert, "/api/game/revert", drogon::Post);
    ADD_METHOD_TO(GameController::State, "/api/game/state", drogon::Get);
    ADD_METHOD_TO(GameController::Save, "/api/game/save", drogon::Post);
    ADD_METHOD_TO(GameController::Resume, "/api/game/resume", drogon::Post);
    METHOD_LIST_END

    static void Start(const drogon::HttpRequestPtr& req,
                      std::function<void(const drogon::HttpResponsePtr&)>&& callback);

    static void Move(const drogon::HttpRequestPtr& req,
                     std::function<void(const drogon::HttpResponsePtr&)>&& callback);

    static void Flag(const drogon::HttpRequestPtr& req,
                     std::function<void(const drogon::HttpResponsePtr&)>&& callback);

    static void Revert(const drogon::HttpRequestPtr& req,
                       std::function<void(const drogon::HttpResponsePtr&)>&& callback);

    static void State(const drogon::HttpRequestPtr& req,
                      std::function<void(const drogon::HttpResponsePtr&)>&& callback);

    static void Save(const drogon::HttpRequestPtr& req,
                     std::function<void(const drogon::HttpResponsePtr&)>&& callback);

    static void Resume(const drogon::HttpRequestPtr& req,
                       std::function<void(const drogon::HttpResponsePtr&)>&& callback);
};
