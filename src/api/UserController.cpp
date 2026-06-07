#include "UserController.h"

#include <set>

#include "JwtHelper.h"
#include "db/DbClient.h"
#include "db/UserRepository.h"

namespace {

drogon::HttpResponsePtr JsonResp(Json::Value body, drogon::HttpStatusCode code) {
    auto resp = drogon::HttpResponse::newHttpJsonResponse(std::move(body));
    resp->setStatusCode(code);
    return resp;
}

const std::set<std::string>& AllowedAvatars() {
    static const std::set<std::string> avatars = {
        "🙂", "🐱", "🤖", "🚀", "💣", "🎯", "🦊", "🐧", "🌟", "🔥",
    };
    return avatars;
}

std::optional<jwt_helper::Claims> Authenticate(const drogon::HttpRequestPtr& req) {
    const std::string auth = req->getHeader("Authorization");
    if (auth.substr(0, 7) != "Bearer ") {
        return std::nullopt;
    }
    return jwt_helper::VerifyToken(auth.substr(7));
}

Json::Value ProfileJson(const UserRow& user) {
    Json::Value resp;
    resp["id"] = user.id;
    resp["nickname"] = user.nickname;
    resp["avatar"] = user.avatar;
    return resp;
}

}  // namespace

void UserController::Me(const drogon::HttpRequestPtr& req,
                        std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    auto claims = Authenticate(req);
    if (!claims) {
        Json::Value err;
        err["error"] = "authentication required";
        callback(JsonResp(err, drogon::k401Unauthorized));
        return;
    }

    UserRepository repo(db::Get());
    repo.FindById(
        claims->user_id,
        [callback](const UserRow& user) { callback(JsonResp(ProfileJson(user), drogon::k200OK)); },
        [callback]() { callback(JsonResp({}, drogon::k404NotFound)); },
        [callback](const drogon::orm::DrogonDbException&) {
            callback(JsonResp({}, drogon::k500InternalServerError));
        });
}

void UserController::Patch(const drogon::HttpRequestPtr& req,
                           std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    auto claims = Authenticate(req);
    if (!claims) {
        Json::Value err;
        err["error"] = "authentication required";
        callback(JsonResp(err, drogon::k401Unauthorized));
        return;
    }

    auto body = req->getJsonObject();
    if (!body) {
        callback(JsonResp({}, drogon::k400BadRequest));
        return;
    }

    std::optional<std::string> nickname;
    if (body->isMember("nickname")) {
        if (!(*body)["nickname"].isString()) {
            callback(JsonResp({}, drogon::k400BadRequest));
            return;
        }
        std::string value = (*body)["nickname"].asString();
        if (value.empty() || value.size() > 32) {
            Json::Value err;
            err["error"] = "nickname must be 1-32 characters";
            callback(JsonResp(err, drogon::k400BadRequest));
            return;
        }
        nickname = std::move(value);
    }

    std::optional<std::string> avatar;
    if (body->isMember("avatar")) {
        if (!(*body)["avatar"].isString()) {
            callback(JsonResp({}, drogon::k400BadRequest));
            return;
        }
        std::string value = (*body)["avatar"].asString();
        if (AllowedAvatars().find(value) == AllowedAvatars().end()) {
            Json::Value err;
            err["error"] = "unknown avatar";
            callback(JsonResp(err, drogon::k400BadRequest));
            return;
        }
        avatar = std::move(value);
    }

    if (!nickname && !avatar) {
        Json::Value err;
        err["error"] = R"(expected "nickname" and/or "avatar")";
        callback(JsonResp(err, drogon::k400BadRequest));
        return;
    }

    auto repo = std::make_shared<UserRepository>(db::Get());
    const std::string user_id = claims->user_id;

    auto error500 = [callback](const drogon::orm::DrogonDbException&) {
        callback(JsonResp({}, drogon::k500InternalServerError));
    };

    auto respond_with_profile = [callback, repo, user_id, error500]() {
        repo->FindById(
            user_id,
            [callback](const UserRow& user) {
                Json::Value resp = ProfileJson(user);
                resp["token"] = jwt_helper::CreateToken(user.id, user.nickname);
                callback(JsonResp(resp, drogon::k200OK));
            },
            [callback]() { callback(JsonResp({}, drogon::k404NotFound)); }, error500);
    };

    auto apply_avatar = [repo, user_id, avatar, respond_with_profile, error500]() {
        if (avatar) {
            repo->UpdateAvatar(user_id, *avatar, respond_with_profile, error500);
        } else {
            respond_with_profile();
        }
    };

    if (nickname) {
        repo->UpdateNickname(user_id, *nickname, apply_avatar,
                             [callback](const drogon::orm::DrogonDbException&) {
                                 Json::Value err;
                                 err["error"] = "nickname already taken";
                                 callback(JsonResp(err, drogon::k409Conflict));
                             });
    } else {
        apply_avatar();
    }
}
