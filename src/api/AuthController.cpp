#include "AuthController.h"

#include <sodium.h>

#include <array>

#include "JwtHelper.h"
#include "db/DbClient.h"
#include "db/UserRepository.h"

static drogon::HttpResponsePtr JsonResp(Json::Value body, drogon::HttpStatusCode code) {
    auto resp = drogon::HttpResponse::newHttpJsonResponse(std::move(body));
    resp->setStatusCode(code);
    return resp;
}

void AuthController::Register(const drogon::HttpRequestPtr& req,
                              std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    auto body = req->getJsonObject();
    if (!body || !(*body)["nickname"] || !(*body)["password"]) {
        callback(JsonResp({}, drogon::k400BadRequest));
        return;
    }

    const std::string nickname = (*body)["nickname"].asString();
    const std::string password = (*body)["password"].asString();

    if (nickname.empty() || nickname.size() > 32 || password.size() < 6) {
        Json::Value err;
        err["error"] = "invalid nickname or password";
        callback(JsonResp(err, drogon::k400BadRequest));
        return;
    }

    std::array<char, crypto_pwhash_STRBYTES> hash{};
    if (crypto_pwhash_str(hash.data(), password.c_str(), password.size(),
                          crypto_pwhash_OPSLIMIT_INTERACTIVE,
                          crypto_pwhash_MEMLIMIT_INTERACTIVE) != 0) {
        callback(JsonResp({}, drogon::k500InternalServerError));
        return;
    }

    UserRepository repo(db::Get());
    repo.Create(
        nickname, std::string(hash.data()),
        [callback, nickname](const std::string& user_id) {
            Json::Value resp;
            resp["token"] = jwt_helper::CreateToken(user_id, nickname);
            callback(JsonResp(resp, drogon::k201Created));
        },
        [callback](const drogon::orm::DrogonDbException&) {
            Json::Value err;
            err["error"] = "nickname already taken";
            callback(JsonResp(err, drogon::k409Conflict));
        });
}

void AuthController::Login(const drogon::HttpRequestPtr& req,
                           std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    auto body = req->getJsonObject();
    if (!body || !(*body)["nickname"] || !(*body)["password"]) {
        callback(JsonResp({}, drogon::k400BadRequest));
        return;
    }

    const std::string nickname = (*body)["nickname"].asString();
    const std::string password = (*body)["password"].asString();

    UserRepository repo(db::Get());
    repo.FindByNickname(
        nickname,
        [callback, password](const UserRow& user) {
            if (crypto_pwhash_str_verify(user.password_hash.c_str(), password.c_str(),
                                         password.size()) != 0) {
                Json::Value err;
                err["error"] = "invalid credentials";
                callback(JsonResp(err, drogon::k401Unauthorized));
                return;
            }
            Json::Value resp;
            resp["token"] = jwt_helper::CreateToken(user.id, user.nickname);
            callback(JsonResp(resp, drogon::k200OK));
        },
        [callback]() {
            Json::Value err;
            err["error"] = "invalid credentials";
            callback(JsonResp(err, drogon::k401Unauthorized));
        },
        [callback](const drogon::orm::DrogonDbException&) {
            callback(JsonResp({}, drogon::k500InternalServerError));
        });
}
