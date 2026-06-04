// list_entities — connect and print every discovered entity, by domain.
//
//   ./esphome_api_list_entities <host> [port] [--key <base64-psk>]

#include "common.hpp"

#include <iostream>

int main(int argc, char** argv) {
    auto cli = example::parse(
        argc, argv, "esphome_api_list_entities <host> [port]", "esphome-api-client/list_entities");
    if (!cli.ok) {
        return argc < 2 ? 1 : 0;
    }

    try {
        esphome::api::SyncClient client(cli.options);
        client.connect();  // automatically enumerates entities

        std::cout << "Connected to " << client.server_hello().name << ". Entities:\n";
        for (const auto* e : client.store().entities()) {
            std::cout << "  [" << esphome::api::entity_type_name(e->type) << "] " << e->name << " ("
                      << e->object_id << ", key=" << e->key << ")\n";
        }
        std::cout << client.store().size() << " entities.\n";

        client.disconnect();
    } catch (const std::exception& e) {
        std::cerr << "error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
