#include <string>
#include <unordered_map>

namespace csp {

std::string parse_qstring(const char ** _query, char delim) {
    std::string result;

    const char * query = *_query;

    while (*query && *query != delim) {
        if (*query == '%') {
            query++;
            char byte;
            if (!*query) break;
            byte = (*query++ - '0') * 10;
            if (!*query) break;
            byte += *query++ - '0';
            result += byte;
        } else {
            result += *query++;
        }
    }

    if (*query == delim) query++;

    *_query = query;
    return result;
}

std::unordered_map<std::string, std::string> parse_query(const char * query) {
    std::unordered_map<std::string, std::string> result;

    while (*query) {
        std::string key = parse_qstring(&query, '=');
        std::string value = parse_qstring(&query, '&');
        result[key] = value;
    }

    return result;
}

}
