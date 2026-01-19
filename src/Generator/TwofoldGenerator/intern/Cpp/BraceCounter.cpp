#include "BraceCounter.h"

namespace Twofold::intern::Cpp {
namespace {

constexpr auto ROUND_OPEN = '(';
constexpr auto ROUND_CLOSE = ')';
constexpr auto INDEX_OPEN = '[';
constexpr auto INDEX_CLOSE = ']';
constexpr auto CURLY_OPEN = '{';
constexpr auto CURLY_CLOSE = '}';
constexpr auto DOUBLE_QUOTE = '"';
constexpr auto SINGLE_QUOTE = '\'';
constexpr auto BACK_SLASH = '\\';

constexpr auto isCurlyOrQuote(char chr) -> bool {
    switch (chr) {
    case CURLY_OPEN:
    case CURLY_CLOSE:
    case DOUBLE_QUOTE:
    case SINGLE_QUOTE: return true;
    default: return false;
    }
}

constexpr auto isBraceOrQuote(char chr) -> bool {
    switch (chr) {
    case ROUND_OPEN:
    case ROUND_CLOSE:
    case INDEX_OPEN:
    case INDEX_CLOSE:
    case CURLY_OPEN:
    case CURLY_CLOSE:
    case DOUBLE_QUOTE:
    case SINGLE_QUOTE: return true;
    default: return false;
    }
}

constexpr auto findQuoteEnd(std::string_view::iterator it, std::string_view::iterator const end)
    -> std::string_view::iterator {
    char quote = *it;
    it++;
    while (it != end) {
        if (*it == quote) return it; // found closing quote
        if (*it == BACK_SLASH) {
            it++;
            if (it == end) break; // invalid (TODO: continue next line)
        }
        it++;
    }
    return it; // invalid
}

} // namespace

auto BraceCounter::countExpressionDepth(std::string_view text, int depth) -> int {
    auto const end = text.end();
    for (auto it = text.begin(); it != end;) {
        auto event = std::find_if(it, end, &isBraceOrQuote);
        if (event == end) break;
        switch (*event) {
        case ROUND_CLOSE:
        case INDEX_CLOSE:
        case CURLY_CLOSE:
            if (0 == depth) return -1; // more cloing than opening
            depth--;
            break;
        case ROUND_OPEN:
        case INDEX_OPEN:
        case CURLY_OPEN: depth++; break;
        default: // quote
            event = findQuoteEnd(event, end);
            if (event == end) return depth + 1; // invalid
        }
        it = event + 1;
    }
    return depth;
}

auto BraceCounter::findExpressionEnd(It it, It end) -> It {
    auto depth = 0;
    while (it != end) {
        auto event = std::find_if(it, end, &isCurlyOrQuote);
        if (event == end) return event; // invalid
        switch (*event) {
        case CURLY_CLOSE:
            if (0 == depth) return event; // found end
            depth--;
            break;
        case CURLY_OPEN: depth++; break;
        default: // quote
            event = findQuoteEnd(event, end);
            if (event == end) return event; // invalid
        }
        it = event + 1;
    }
    return it;
}

} // namespace Twofold::intern::Cpp
