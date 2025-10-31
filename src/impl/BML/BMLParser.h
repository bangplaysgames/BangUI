#pragma once
#include <string>
#include <map>
#include <vector>
#include <unordered_map>
#include <memory>
#include <iostream>
#include <fstream>
#include "../../api/UIElement.h"

// Very small BML parser implementation used during the overhaul.
// Supports minimal BML syntax described in BMLSyntax.md:
// - Objects start with '#' followed by an identifier
// - Property lines start with '>' optionally followed by whitespace, then key: value
// This parser is intentionally conservative and only provides the basic
// stylesheet mapping needed by the rest of the code during migration.
namespace BangUI {
namespace impl {

struct StyleSheet {
    // map<styleName, map<propName, propValue>>
    std::unordered_map<std::string, std::unordered_map<std::string, std::string>> styles;

    const std::unordered_map<std::string, std::string>* getStyle(const std::string& name) const {
        auto it = styles.find(name);
        if (it == styles.end()) return nullptr;
        return &it->second;
    }

    // Convenience lookup for Global_<Type> style keys
    const std::unordered_map<std::string, std::string>* getGlobalStyleForType(const std::string& type) const {
        auto key = std::string("Global_") + type;
        auto it = styles.find(key);
        if (it == styles.end()) return nullptr;
        return &it->second;
    }
};

class BMLParser {
public:
    BMLParser() = default;

    bool parseFromString(const std::string& text) {
        styles = std::make_shared<StyleSheet>();
        std::string curName;
        size_t pos = 0;
        while (pos < text.size()) {
            // read a line
            size_t eol = text.find('\n', pos);
            std::string line;
            if (eol == std::string::npos) {
                line = text.substr(pos);
                pos = text.size();
            } else {
                line = text.substr(pos, eol - pos);
                pos = eol + 1;
            }

            // trim
            auto l = line.find_first_not_of(" \t\r");
            if (l == std::string::npos) continue;
            auto r = line.find_last_not_of(" \t\r");
            line = line.substr(l, r - l + 1);
            if (line.empty()) continue;

            if (line[0] == '#') {
                // object name
                curName = line.substr(1);
                // trim again
                auto s = curName.find_first_not_of(" \t");
                auto e = curName.find_last_not_of(" \t");
                if (s == std::string::npos) curName.clear();
                else curName = curName.substr(s, e - s + 1);
                if (!curName.empty()) styles->styles[curName] = {};
            } else {
                // property lines optionally start with '>'
                std::string propLine = line;
                if (!propLine.empty() && propLine[0] == '>') {
                    // find first non-space after the initial '>' character
                    size_t p = propLine.find_first_not_of(" \t", 1);
                    if (p != std::string::npos) propLine = propLine.substr(p);
                    else propLine.clear();
                }
                if (propLine.empty() || curName.empty()) continue;
                // split key: value
                auto colon = propLine.find(':');
                if (colon == std::string::npos) continue;
                std::string key = propLine.substr(0, colon);
                std::string val = propLine.substr(colon + 1);
                // trim
                auto ks = key.find_first_not_of(" \t");
                auto ke = key.find_last_not_of(" \t");
                if (ks == std::string::npos) continue;
                key = key.substr(ks, ke - ks + 1);
                auto vs = val.find_first_not_of(" \t");
                if (vs == std::string::npos) val.clear();
                else {
                    auto ve = val.find_last_not_of(" \t");
                    val = val.substr(vs, ve - vs + 1);
                }

                styles->styles[curName][key] = val;
            }
        }
        return true;
    }

    bool loadFromFile(const std::string& path) {
        std::ifstream ifs(path);
        if (!ifs.is_open()) return false;
        std::string contents((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
        return parseFromString(contents);
    }

    std::shared_ptr<StyleSheet> getStyleSheet() const { return styles; }

    // Global singleton access: library-wide parser used when loading BML stylesheet
    static BMLParser& getGlobalParser() {
        static BMLParser globalParser;
        return globalParser;
    }

    // Return the Global_<Type> mapping if present (nullptr otherwise)
    const std::unordered_map<std::string, std::string>* getGlobalStyleForType(const std::string& type) const {
        if (!styles) return nullptr;
        return styles->getGlobalStyleForType(type);
    }

    // Apply only-missing properties from a Global_<Type> style to element->properties
    void applyGlobalStyleToElement(const std::string& type, BangUI::API::UIElement* element) const {
        if (!element) return;
        auto gs = getGlobalStyleForType(type);
        if (!gs) return;
        for (const auto& kv : *gs) {
            // only set property if element hasn't explicitly provided it
            if (element->properties.find(kv.first) == element->properties.end()) {
                element->properties[kv.first] = kv.second;
            }
        }
    }

    // Apply a named style to a UIElement by copying known properties into element->properties map
    void applyStyleToElement(const std::string& styleName, BangUI::API::UIElement* element) const {
        if (!element) return;
        if (!styles) return;
        auto it = styles->styles.find(styleName);
        if (it == styles->styles.end()) return;
        for (const auto& kv : it->second) {
            element->properties[kv.first] = kv.second;
        }
    }

private:
    std::shared_ptr<StyleSheet> styles;
};

} // namespace impl
} // namespace BangUI
