#pragma once

#include <drogon/orm/DbClient.h>

#include <string>

namespace db {

void Init(const std::string& conn_str, std::size_t pool_size = 5);
drogon::orm::DbClientPtr Get();

}
