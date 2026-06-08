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
    resp["theme"] = user.theme;
    resp["controls"] = user.controls_mode;
    return resp;
}

const std::set<std::string>& AllowedThemes() {
    static const std::set<std::string> themes = {"light", "dark"};
    return themes;
}

const std::set<std::string>& AllowedControlsModes() {
    static const std::set<std::string> modes = {"standard", "swapped"};
    return modes;
}

}

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

    std::optional<std::string> theme;
    if (body->isMember("theme")) {
        if (!(*body)["theme"].isString()) {
            callback(JsonResp({}, drogon::k400BadRequest));
            return;
        }
        std::string value = (*body)["theme"].asString();
        if (AllowedThemes().find(value) == AllowedThemes().end()) {
            Json::Value err;
            err["error"] = "unknown theme";
            callback(JsonResp(err, drogon::k400BadRequest));
            return;
        }
        theme = std::move(value);
    }

    std::optional<std::string> controls;
    if (body->isMember("controls")) {
        if (!(*body)["controls"].isString()) {
            callback(JsonResp({}, drogon::k400BadRequest));
            return;
        }
        std::string value = (*body)["controls"].asString();
        if (AllowedControlsModes().find(value) == AllowedControlsModes().end()) {
            Json::Value err;
            err["error"] = "unknown controls mode";
            callback(JsonResp(err, drogon::k400BadRequest));
            return;
        }
        controls = std::move(value);
    }

    if (!nickname && !avatar && !theme && !controls) {
        Json::Value err;
        err["error"] = R"(expected "nickname", "avatar", "theme" and/or "controls")";
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

    auto apply_controls = [repo, user_id, controls, respond_with_profile, error500]() {
        if (controls) {
            repo->UpdateControlsMode(user_id, *controls, respond_with_profile, error500);
        } else {
            respond_with_profile();
        }
    };

    auto apply_theme = [repo, user_id, theme, apply_controls, error500]() {
        if (theme) {
            repo->UpdateTheme(user_id, *theme, apply_controls, error500);
        } else {
            apply_controls();
        }
    };

    auto apply_avatar = [repo, user_id, avatar, apply_theme, error500]() {
        if (avatar) {
            repo->UpdateAvatar(user_id, *avatar, apply_theme, error500);
        } else {
            apply_theme();
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
