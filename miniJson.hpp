#pragma once
#include <string>
#include <map>
#include <vector>
#include <variant>
#include <sstream>
#include <stdexcept>
#include <cctype>

namespace mini_json {

struct value;
using object = std::map<std::string, value>;
using array  = std::vector<value>;

struct value {
    using var_t = std::variant<std::nullptr_t, bool, double, std::string, object, array, std::uint64_t>;
    var_t data;

    value() : data(nullptr) {}
    value(const char* s) : data(std::string(s)) {}
    value(std::string s) : data(std::move(s)) {}
    value(object o) : data(std::move(o)) {}
    value(array a) : data(std::move(a)) {}
    value(double d) : data(d) {}
    value(bool b) : data(b) {}
    value(uint64_t u): data(u) {}

    bool is_null()   const { return std::holds_alternative<std::nullptr_t>(data); }
    bool is_string() const { return std::holds_alternative<std::string>(data); }
    bool is_object() const { return std::holds_alternative<object>(data); }
    bool is_array()  const { return std::holds_alternative<array>(data); }
    bool is_uint64()  const { return std::holds_alternative<std::uint64_t>(data); }

    const std::string& as_string() const { return std::get<std::string>(data); }
    const object& as_object() const { return std::get<object>(data); }
    const array& as_array() const { return std::get<array>(data); }
    const std::uint64_t& as_uint64{ return std::get<std::uint64_t>(data); }

    object& as_object() { return std::get<object>(data); }
    array& as_array() { return std::get<array>(data); }
};

// --- Simple JSON dumping ---
inline void dump_impl(const value& v, std::ostringstream& os, int indent, int level) {
    std::string pad(level * indent, ' ');
    if (v.is_string()) os << "\"" << v.as_string() << "\"";
    else if (v.is_object()) {
        os << "{\n";
        bool first = true;
        for (auto& [k, val] : v.as_object()) {
            if (!first) os << ",\n";
            os << pad << std::string(indent, ' ') << "\"" << k << "\": ";
            dump_impl(val, os, indent, level + 1);
            first = false;
        }
        os << "\n" << pad << "}";
    } else if (v.is_array()) {
        os << "[";
        bool first = true;
        for (auto& el : v.as_array()) {
            if (!first) os << ", ";
            dump_impl(el, os, indent, level);
            first = false;
        }
        os << "]";
    } else if (std::holds_alternative<bool>(v.data))
        os << (std::get<bool>(v.data) ? "true" : "false");
    else if (std::holds_alternative<double>(v.data))
        os << std::get<double>(v.data);
    else if (std::holds_alternative<std::uint64_t>(v.data))
        os << std::get<std::uint64_t>(v.data);
    else
        os << "null";
}

inline std::string dump(const object& o, int indent = 2) {
    std::ostringstream os;
    value v(o);
    dump_impl(v, os, indent, 0);
    return os.str();
}

// --- Minimal JSON parser (strings, objects, arrays only) ---
class parser {
    const std::string& s;
    size_t pos = 0;
public:
    parser(const std::string& str) : s(str) {}

    void skip_ws() { while (pos < s.size() && std::isspace((unsigned char)s[pos])) pos++; }
    bool match(char c) { skip_ws(); if (pos < s.size() && s[pos] == c) { pos++; return true; } return false; }

    std::string parse_string() {
        skip_ws();
        if (s[pos] != '"') throw std::runtime_error("expected string");
        pos++;
        std::string out;
        while (pos < s.size() && s[pos] != '"') out += s[pos++];
        pos++;
        return out;
    }

    value parse_value() {
        skip_ws();
        if (pos >= s.size()) throw std::runtime_error("unexpected end");
        if (s[pos] == '"') return value(parse_string());
        if (s[pos] == '{') return value(parse_object());
        if (s[pos] == '[') return value(parse_array());
        if (s.compare(pos, 4, "null") == 0) { pos += 4; return value(); }
        throw std::runtime_error("unexpected token");
    }

    object parse_object() {
        object o;
        match('{');
        skip_ws();
        while (!match('}')) {
            std::string key = parse_string();
            skip_ws();
            if (!match(':')) throw std::runtime_error("expected ':'");
            value val = parse_value();
            o[key] = std::move(val);
            skip_ws();
            match(',');
            skip_ws();
        }
        return o;
    }

    array parse_array() {
        array a;
        match('[');
        skip_ws();
        while (!match(']')) {
            a.push_back(parse_value());
            skip_ws();
            match(',');
            skip_ws();
        }
        return a;
    }
};

inline value parse(const std::string& text) {
    parser p(text);
    return value(p.parse_object());
}

} // namespace mini_json
