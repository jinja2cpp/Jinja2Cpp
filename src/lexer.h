#ifndef JINJA2CPP_SRC_LEXER_H
#define JINJA2CPP_SRC_LEXER_H

#include "internal_value.h"
#include "lexertk.h"

#include <cstddef>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace jinja2
{
struct CharRange
{
    size_t startOffset;
    size_t endOffset;
    [[nodiscard]] auto size() const { return endOffset - startOffset; }
};

enum class Keyword
{
    Unknown,

    // Keywords
    LogicalOr,
    LogicalAnd,
    LogicalNot,
    True,
    False,
    None,
    In,
    Is,
    For,
    Endfor,
    If,
    Else,
    ElIf,
    EndIf,
    Block,
    EndBlock,
    Extends,
    Macro,
    EndMacro,
    Call,
    EndCall,
    Filter,
    EndFilter,
    Autoescape,
    EndAutoescape,
    Set,
    EndSet,
    Include,
    Import,
    Recursive,
    Scoped,
    With,
    EndWith,
    Without,
    Ignore,
    Missing,
    Context,
    From,
    As,
    Do,
};

struct Token
{
    // One-character operators are their character; the rest count up from Eof
    enum Type // NOLINT(readability-enum-initial-value)
    {
        Unknown,

        // One-symbol operators
        Lt = '<',
        Gt = '>',
        Plus = '+',
        Minus = '-',
        Percent = '%',
        Mul = '*',
        Div = '/',
        LBracket = '(',
        RBracket = ')',
        LSqBracket = '[',
        RSqBracket = ']',
        LCrlBracket = '{',
        RCrlBracket = '}',
        Assign = '=',
        Comma = ',',
        Colon = ':',
        Eof = 256,

        // General
        Identifier,
        IntegerNum,
        FloatNum,
        String,

        // Operators
        Equal,
        NotEqual,
        LessEqual,
        GreaterEqual,
        StarStar,
        DashDash,
        MulMul,
        DivDiv,
        True,
        False,
        None,

        // Keywords
        LogicalOr,
        LogicalAnd,
        LogicalNot,
        In,
        Is,
        For,
        Endfor,
        If,
        Else,
        ElIf,
        EndIf,
        Block,
        EndBlock,
        Extends,
        Macro,
        EndMacro,
        Call,
        EndCall,
        Filter,
        EndFilter,
        Autoescape,
        EndAutoescape,
        Set,
        EndSet,
        Include,
        Import,
        Recursive,
        Scoped,
        With,
        EndWith,
        Without,
        Ignore,
        Missing,
        Context,
        From,
        As,
        Do,

        // Template control
        CommentBegin,
        CommentEnd,
        RawBegin,
        RawEnd,
        MetaBegin,
        MetaEnd,
        StmtBegin,
        StmtEnd,
        ExprBegin,
        ExprEnd,
    };

    Type type = Unknown;
    // What the text of a symbol token is as a keyword, found once by the lexer
    Keyword keyword = Keyword::Unknown;
    // The value of a literal is read from here when the parser asks (LexScanner::GetValue): the
    // token stays trivial to copy and to drop
    CharRange range = { 0, 0 };

    [[nodiscard]] bool IsEof() const
    {
        return type == Eof;
    }

    bool operator==(char ch) const
    {
        return type == ch;
    }

    bool operator==(Type t) const
    {
        return type == t;
    }

    template<typename T>
    bool operator!=(T v) const
    {
        return !(*this == v);
    }
};


struct LexerHelper
{
    virtual ~LexerHelper() = default;
    virtual std::string GetAsString(const CharRange& range) = 0;
    // The text of a narrow template, null for a wide one: LexScanner::GetAsView reads names from it
    [[nodiscard]] virtual const char* NarrowSource() const = 0;
    // A wide template's text converted to UTF-8; valid until the next call
    virtual std::string_view GetAsConvertedView(const CharRange& range) = 0;
    // The value of a string or number literal
    virtual InternalValue GetAsValue(const CharRange& range, Token::Type type) = 0;
    virtual Keyword GetKeyword(const CharRange& range) = 0;
};

using TokensList = std::vector<Token>;

// A numeric literal as the lexer has checked it: an int64_t, or a double when it is not an
// integer or does not fit one
InternalValue ParseNumberLiteral(std::string number);

// The lexer of a tag's body (docs/tasks/0142). It reads the template source in place and makes
// the parser's tokens straight away. LexToEnd also finds where the tag ends, so the splitter does
// not scan a tag before it is lexed. `Helper` gives the keyword a name is (GetKeyword).
template<typename CharT, typename Helper>
class TagLexer
{
public:
    TagLexer(std::basic_string_view<CharT> source, Helper& helper, TokensList& tokens)
        : m_src(source)
        , m_helper(helper)
        , m_tokens(tokens)
    {
        m_tokens.clear();
    }

    // Lexes [begin, end) as a whole; false on a lexing error. A `;` ends the tokens (an end of
    // block token stands for it), though the rest of the range must still lex.
    bool LexRange(size_t begin, size_t end)
    {
        // A tag whose start modifier is also the first character of its end delimiter, as in
        // `{{-}` with the end `-}`, ends before its body starts
        if (end < begin)
        {
            return false;
        }
        m_base = begin;
        bool stopped = false;
        for (size_t pos = SkipWhitespace(begin, end); pos != end; pos = SkipWhitespace(pos, end))
        {
            Token tok;
            switch (ScanToken(pos, end, tok))
            {
            case Scan::Error:
                return false;
            case Scan::Semicolon:
                if (!stopped)
                {
                    tok.type = Token::Eof;
                    m_tokens.push_back(std::move(tok));
                    stopped = true;
                }
                break;
            case Scan::Token:
                if (!stopped)
                {
                    m_tokens.push_back(std::move(tok));
                }
                break;
            }
            pos = m_next;
        }
        if (!stopped)
        {
            PushEof(end);
        }
        return true;
    }

    // Lexes from `begin` up to the first `end` delimiter, or `-`/`+` and the delimiter, that
    // starts a token outside brackets, as Jinja2 finds the end of a tag. Returns where the
    // delimiter itself starts, or npos when the splitter has to find the end by its own scan:
    // after a lexing error or a `;`, and in a tag that runs to the end of the template. Only for
    // an `end` that cannot start inside a token (UsableEnd).
    size_t LexToEnd(size_t begin, std::basic_string_view<CharT> end)
    {
        m_base = begin;
        const auto size = m_src.size();
        unsigned balance = 0;
        for (size_t pos = SkipWhitespace(begin, size);; pos = SkipWhitespace(m_next, size))
        {
            if (pos == size)
            {
                return std::basic_string_view<CharT>::npos;
            }
            if (balance == 0)
            {
                const auto ch = m_src[pos];
                if ((ch == '-' || ch == '+') && IsAt(pos + 1, end))
                {
                    PushEof(pos);
                    return pos + 1;
                }
                if (IsAt(pos, end))
                {
                    PushEof(pos);
                    return pos;
                }
            }
            auto& tok = m_tokens.emplace_back();
            if (ScanToken(pos, size, tok) != Scan::Token || EndsInExponent(tok))
            {
                return std::basic_string_view<CharT>::npos;
            }
            // A tag that is never closed would be lexed to the end of the template before the
            // splitter's own scan reports it: check for an end delimiter once the tag is long
            if (m_tokens.size() == LongTag && m_src.find(end, m_next) == std::basic_string_view<CharT>::npos)
            {
                return std::basic_string_view<CharT>::npos;
            }
            if (tok.type == '(' || tok.type == '[' || tok.type == '{')
            {
                ++balance;
            }
            else if ((tok.type == ')' || tok.type == ']' || tok.type == '}') && balance != 0)
            {
                --balance;
            }
        }
    }

    // Whether LexToEnd can find `end`: every place outside a string literal where it may match
    // has to be where a token starts, so that the scan by characters the splitter did before
    // finds the same end. That holds unless `end` starts with a character that can be inside a
    // token: of a name, a number, a two-character operator or a modifier.
    static bool UsableEnd(std::basic_string_view<CharT> end)
    {
        if (end.empty())
        {
            return false;
        }
        const auto ch = end[0];
        if (!traits::is_ascii(ch) || traits::is_letter_or_digit(ch) || traits::is_whitespace(ch))
        {
            return false;
        }
        for (char special : { '_', '.', '=', '<', '>', '*', '/', '-', '+', '\'', '"', '\\' })
        {
            if (ch == static_cast<CharT>(special))
            {
                return false;
            }
        }
        return true;
    }

private:
    using traits = lexertk::details::lexer_traits<CharT>;

    enum class Scan
    {
        Token,
        Error,
        // `;`, which ends the tokens of a tag
        Semicolon,
    };

    [[nodiscard]] size_t SkipWhitespace(size_t pos, size_t end) const
    {
        while (pos != end && traits::is_whitespace(m_src[pos]))
        {
            ++pos;
        }
        return pos;
    }

    // Delimiters are short and the first character decides most calls: no compare call
    bool IsAt(size_t pos, std::basic_string_view<CharT> str) const
    {
        if (str.size() > m_src.size() - pos || m_src[pos] != str[0])
        {
            return false;
        }
        for (size_t idx = 1; idx < str.size(); ++idx)
        {
            if (m_src[pos + idx] != str[idx])
            {
                return false;
            }
        }
        return true;
    }

    void PushEof(size_t pos)
    {
        auto& eof = m_tokens.emplace_back();
        eof.type = Token::Eof;
        eof.range = { pos, pos };
    }

    // The token that starts at `pos` (not whitespace), lexed up to `end` at most. m_next gets
    // where the next one may start.
    Scan ScanToken(size_t pos, size_t end, Token& tok)
    {
        const auto ch = m_src[pos];
        if (traits::is_operator_char(ch))
        {
            return ScanOperator(pos, end, tok);
        }
        if (traits::is_letter(ch) || ch == '_')
        {
            auto last = pos + 1;
            while (last != end && (traits::is_letter_or_digit(m_src[last]) || m_src[last] == '_'))
            {
                ++last;
            }
            SetSymbol(pos, last, tok);
            return Scan::Token;
        }
        if (ch == '.')
        {
            // Python has no .5 literal: the dot is an attribute or subscript operator
            return ScanOperator(pos, end, tok);
        }
        if (traits::is_digit(ch))
        {
            return ScanNumber(pos, end, tok);
        }
        if (ch == '\'' || ch == '"')
        {
            return ScanString(pos, end, tok);
        }
        return Scan::Error;
    }

    Scan ScanOperator(size_t pos, size_t end, Token& tok)
    {
        const auto c0 = m_src[pos];
        tok.range = { pos, pos + 1 };
        m_next = pos + 1;
        if (pos + 1 != end)
        {
            const auto c1 = m_src[pos + 1];
            auto type = Token::Unknown;
            if (c1 == '=')
            {
                switch (c0)
                {
                case '<':
                    type = Token::LessEqual;
                    break;
                case '>':
                    type = Token::GreaterEqual;
                    break;
                case '!':
                    type = Token::NotEqual;
                    break;
                case '=':
                    type = Token::Equal;
                    break;
                case ':':
                    // `:=` lexes as `=`
                    type = Token::Assign;
                    break;
                default:
                    break;
                }
            }
            else if (c0 == '<' && c1 == '>')
            {
                type = Token::NotEqual;
            }
            else if (c0 == c1)
            {
                switch (c0)
                {
                case '*':
                    type = Token::MulMul;
                    break;
                case '/':
                    type = Token::DivDiv;
                    break;
                // `<<` and `>>` are no Jinja2 operators: the parser rejects these types
                case '<':
                    type = static_cast<Token::Type>(ShiftLeft);
                    break;
                case '>':
                    type = static_cast<Token::Type>(ShiftRight);
                    break;
                default:
                    break;
                }
            }
            if (type != Token::Unknown)
            {
                tok.type = type;
                tok.range.endOffset = m_next = pos + 2;
                return Scan::Token;
            }
        }
        if (c0 == ';')
        {
            return Scan::Semicolon;
        }
        if (c0 == '&')
        {
            // A name to the parser, which rejects it
            SetSymbol(pos, pos + 1, tok);
            return Scan::Token;
        }
        // One-character operators are their character
        tok.type = static_cast<Token::Type>(c0); // NOLINT(clang-analyzer-optin.core.EnumCastOutOfRange)
        return Scan::Token;
    }

    void SetSymbol(size_t pos, size_t last, Token& tok)
    {
        tok.range = { pos, last };
        m_next = last;
        tok.keyword = m_helper.GetKeyword(tok.range);
        switch (tok.keyword)
        {
        case Keyword::None:
            tok.type = Token::None;
            break;
        case Keyword::True:
            tok.type = Token::True;
            break;
        case Keyword::False:
            tok.type = Token::False;
            break;
        default:
            // The name stays in the source: the parser reads it with LexScanner::GetAsString
            // where a node needs it
            tok.type = Token::Identifier;
            break;
        }
    }

    [[nodiscard]] bool IsDigitAt(size_t pos, size_t end) const { return pos < end && traits::is_digit(m_src[pos]); }

    // A numeric literal as Python writes it: 123, 1.5, 1e3, 1.5E-3, 1_000, 0x1F, 0o17, 0b101
    Scan ScanNumber(size_t begin, size_t end, Token& tok)
    {
        if (m_src[begin] == '0' && begin + 1 != end)
        {
            if (const int radix = GetRadix(m_src[begin + 1]))
            {
                return ScanRadixNumber(begin, end, radix, tok);
            }
        }

        // A number right after a dot is an integer: l.0.1 is l .0 .1
        DecimalState state;
        state.afterDot = begin != m_base && m_src[begin - 1] == '.';
        auto pos = begin;
        for (; pos != end; ++pos)
        {
            const auto step = DecimalStep(begin, pos, end, state);
            if (step == Step::Error)
            {
                return Scan::Error;
            }
            if (step == Step::Stop)
            {
                break;
            }
        }

        // 1e+ has no exponent digits; Python rejects decimal integers with a leading zero (01,
        // 0_1), except 0, 00 and 0_0
        if ((state.exponent && !state.exponentDigit) || (!state.dot && !state.exponent && m_src[begin] == '0' && !IsZeroInteger(begin, pos)))
        {
            return Scan::Error;
        }
        SetNumber(begin, pos, tok);
        return Scan::Token;
    }

    // What a decimal literal has met so far
    struct DecimalState
    {
        bool afterDot = false;
        bool dot = false;
        bool exponent = false;
        bool exponentSign = false;
        bool exponentDigit = false;
    };

    enum class Step
    {
        Consume,
        Stop,
        Error,
    };

    // Whether the character at `pos` continues the decimal literal that starts at `begin`
    Step DecimalStep(size_t begin, size_t pos, size_t end, DecimalState& state) const
    {
        const auto ch = m_src[pos];
        if (ch == '.')
        {
            // As in Jinja2, a fraction needs a digit after the dot: 1.e3 is 1 .e3, 1.5.2 is 1.5 .2
            if (state.dot || state.exponent || state.afterDot || !IsDigitAt(pos + 1, end))
            {
                return Step::Stop;
            }
            state.dot = true;
            return Step::Consume;
        }
        if ((ch == 'e' || ch == 'E') && !state.afterDot)
        {
            if (pos + 1 == end || (m_src[pos + 1] != '+' && m_src[pos + 1] != '-' && !traits::is_digit(m_src[pos + 1])))
            {
                return Step::Error;
            }
            state.exponent = true;
            return Step::Consume;
        }
        if (state.exponent && (ch == '+' || ch == '-') && !state.exponentDigit)
        {
            if (state.exponentSign)
            {
                return Step::Error;
            }
            state.exponentSign = true;
            return Step::Consume;
        }
        if (traits::is_digit(ch))
        {
            state.exponentDigit = state.exponentDigit || state.exponent;
            return Step::Consume;
        }
        // Digit separator, as in Python: 1_000, 1_000.5, 1e1_0
        if (ch == '_' && pos != begin && traits::is_digit(m_src[pos - 1]) && IsDigitAt(pos + 1, end))
        {
            return Step::Consume;
        }
        return Step::Stop;
    }

    static int GetRadix(CharT ch)
    {
        switch (ch)
        {
        case 'x':
        case 'X':
            return 16;
        case 'o':
        case 'O':
            return 8;
        case 'b':
        case 'B':
            return 2;
        default:
            return 0;
        }
    }

    static bool IsRadixDigit(CharT ch, int radix)
    {
        if (ch >= '0' && ch <= '9')
        {
            return ch - '0' < radix;
        }
        return radix == 16 && ((ch >= 'a' && ch <= 'f') || (ch >= 'A' && ch <= 'F'));
    }

    [[nodiscard]] bool IsZeroInteger(size_t begin, size_t end) const
    {
        for (; begin != end; ++begin)
        {
            if (m_src[begin] != '0' && m_src[begin] != '_')
            {
                return false;
            }
        }
        return true;
    }

    // 0x1F, 0o17, 0b101, with single underscores before digits (0x_ff, 0b1_0). Like Python, the
    // number stops at the first character that is not a digit of its radix: 0x1for x is 0x1f or x
    Scan ScanRadixNumber(size_t begin, size_t end, int radix, Token& tok)
    {
        bool digitFound = false;
        auto pos = begin + 2;
        while (pos != end)
        {
            if (IsRadixDigit(m_src[pos], radix))
            {
                digitFound = true;
            }
            else if (m_src[pos] != '_' || pos + 1 == end || !IsRadixDigit(m_src[pos + 1], radix))
            {
                break;
            }
            ++pos;
        }
        if (!digitFound)
        {
            return Scan::Error;
        }
        SetNumber(begin, pos, tok);
        return Scan::Token;
    }

    void SetNumber(size_t begin, size_t last, Token& tok)
    {
        tok.type = Token::FloatNum;
        tok.range = { begin, last };
        m_next = last;
    }

    // A string literal: the token's range is its text between the quotes
    Scan ScanString(size_t pos, size_t end, Token& tok)
    {
        const auto quote = m_src[pos];
        if (end - pos < 2)
        {
            return Scan::Error;
        }
        auto cur = pos + 1;
        bool escaped = false;
        for (; cur != end; ++cur)
        {
            if (escaped)
            {
                escaped = false;
            }
            else if (m_src[cur] == '\\')
            {
                escaped = true;
            }
            else if (m_src[cur] == quote)
            {
                break;
            }
        }
        if (cur == end)
        {
            return Scan::Error;
        }
        tok.type = Token::String;
        tok.range = { pos + 1, cur };
        m_next = cur + 1;
        return Scan::Token;
    }

    // A decimal literal that ends in `e`, as in 1e1e-: lexed up to the end of the tag only, it
    // would end in the `e` and be an error, since the `-` after it is the end's modifier
    [[nodiscard]] bool EndsInExponent(const Token& tok) const
    {
        if (tok.type != Token::FloatNum)
        {
            return false;
        }
        const auto last = m_src[tok.range.endOffset - 1];
        const bool radix = tok.range.size() > 1 && m_src[tok.range.startOffset] == '0' && GetRadix(m_src[tok.range.startOffset + 1]) != 0;
        return !radix && (last == 'e' || last == 'E');
    }

    // The tokens after which LexToEnd makes sure the tag has an end at all
    static constexpr size_t LongTag = 256;
    // The token types lexertk gave `<<` and `>>`, kept for the error messages
    static constexpr int ShiftRight = 11;
    static constexpr int ShiftLeft = 12;

    std::basic_string_view<CharT> m_src;
    Helper& m_helper;
    TokensList& m_tokens;
    // Where the tag's body starts
    size_t m_base = 0;
    size_t m_next = 0;
};

class LexScanner
{
public:
    struct State
    {
        TokensList::const_iterator m_begin;
        TokensList::const_iterator m_end;
        TokensList::const_iterator m_cur;
    };

    struct StateSaver
    {
        explicit StateSaver(LexScanner& scanner)
            : m_state(scanner.m_state)
            , m_scanner(scanner)
        {
        }
        StateSaver(const StateSaver&) = delete;
        StateSaver(StateSaver&&) = delete;
        StateSaver& operator=(const StateSaver&) = delete;
        StateSaver& operator=(StateSaver&&) = delete;

        ~StateSaver()
        {
            if (!m_commited)
            {
                m_scanner.m_state = m_state;
            }
        }

        void Commit()
        {
            m_commited = true;
        }

        State m_state;
        LexScanner& m_scanner;
        bool m_commited = false;
    };

    LexScanner(const TokensList& tokens, LexerHelper* helper)
        : m_helper(helper)
        , m_narrowSource(helper->NarrowSource())
    {
        m_state.m_begin = tokens.begin();
        m_state.m_end = tokens.end();
        Reset();
    }

    void Reset()
    {
        m_state.m_cur = m_state.m_begin;
    }

    [[nodiscard]] auto GetState() const
    {
        return m_state;
    }

    void RestoreState(const State& state)
    {
        m_state = state;
    }

    const Token& NextToken()
    {
        if (m_state.m_cur == m_state.m_end)
        {
            return EofToken();
        }

        return *m_state.m_cur++;
    }

    void EatToken()
    {
        if (m_state.m_cur != m_state.m_end)
        {
            ++m_state.m_cur;
        }
    }

    void ReturnToken()
    {
        if (m_state.m_cur != m_state.m_begin)
        {
            --m_state.m_cur;
        }
    }

    [[nodiscard]] const Token& PeekNextToken() const
    {
        if (m_state.m_cur == m_state.m_end)
        {
            return EofToken();
        }

        return *m_state.m_cur;
    }

    bool EatIfEqual(char type, Token* tok = nullptr)
    {
        // A token type is a character for every one-character operator, enumerator or not
        return EatIfEqual(static_cast<Token::Type>(type), tok); // NOLINT(clang-analyzer-optin.core.EnumCastOutOfRange)
    }

    bool EatIfEqual(Token::Type type, Token* tok = nullptr)
    {
        if (m_state.m_cur == m_state.m_end)
        {
            if (type == Token::Type::Eof && tok)
            {
                *tok = EofToken();
            }

            return type == Token::Type::Eof;
        }

        return EatIfEqualImpl(tok, [type](const Token& t) { return t.type == type; });
    }

    // The token's source text: the name of an identifier (which the lexer does not copy into
    // the token's value), or a keyword token that also serves as a name (is none)
    [[nodiscard]] std::string GetAsString(const Token& tok) const
    {
        return m_helper->GetAsString(tok.range);
    }

    // GetAsString without a copy: valid until the next call, and as long as the source
    [[nodiscard]] std::string_view GetAsView(const Token& tok) const
    {
        if (m_narrowSource)
        {
            return { m_narrowSource + tok.range.startOffset, tok.range.size() };
        }
        return m_helper->GetAsConvertedView(tok.range);
    }

    // The value of a string or number literal
    [[nodiscard]] InternalValue GetValue(const Token& tok) const
    {
        return m_helper->GetAsValue(tok.range, tok.type);
    }

    bool EatIfEqual(Keyword kwType, Token* tok = nullptr)
    {
        if (m_state.m_cur == m_state.m_end)
        {
            return false;
        }

        return EatIfEqualImpl(tok, [kwType](const Token& t) { return t.keyword == kwType; });
    }

private:
    template<typename Fn>
    bool EatIfEqualImpl(Token* tok, const Fn& predicate)
    {
        if (predicate(*m_state.m_cur))
        {
            if (tok)
            {
                *tok = *m_state.m_cur;
            }
            ++m_state.m_cur;
            return true;
        }

        return false;
    }

    State m_state;
    LexerHelper* m_helper;
    const char* m_narrowSource;

    static const Token& EofToken()
    {
        static Token eof;
        eof.type = Token::Eof;
        return eof;
    }
};

} // namespace jinja2

namespace std
{
template<>
struct hash<jinja2::Keyword>
{
    size_t operator()(jinja2::Keyword kw) const
    {
        return std::hash<int>{}(static_cast<int>(kw));
    }
};
} // namespace std

#endif // JINJA2CPP_SRC_LEXER_H
