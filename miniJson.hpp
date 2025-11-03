#pragma once
#include <cctype>
#include <cstdint>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <variant>
#include <vector>

namespace mini_json {

// Forward declarations
struct value;
using object = std::map<std::string, value>;
using array  = std::vector<value>;

struct value
{
    using variant_t = std::variant<std::nullptr_t, bool, uint64_t, double, std::string, object, array>;

    variant_t data;

    value() : data(nullptr) {}
    value(std::nullptr_t) : data(nullptr) {}
    value(bool b) : data(b) {}
    value(uint64_t n) : data(n) {}
    value(double n) : data(n) {}
    value(const char* s) : data(std::string(s)) {}
    value(std::string s) : data(std::move(s)) {}
    value(object o) : data(std::move(o)) {}
    value(array a) : data(std::move(a)) {}

    bool is_null() const { return std::holds_alternative<std::nullptr_t>(data); }
    bool is_bool() const { return std::holds_alternative<bool>(data); }
    bool is_uint64() const { return std::holds_alternative<uint64_t>(data); }
    bool is_double() const { return std::holds_alternative<double>(data); }
    bool is_string() const { return std::holds_alternative<std::string>(data); }
    bool is_object() const { return std::holds_alternative<object>(data); }
    bool is_array() const { return std::holds_alternative<array>(data); }

    bool as_bool() const { return std::get<bool>(data); }
    uint64_t as_uint64() const { return std::get<uint64_t>(data); }
    double as_double() const { return std::get<double>(data); }
    const std::string& as_string() const { return std::get<std::string>(data); }
    const object& as_object() const { return std::get<object>(data); }
    const array& as_array() const { return std::get<array>(data); }

    object& as_object() { return std::get<object>(data); }
    array& as_array() { return std::get<array>(data); }
};

// -----------------------------------------------------------------------------
// Dump (serialize to string)
// -----------------------------------------------------------------------------
inline void dump_value(std::ostringstream& oss, const value& v, int indent = 0, int depth = 0);

inline std::string escape(const std::string& s)
{
    std::string out;
    for (char c : s)
    {
        switch (c)
        {
            case '\"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default: out += c; break;
        }
    }
    return out;
}

inline void dump_object(std::ostringstream& oss, const object& obj, int indent, int depth)
{
    oss << "{";
    bool first = true;
    for (auto& [k, v] : obj)
    {
        if (!first)
            oss << ",";
        if (indent)
            oss << "\n" << std::string((depth + 1) * indent, ' ');
        oss << "\"" << escape(k) << "\": ";
        dump_value(oss, v, indent, depth + 1);
        first = false;
    }
    if (indent && !obj.empty())
        oss << "\n" << std::string(depth * indent, ' ');
    oss << "}";
}

inline void dump_array(std::ostringstream& oss, const array& arr, int indent, int depth)
{
    oss << "[";
    bool first = true;
    for (auto& v : arr)
    {
        if (!first)
            oss << ",";
        if (indent)
            oss << "\n" << std::string((depth + 1) * indent, ' ');
        dump_value(oss, v, indent, depth + 1);
        first = false;
    }
    if (indent && !arr.empty())
        oss << "\n" << std::string(depth * indent, ' ');
    oss << "]";
}

inline void dump_value(std::ostringstream& oss, const value& v, int indent, int depth)
{
    if (v.is_null())
        oss << "null";
    else if (v.is_bool())
        oss << (v.as_bool() ? "true" : "false");
    else if (v.is_uint64())
        oss << v.as_uint64();
    else if (v.is_double())
        oss << v.as_double();
    else if (v.is_string())
        oss << "\"" << escape(v.as_string()) << "\"";
    else if (v.is_object())
        dump_object(oss, v.as_object(), indent, depth);
    else if (v.is_array())
        dump_array(oss, v.as_array(), indent, depth);
}

inline std::string dump(const value& v, int indent = 0)
{
    std::ostringstream oss;
    dump_value(oss, v, indent, 0);
    return oss.str();
}

// -----------------------------------------------------------------------------
// Minimal parser (enough for header reconstruction)
// -----------------------------------------------------------------------------
struct parser
{
    const std::string& s;
    size_t pos = 0;

    parser(const std::string& str) : s(str) {}

    void skip_ws()
    {
        while (pos < s.size() && std::isspace((unsigned char)s[pos]))
            ++pos;
    }

    bool match(char c)
    {
        skip_ws();
        if (pos < s.size() && s[pos] == c)
        {
            ++pos;
            return true;
        }
        return false;
    }

    std::string parse_string()
    {
        if (!match('\"'))
            throw std::runtime_error("expected string");
        std::string out;
        while (pos < s.size())
        {
            char c = s[pos++];
            if (c == '\\')
            {
                if (pos >= s.size())
                    break;
                char esc = s[pos++];
                switch (esc)
                {
                    case 'n': out += '\n'; break;
                    case 'r': out += '\r'; break;
                    case 't': out += '\t'; break;
                    case '\"': out += '\"'; break;
                    case '\\': out += '\\'; break;
                    default: out += esc; break;
                }
            }
            else if (c == '\"')
                break;
            else
                out += c;
        }
        return out;
    }

    uint64_t parse_uint64()
    {
        skip_ws();
        uint64_t val = 0;
        if (pos >= s.size() || !std::isdigit((unsigned char)s[pos]))
            throw std::runtime_error("expected number");
        while (pos < s.size() && std::isdigit((unsigned char)s[pos]))
        {
            val = val * 10 + (s[pos++] - '0');
        }
        return val;
    }

    value parse_value()
    {
        skip_ws();
        if (pos >= s.size())
            throw std::runtime_error("unexpected end");

        char c = s[pos];
        if (c == '\"')
            return parse_string();
        if (c == '{')
            return parse_object();
        if (c == '[')
            return parse_array();
        if (std::isdigit((unsigned char)c))
            return parse_uint64();
        if (s.compare(pos, 4, "true") == 0)
        {
            pos += 4;
            return true;
        }
        if (s.compare(pos, 5, "false") == 0)
        {
            pos += 5;
            return false;
        }
        if (s.compare(pos, 4, "null") == 0)
        {
            pos += 4;
            return nullptr;
        }

        throw std::runtime_error("unexpected token");
    }

    object parse_object()
    {
        object obj;
        match('{');
        skip_ws();
        bool first = true;
        while (pos < s.size() && !match('}'))
        {
            if (!first)
            {
                if (!match(','))
                    throw std::runtime_error("expected ',' in object");
            }
            std::string key = parse_string();
            if (!match(':'))
                throw std::runtime_error("expected ':'");
            value val = parse_value();
            obj.emplace(std::move(key), std::move(val));
            first = false;
            skip_ws();
        }
        return obj;
    }

    array parse_array()
    {
        array arr;
        match('[');
        skip_ws();
        bool first = true;
        while (pos < s.size() && !match(']'))
        {
            if (!first)
            {
                if (!match(','))
                    throw std::runtime_error("expected ',' in array");
            }
            arr.push_back(parse_value());
            first = false;
            skip_ws();
        }
        return arr;
    }
};

inline value parse(const std::string& s)
{
    parser p(s);
    return p.parse_value();
}

} // namespace mini_json
