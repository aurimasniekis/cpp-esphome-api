#include <esphome/api/model/entities/climate.hpp>
#include <esphome/api/model/entities/light.hpp>
#include <esphome/api/model/entities/number.hpp>
#include <esphome/api/model/entities/switch.hpp>
#include <esphome/api/proto/message_registry.hpp>

#include <gtest/gtest.h>

#include "api.pb.h"

#include <memory>

namespace proto = esphome::api::proto;
using esphome::api::MessageRegistry;

TEST(CommandBuilder, SwitchCommand) {
    esphome::api::SwitchCommand cmd;
    cmd.key = 11;
    cmd.state = true;
    const auto msg = to_message(cmd);
    ASSERT_NE(msg, nullptr);
    const auto& req = static_cast<const proto::SwitchCommandRequest&>(*msg);
    EXPECT_EQ(req.key(), 11U);
    EXPECT_TRUE(req.state());
    // Routed to the right message id.
    EXPECT_EQ(MessageRegistry::id_of(*msg),
              static_cast<std::uint32_t>(esphome::api::MessageId::SwitchCommandRequest));
}

TEST(CommandBuilder, LightCommandOptionalsGateHasFlags) {
    esphome::api::LightCommand cmd;
    cmd.key = 3;
    cmd.state = true;
    cmd.brightness = 0.5F;
    // color/effect left unset.
    const auto msg = to_message(cmd);
    const auto& req = static_cast<const proto::LightCommandRequest&>(*msg);
    EXPECT_EQ(req.key(), 3U);
    EXPECT_TRUE(req.has_state());
    EXPECT_TRUE(req.state());
    EXPECT_TRUE(req.has_brightness());
    EXPECT_FLOAT_EQ(req.brightness(), 0.5F);
    // Unset optionals must NOT set their has_ flags.
    EXPECT_FALSE(req.has_rgb());
    EXPECT_FALSE(req.has_effect());
    EXPECT_FALSE(req.has_color_temperature());
}

TEST(CommandBuilder, LightRgbExpandsToThreeChannels) {
    esphome::api::LightCommand cmd;
    cmd.key = 4;
    cmd.rgb = esphome::api::LightRgb{0.1F, 0.2F, 0.3F};
    const auto msg = to_message(cmd);
    const auto& req = static_cast<const proto::LightCommandRequest&>(*msg);
    EXPECT_TRUE(req.has_rgb());
    EXPECT_FLOAT_EQ(req.red(), 0.1F);
    EXPECT_FLOAT_EQ(req.green(), 0.2F);
    EXPECT_FLOAT_EQ(req.blue(), 0.3F);
}

TEST(CommandBuilder, NumberCommand) {
    esphome::api::NumberCommand cmd;
    cmd.key = 9;
    cmd.state = 42.5F;
    const auto msg = to_message(cmd);
    const auto& req = static_cast<const proto::NumberCommandRequest&>(*msg);
    EXPECT_EQ(req.key(), 9U);
    EXPECT_FLOAT_EQ(req.state(), 42.5F);
}

TEST(CommandBuilder, ClimateModeEnumMapsToProto) {
    esphome::api::ClimateCommand cmd;
    cmd.key = 2;
    cmd.mode = esphome::api::ClimateMode::Heat;
    const auto msg = to_message(cmd);
    const auto& req = static_cast<const proto::ClimateCommandRequest&>(*msg);
    EXPECT_TRUE(req.has_mode());
    EXPECT_EQ(static_cast<int>(req.mode()), static_cast<int>(esphome::api::ClimateMode::Heat));
}
