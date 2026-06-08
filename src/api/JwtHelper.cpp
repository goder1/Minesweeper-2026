#include "JwtHelper.h"

#include <jwt-cpp/jwt.h>

#include <chrono>
#include <cstdlib>
#include <stdexcept>

namespace jwt_helper {

static std::string GetSecret() {
    const char* secret = std::getenv("JWT_SECRET");
    if (secret == nullptr || std::string(secret).empty()) {
        throw std::runtime_error("JWT_SECRET environment variable is not set");
    }
    return secret;
}

std::string CreateToken(const std::string& user_id, const std::string& nickname) {
    return jwt::create()
        .set_issuer("minesweeper-2026")
        .set_type("JWT")
        .set_payload_claim("user_id", jwt::claim(user_id))
        .set_payload_claim("nickname", jwt::claim(nickname))
        .set_issued_at(std::chrono::system_clock::now())
        .set_expires_at(std::chrono::system_clock::now() + std::chrono::hours(24))
        .sign(jwt::algorithm::hs256{GetSecret()});
}

std::optional<Claims> VerifyToken(const std::string& token) {
    try {
        auto verifier = jwt::verify()
                            .allow_algorithm(jwt::algorithm::hs256{GetSecret()})
                            .with_issuer("minesweeper-2026");

        auto decoded = jwt::decode(token);
        verifier.verify(decoded);

        return Claims{decoded.get_payload_claim("user_id").as_string(),
                      decoded.get_payload_claim("nickname").as_string()};
    } catch (...) {
        return std::nullopt;
    }
}

}
