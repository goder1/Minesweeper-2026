#include "DbClient.h"

#include <stdexcept>

namespace db {

static drogon::orm::DbClientPtr g_client;

void Init(const std::string& conn_str, std::size_t pool_size) {
    g_client = drogon::orm::DbClient::newPgClient(conn_str, pool_size);
}

drogon::orm::DbClientPtr Get() {
    if (!g_client) {
        throw std::runtime_error("db::Init() must be called before db::Get()");
    }
    return g_client;
}

}  // namespace db
