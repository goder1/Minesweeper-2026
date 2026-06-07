#pragma once

#include <optional>
#include <string>

namespace jwt_helper {

std::string CreateToken(const std::string& user_id, const std::string& nickname);

struct Claims {
    std::string user_id;
    std::string nickname;
};

std::optional<Claims> VerifyToken(const std::string& token);

}  // namespace jwt_helper
