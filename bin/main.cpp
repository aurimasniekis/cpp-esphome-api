/// @file
/// @brief esphome-cli entry point. Argument classification and command dispatch
///        live in app/dispatch.cpp; this just forwards and guards against any
///        escaping exception.

#include "app/dispatch.hpp"

#include <exception>
#include <iostream>

int main(const int argc, char** argv) {
    try {
        return cli::run(argc, argv);
    } catch (const std::exception& e) {
        std::cerr << "error: " << e.what() << "\n";
        return 1;
    }
}
