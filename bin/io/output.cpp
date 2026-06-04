#include "io/output.hpp"

#include "io/yaml_emit.hpp"

#include <iostream>

namespace cli {

Format parse_format(const std::string& name) {
    if (name == "json")
        return Format::Json;
    if (name == "yaml" || name == "yml")
        return Format::Yaml;
    return Format::Text;
}

void OutputWriter::emit(const nlohmann::json& doc,
                        const std::function<void(std::ostream&)>& render_text) const {
    switch (format_) {
    case Format::Json:
        std::cout << doc.dump(2) << "\n";
        break;
    case Format::Yaml:
        std::cout << to_yaml(doc) << "\n";
        break;
    case Format::Text:
        render_text(std::cout);
        break;
    }
}

void OutputWriter::stream_line(const std::string& text, const nlohmann::json& obj) const {
    switch (format_) {
    case Format::Json:
        std::cout << obj.dump() << "\n";
        break;
    case Format::Yaml:
        std::cout << to_yaml_list_item(obj) << "\n";
        break;
    case Format::Text:
        std::cout << text << "\n";
        break;
    }
    std::cout.flush();
}

}  // namespace cli
