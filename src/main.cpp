#include <drogon/drogon.h>

#include "db/DbClient.h"

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - both are string literals at every call
// site.
static std::string Env(const char* name, const char* fallback = "") {
    const char* val = std::getenv(name);
    return (val != nullptr && *val != '\0') ? val : fallback;
}

int main() {
    const std::string conn_str = "host=" + Env("POSTGRES_HOST", "localhost") +
                                 " port=" + Env("POSTGRES_PORT", "5433") +
                                 " dbname=" + Env("POSTGRES_DB", "minesweeper") +
                                 " user=" + Env("POSTGRES_USER", "minesweeper") +
                                 " password=" + Env("POSTGRES_PASSWORD", "");

    db::Init(conn_str);

    drogon::app()
        .addListener("0.0.0.0", 8080)
        .setThreadNum(4)
        .setDocumentRoot(PROJECT_WEB_DIR)
        .setStaticFilesCacheTime(0)
        .run();

    return 0;
}
