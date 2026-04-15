#include <horizon/text/SyntaxHighlighter.hpp>
#include <horizon/Logger.hpp>
#include <nlohmann/json.hpp>
#include <fstream>
#include <codecvt>
#include <locale>

namespace horizon {
namespace text {

SyntaxHighlighter::SyntaxHighlighter() {
}

void SyntaxHighlighter::load_default_cpp_rules() {
    m_rules.clear();
    
    // Preprocessor
    m_rules.push_back({"preprocessor", std::regex("^#.*"), TokenType::Preprocessor});
    
    // Comments
    m_rules.push_back({"comment_line", std::regex("//.*"), TokenType::Comment});
    m_rules.push_back({"comment_block", std::regex("/\\*.*\\*/"), TokenType::Comment});
    
    // Strings
    m_rules.push_back({"string", std::regex("\".*?\""), TokenType::String});
    
    // Keywords
    m_rules.push_back({"keyword", std::regex("\\b(if|else|for|while|do|switch|case|default|break|continue|return|goto|try|catch|throw|new|delete|sizeof|typedef|struct|union|enum|class|namespace|using|public|protected|private|template|typename|virtual|override|final|static|const|inline|explicit|friend|volatile|extern|nullptr|true|false)\\b"), TokenType::Keyword});
    
    // Types
    m_rules.push_back({"type", std::regex("\\b(int|char|float|double|bool|void|unsigned|signed|long|short|size_t|std::string|char32_t|char16_t)\\b"), TokenType::Type});
    
    // Numbers
    m_rules.push_back({"number", std::regex("\\b\\d+(\\.\\d+)?\\b"), TokenType::Number});
}

std::vector<SyntaxHighlighter::HighlightedToken> SyntaxHighlighter::highlight_line(const std::u32string& line) {
    std::vector<HighlightedToken> tokens;
    
    // Convert u32string to utf8 for regex matching
    std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> converter;
    std::string utf8_line = converter.to_bytes(line);
    
    // This is a simplified approach: find matches for each rule
    // In a real high-quality highlighter, we'd use a more sophisticated lexer
    for (const auto& rule : m_rules) {
        auto words_begin = std::sregex_iterator(utf8_line.begin(), utf8_line.end(), rule.pattern);
        auto words_end = std::sregex_iterator();
        
        for (std::sregex_iterator i = words_begin; i != words_end; ++i) {
            std::smatch match = *i;
            // Map byte indices back to character indices
            size_t start_byte = match.position();
            size_t len_byte = match.length();
            
            size_t start_char = converter.from_bytes(utf8_line.substr(0, start_byte)).length();
            size_t len_char = converter.from_bytes(match.str()).length();
            
            tokens.push_back({start_char, start_char + len_char, rule.type});
        }
    }
    
    // Sort tokens by start position
    std::sort(tokens.begin(), tokens.end(), [](const HighlightedToken& a, const HighlightedToken& b) {
        return a.start < b.start;
    });
    
    return tokens;
}

Color SyntaxHighlighter::get_token_color(TokenType type) {
    switch (type) {
        case TokenType::Keyword: return Color(0.1, 0.4, 0.8); // Blueish
        case TokenType::Type: return Color(0.2, 0.6, 0.4);    // Greenish
        case TokenType::Comment: return Color(0.5, 0.5, 0.5); // Grey
        case TokenType::String: return Color(0.8, 0.4, 0.1);  // Orange
        case TokenType::Number: return Color(0.5, 0.2, 0.6);  // Purple
        case TokenType::Preprocessor: return Color(0.6, 0.1, 0.1); // Reddish
        default: return Color(0.2, 0.2, 0.2); // Normal
    }
}

} // namespace text
} // namespace horizon
