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
    
    // Manual UTF-8 conversion for regex matching
    std::string utf8_line;
    utf8_line.reserve(line.length());
    std::vector<size_t> char_to_byte;
    char_to_byte.reserve(line.length() + 1);
    
    for (char32_t c : line) {
        char_to_byte.push_back(utf8_line.length());
        if (c <= 0x7F) utf8_line.push_back((char)c);
        else if (c <= 0x7FF) {
            utf8_line.push_back((char)(0xC0 | (c >> 6)));
            utf8_line.push_back((char)(0x80 | (c & 0x3F)));
        } else if (c <= 0xFFFF) {
            utf8_line.push_back((char)(0xE0 | (c >> 12)));
            utf8_line.push_back((char)(0x80 | ((c >> 6) & 0x3F)));
            utf8_line.push_back((char)(0x80 | (c & 0x3F)));
        } else if (c <= 0x10FFFF) {
            utf8_line.push_back((char)(0xF0 | (c >> 18)));
            utf8_line.push_back((char)(0x80 | ((c >> 12) & 0x3F)));
            utf8_line.push_back((char)(0x80 | ((c >> 6) & 0x3F)));
            utf8_line.push_back((char)(0x80 | (c & 0x3F)));
        }
    }
    char_to_byte.push_back(utf8_line.length());
    
    for (const auto& rule : m_rules) {
        auto words_begin = std::sregex_iterator(utf8_line.begin(), utf8_line.end(), rule.pattern);
        auto words_end = std::sregex_iterator();
        
        for (std::sregex_iterator i = words_begin; i != words_end; ++i) {
            std::smatch match = *i;
            size_t start_byte = match.position();
            size_t len_byte = match.length();
            size_t end_byte = start_byte + len_byte;
            
            // Map byte indices back to character indices using our table
            size_t start_char = 0;
            while (start_char < char_to_byte.size() && char_to_byte[start_char] < start_byte) start_char++;
            
            size_t end_char = start_char;
            while (end_char < char_to_byte.size() && char_to_byte[end_char] < end_byte) end_char++;
            
            tokens.push_back({start_char, end_char, rule.type});
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
