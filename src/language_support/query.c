#include <stdlib.h>
#include <string.h>

const char * csp_parse_qstring(const char ** _query, char delim) {
    char * result = malloc(16);
    size_t i = 0;

    const char * query = *_query;

    while (*query && *query != delim) {
        if (*query == '%') {
            query++;
            char byte;
            if (!*query) break;
            byte = (*query++ - '0') * 10;
            if (!*query) break;
            byte += *query++ - '0';
            result[i++] = byte;
        } else {
            result[i++] = *query++;
        }
        if (i % 16) result = realloc(result, i + 16);
    }

    if (*query == delim) query++;

    *_query = query;
    return result;
}
