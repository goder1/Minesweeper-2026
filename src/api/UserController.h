#pragma once

#include <drogon/HttpController.h>

class UserController : public drogon::HttpController<UserController> {
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(UserController::Me, "/api/users/me", drogon::Get);
    ADD_METHOD_TO(UserController::Patch, "/api/users/me", drogon::Patch);
    METHOD_LIST_END

    static void Me(const drogon::HttpRequestPtr& req,
                   std::function<void(const drogon::HttpResponsePtr&)>&& callback);

    static void Patch(const drogon::HttpRequestPtr& req,
                      std::function<void(const drogon::HttpResponsePtr&)>&& callback);
};
