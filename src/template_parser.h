#ifndef TEMPLATE_PARSER_H
#define TEMPLATE_PARSER_H

#include "renderer.h"
#include "template_parser.h"
#include "lexer.h"
#include "lexertk.h"
#include "expression_evaluator.h"
#include "expression_parser.h"
#include "statements.h"

#include <string>
#include <regex>
#include <vector>
#include <iostream>
#include <list>

namespace jinja2
{
template<typename CharT>
struct ParserTraits;

struct KeywordsInfo
{
    const char* charName;
    const wchar_t* wcharName;
    Token::Type type;
};

template<>
struct ParserTraits<char>
{
    static std::regex GetRoughTokenizer()
    {
        return std::regex(R"((\{\{)|(\}\})|(\{%)|(%\})|(\{#)|(#\})|(\n))");
    }
    template<size_t N>
    static std::regex GetKeywords(KeywordsInfo (&keywords)[N])
    {
        std::string pattern;
        std::string prefix("(^");
        std::string postfix("$)");

        bool isFirst = true;
        for (auto& info : keywords)
        {
            if (!isFirst)
                pattern += "|";
            else
                isFirst = false;

            pattern += prefix + info.charName + postfix;
        }
        return std::regex(pattern);
    }
    static std::string GetAsString(const std::string& str, CharRange range)
    {
        return str.substr(range.startOffset, range.size());
    }
    static Value RangeToNum(const std::string& str, CharRange range, Token::Type hint)
    {
        char buff[std::max(std::numeric_limits<int64_t>::max_digits10, std::numeric_limits<double>::max_digits10) * 2 + 1];
        std::copy(str.data() + range.startOffset, str.data() + range.endOffset, buff);
        buff[range.size()] = 0;
        Value result;
        if (hint == Token::IntegerNum)
        {
            result = Value(static_cast<int64_t>(strtoll(buff, nullptr, 0)));
        }
        else
        {
            char* endBuff = nullptr;
            int64_t val = strtoll(buff, &endBuff, 10);
            if ((errno == ERANGE) || *endBuff)
            {
                endBuff = nullptr;
                double dblVal = strtod(buff, nullptr);
                result = dblVal;
            }
            else
                result = val;
        }
        return result;
    }
    static Token::Type s_keywords[];
};

template<>
struct ParserTraits<wchar_t>
{
    static std::wregex GetRoughTokenizer()
    {
        return std::wregex(LR"((\{\{)|(\}\})|(\{%)|(%\})|(\{#)|(#\})|(\n))");
    }
    template<size_t N>
    static std::wregex GetKeywords(KeywordsInfo (&keywords)[N])
    {
        std::wstring pattern;
        std::wstring prefix(L"(^");
        std::wstring postfix(L"$)");

        bool isFirst = true;
        for (auto& info : keywords)
        {
            if (!isFirst)
                pattern += L"|";
            else
                isFirst = false;

            pattern += prefix + info.wcharName + postfix;
        }
        return std::wregex(pattern);
    }
    static std::string GetAsString(const std::wstring& str, CharRange range)
    {
        auto tmpStr = str.substr(range.startOffset, range.size());
        std::string result;
        result.resize(tmpStr.size(), '\0');
#ifdef _MSC_VER
        size_t dummy = 0;
        wcstombs_s(&dummy, &result[0], result.size(), tmpStr.c_str(), tmpStr.size());
#else
        wcstombs(&result[0], tmpStr.c_str(), result.size());
#endif
        return result;
    }
    static Value RangeToNum(const std::wstring& /*str*/, CharRange /*range*/, Token::Type /*hint*/)
    {
        return Value();
    }
    static Token::Type s_keywords[];
};

struct StatementInfo
{
    enum Type
    {
        TemplateRoot,
        IfStatement,
        ElseIfStatement,
        ForStatement,
        SetStatement
    };

    using ComposedPtr = std::shared_ptr<ComposedRenderer>;
    Type type;
    ComposedPtr currentComposition;
    std::vector<ComposedPtr> compositions;
    size_t position;
    RendererPtr renderer;

    static StatementInfo Create(Type type, size_t pos, ComposedPtr renderers = std::make_shared<ComposedRenderer>())
    {
        StatementInfo result;
        result.type = type;
        result.currentComposition = renderers;
        result.compositions.push_back(renderers);
        result.position = pos;
        return result;
    }
};

using StatementInfoList = std::list<StatementInfo>;

class StatementsParser
{
public:
    bool Parse(LexScanner& lexer, StatementInfoList& statementsInfo);

private:
    bool ParseFor(LexScanner& lexer, StatementInfoList& statementsInfo, size_t pos);
    bool ParseEndFor(LexScanner& lexer, StatementInfoList& statementsInfo, size_t pos);
    bool ParseIf(LexScanner& lexer, StatementInfoList& statementsInfo, size_t pos);
    bool ParseElse(LexScanner& lexer, StatementInfoList& statementsInfo, size_t pos);
    bool ParseElIf(LexScanner& lexer, StatementInfoList& statementsInfo, size_t pos);
    bool ParseEndIf(LexScanner& lexer, StatementInfoList& statementsInfo, size_t pos);
    bool ParseSet(LexScanner& lexer, StatementInfoList& statementsInfo, size_t pos);
    bool ParseEndSet(LexScanner& lexer, StatementInfoList& statementsInfo, size_t pos);
};

template<typename CharT>
class TemplateParser : public LexerHelper
{
public:
    using string_t = std::basic_string<CharT>;
    using traits_t = ParserTraits<CharT>;
    using sregex_iterator = std::regex_iterator<typename string_t::const_iterator>;

    TemplateParser(const string_t& tpl)
        : m_template(&tpl)
        , m_roughTokenizer(traits_t::GetRoughTokenizer())
        , m_keywords(traits_t::GetKeywords(s_keywordsInfo))
    {
    }

    RendererPtr Parse()
    {
        if (!DoRoughParsing())
            return RendererPtr();

        auto composeRenderer = std::make_shared<ComposedRenderer>();

        if (!DoFineParsing(composeRenderer))
            return RendererPtr();

        return composeRenderer;
    }

private:
    enum
    {
        RM_Unknown = 0,
        RM_ExprBegin = 1,
        RM_ExprEnd,
        RM_StmtBegin,
        RM_StmtEnd,
        RM_CommentBegin,
        RM_CommentEnd,
        RM_NewLine
    };

    struct LineInfo
    {
        CharRange range;
        unsigned lineNumber;
    };

    enum class TextBlockType
    {
        RawText,
        Expression,
        Statement,
        Comment,
        LineStatement
    };

    struct TextBlockInfo
    {
        CharRange range;
        TextBlockType type;
    };

    bool DoRoughParsing()
    {
        auto matchBegin = sregex_iterator(m_template->begin(), m_template->end(), m_roughTokenizer);
        auto matchEnd = sregex_iterator();

        auto matches = std::distance(matchBegin, matchEnd);
        // One line, no customization
        if (matches == 0)
        {
            CharRange range{0ULL, m_template->size()};
            m_lines.push_back(LineInfo{range, 0});
            m_textBlocks.push_back(TextBlockInfo{range, m_template->front() == '#' ? TextBlockType::LineStatement : TextBlockType::RawText});
            return true;
        }

        m_currentBlockInfo.range.startOffset = 0;
        m_currentBlockInfo.range.endOffset = 0;
        m_currentLineInfo.range = m_currentBlockInfo.range;
        m_currentLineInfo.lineNumber = 0;
        m_currentBlockInfo.type = m_template->front() == '#' ? TextBlockType::LineStatement : TextBlockType::RawText;
        do
        {
            if (!ParseRoughMatch(matchBegin, matchEnd))
                return false;
        } while (matchBegin != matchEnd);
        FinishCurrentLine(m_template->size());
        FinishCurrentBlock(m_template->size());

        return true;
    }
    bool ParseRoughMatch(sregex_iterator& curMatch, const sregex_iterator& /*endMatch*/)
    {
        auto& match = *curMatch;
        int matchType = RM_Unknown;
        for (int idx = 1; idx != match.size(); ++ idx)
        {
            if (match.length(idx) != 0)
            {
                matchType = idx;
                break;
            }
        }

        switch (matchType)
        {
        case RM_NewLine:
            FinishCurrentLine(match.position());
            m_currentLineInfo.range.startOffset = m_currentLineInfo.range.endOffset + 1;
            if (m_currentLineInfo.range.startOffset < m_template->size() &&
                    (m_currentBlockInfo.type == TextBlockType::RawText || m_currentBlockInfo.type == TextBlockType::LineStatement))
            {
                if (m_currentBlockInfo.type == TextBlockType::LineStatement)
                {
                    FinishCurrentBlock(match.position());
                    m_currentBlockInfo.range.startOffset = m_currentLineInfo.range.startOffset;
                }

                m_currentBlockInfo.type = (*m_template)[m_currentLineInfo.range.startOffset] == '#' ? TextBlockType::LineStatement : TextBlockType::RawText;
            }
            break;
        case RM_CommentBegin:
            if (m_currentBlockInfo.type != TextBlockType::RawText)
            {
                std::cout << "Comment block can be occured only within text" << std::endl;
                return false;
            }
            FinishCurrentBlock(match.position());
            m_currentBlockInfo.range.startOffset = match.position() + 2;
            m_currentBlockInfo.type = TextBlockType::Comment;
            break;

        case RM_CommentEnd:
            if (m_currentBlockInfo.type != TextBlockType::Comment)
            {
                std::cout << "Unexpected '#>'" << std::endl;
                return false;
            }
            FinishCurrentBlock(match.position());
            m_currentBlockInfo.range.startOffset = match.position() + 2;
            break;
        case RM_ExprBegin:
            if (m_currentBlockInfo.type != TextBlockType::RawText)
            {
                break;
            }
            FinishCurrentBlock(match.position());
            m_currentBlockInfo.range.startOffset = match.position() + 2;
            m_currentBlockInfo.type = TextBlockType::Expression;
            break;
        case RM_ExprEnd:
            if (m_currentBlockInfo.type == TextBlockType::RawText)
            {
                std::cout << "Unexpected '}}'" << std::endl;
                return false;
            }
            else if (m_currentBlockInfo.type != TextBlockType::Expression || (*m_template)[match.position() - 1] == '\'')
            {
                break;
            }
            FinishCurrentBlock(match.position());
            m_currentBlockInfo.range.startOffset = match.position() + 2;
            break;
        case RM_StmtBegin:
            if (m_currentBlockInfo.type != TextBlockType::RawText)
            {
                break;
            }
            FinishCurrentBlock(match.position());
            m_currentBlockInfo.range.startOffset = match.position() + 2;
            m_currentBlockInfo.type = TextBlockType::Statement;
            break;
        case RM_StmtEnd:
            if (m_currentBlockInfo.type == TextBlockType::RawText)
            {
                std::cout << "Unexpected '%}'" << std::endl;
                return false;
            }
            else if (m_currentBlockInfo.type != TextBlockType::Statement || (*m_template)[match.position() - 1] == '\'')
            {
                break;
            }
            FinishCurrentBlock(match.position());
            m_currentBlockInfo.range.startOffset = match.position() + 2;
            break;
        }

        ++ curMatch;
        return true;
    }
    bool DoFineParsing(std::shared_ptr<ComposedRenderer> renderers)
    {
        TextBlockInfo* prevBlock = nullptr;
        StatementInfoList statementsStack;
        StatementInfo root = StatementInfo::Create(StatementInfo::TemplateRoot, 0, renderers);
        statementsStack.push_back(root);
        try
        {
            for (auto& origBlock : m_textBlocks)
            {
                auto block = origBlock;
                if (block.type == TextBlockType::LineStatement)
                    ++ block.range.startOffset;

                switch (block.type)
                {
                case TextBlockType::RawText:
                {
                    if (block.range.size() == 0)
                        break;
                    auto range = block.range;
                    if ((*m_template)[range.startOffset] == '\n' && prevBlock != nullptr &&
                            prevBlock->type != TextBlockType::RawText && prevBlock->type != TextBlockType::Expression)
                        range.startOffset ++;
                    if (range.size() == 0)
                        break;
                    auto renderer = std::make_shared<RawTextRenderer>(range.startOffset, range.size());
                    statementsStack.back().currentComposition->AddRenderer(renderer);
                    break;
                }
                case TextBlockType::Expression:
                {
                    auto exprRenderer = InvokeParser<ExpressionParser>(block);
                    if (exprRenderer)
                        statementsStack.back().currentComposition->AddRenderer(exprRenderer);
                    break;
                }
                case TextBlockType::Statement:
                case TextBlockType::LineStatement:
                {
                    if (!InvokeParser<StatementsParser>(block, statementsStack))
                        return false;
                    break;
                }
                default:
                    break;
                }
                prevBlock = &origBlock;
            }
        }
        catch (bool)
        {
            return false;
        }

        return true;
    }
    template<typename P, typename ... Args>
    auto InvokeParser(const TextBlockInfo& block, Args&& ... args)
    {
        lexertk::generator<CharT> tokenizer;
        auto range = block.range;
        auto start = m_template->data();
        if (!tokenizer.process(start + range.startOffset, start + range.endOffset))
        {
            std::cout << "Error processing expression block - tokenizing error";
            throw false;
        }
        tokenizer.begin();
        Lexer lexer([this, &tokenizer, adjust = range.startOffset]() mutable {
            lexertk::token tok = tokenizer.next_token();
            tok.position += adjust;
            return tok;
        }, this);

        if (!lexer.Preprocess())
        {
            std::cout << "Error processing expression block - lexer error";
            throw false;
        }

        P praser;
        LexScanner scanner(lexer);
        return praser.Parse(scanner, std::forward<Args>(args)...);

    }

    void FinishCurrentBlock(size_t position)
    {
        m_currentBlockInfo.range.endOffset = position;
        m_textBlocks.push_back(m_currentBlockInfo);
        m_currentBlockInfo.type = TextBlockType::RawText;
    }
    void FinishCurrentLine(size_t position)
    {
        m_currentLineInfo.range.endOffset = position;
        m_lines.push_back(m_currentLineInfo);
    }

    // LexerHelper interface
    std::string GetAsString(const CharRange& range) override
    {
        return traits_t::GetAsString(*m_template, range);
    }
    Value GetAsValue(const CharRange& range, Token::Type type) override
    {
        if (type == Token::String)
            return Value(m_template->substr(range.startOffset, range.size()));
        else if (type == Token::IntegerNum || type == Token::FloatNum)
            return traits_t::RangeToNum(*m_template, range, type);
        return Value();
    }
    Token::Type GetKeyword(const CharRange& range) override
    {
        auto matchBegin = sregex_iterator(m_template->begin() + range.startOffset, m_template->begin() + range.endOffset, m_keywords);
        auto matchEnd = sregex_iterator();

        auto matches = std::distance(matchBegin, matchEnd);
        // One line, no customization
        if (matches == 0)
            return Token::Unknown;

        auto& match = *matchBegin;
        for (size_t idx = 1; idx != match.size(); ++ idx)
        {
            if (match.length(idx) != 0)
            {
                return s_keywordsInfo[idx - 1].type;
            }
        }

        return Token::Unknown;
    }
    char GetCharAt(size_t /*pos*/) override
    {
        return '\0';
    }
private:
    const string_t* m_template;
    std::basic_regex<CharT> m_roughTokenizer;
    std::basic_regex<CharT> m_keywords;
    std::vector<LineInfo> m_lines;
    std::vector<TextBlockInfo> m_textBlocks;
    LineInfo m_currentLineInfo;
    TextBlockInfo m_currentBlockInfo;
    static KeywordsInfo s_keywordsInfo[30];

};

template<typename CharT>
KeywordsInfo TemplateParser<CharT>::s_keywordsInfo[30] = {
    {"for", L"for", Token::For},
    {"endfor", L"endfor", Token::Endfor},
    {"in", L"in", Token::In},
    {"if", L"if", Token::If},
    {"else", L"else", Token::Else},
    {"elif", L"elif", Token::ElIf},
    {"endif", L"endif", Token::EndIf},
    {"or", L"or", Token::LogicalOr},
    {"and", L"and", Token::LogicalAnd},
    {"not", L"not", Token::LogicalNot},
    {"is", L"is", Token::Is},
    {"block", L"block", Token::Block},
    {"endblock", L"endblock", Token::EndBlock},
    {"extends", L"extends", Token::Extends},
    {"macro", L"macro", Token::Macro},
    {"endmacro", L"endmacro", Token::EndMacro},
    {"call", L"call", Token::Call},
    {"endcall", L"endcall", Token::EndCall},
    {"filter", L"filter", Token::Filter},
    {"endfilter", L"endfilter", Token::EndFilter},
    {"set", L"set", Token::Set},
    {"endset", L"endset", Token::EndSet},
    {"include", L"include", Token::Include},
    {"import", L"import", Token::Import},
    {"true", L"true", Token::True},
    {"false", L"false", Token::False},
    {"True", L"True", Token::True},
    {"False", L"False", Token::False},
    {"none", L"none", Token::None},
    {"None", L"None", Token::None},
};

} // jinga2

#endif // TEMPLATE_PARSER_H
