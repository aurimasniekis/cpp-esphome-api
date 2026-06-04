#include "config/xdg.hpp"

#include <cstdlib>
#include <string>

namespace cli {
namespace {

std::string env_or_empty(const char* name) {
    const char* value = std::getenv(name);
    return (value != nullptr) ? std::string(value) : std::string{};
}

}  // namespace

std::filesystem::path config_dir() {
    if (const std::string xdg = env_or_empty("XDG_CONFIG_HOME"); !xdg.empty())
        return std::filesystem::path(xdg) / "esphome-cli";

    if (const std::string home = env_or_empty("HOME"); !home.empty())
        return std::filesystem::path(home) / ".config" / "esphome-cli";

    // Last resort: a relative directory in the current working directory.
    return std::filesystem::path(".esphome-cli");
}

std::filesystem::path default_config_path() {
    return config_dir() / "config.json";
}

}  // namespace cli
