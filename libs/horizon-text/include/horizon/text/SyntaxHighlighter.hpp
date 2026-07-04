#pragma once

#include <string>
#include <vector>
#include <regex>
#include <map>
#include <horizon/Color.hpp>

namespace horizon {
namespace text {

enum class TokenType {
    Normal,
    Keyword,
    Operator,
    Comment,
    String,
    Number,
    Type,
    Function,
    Preprocessor
};

struct SyntaxRule {
    std::string name;
    std::regex pattern;
    TokenType type;
};

class SyntaxHighlighter {
public:
    SyntaxHighlighter();
    ~SyntaxHighlighter() = default;

    void load_rules_from_json(const std::string& json_path);
    void load_default_cpp_rules();

    struct HighlightedToken {
        size_t start;
        size_t end;
        TokenType type;
    };

    std::vector<HighlightedToken> highlight_line(const std::u32string& line);

    static Color get_token_color(TokenType type);
    static std::string get_token_color_role(TokenType type);

private:
    std::vector<SyntaxRule> m_rules;
};

} // namespace text
} // namespace horizon
