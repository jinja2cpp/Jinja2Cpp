#ifndef JINJA2CPP_SRC_LEXER_H
#define JINJA2CPP_SRC_LEXER_H

#include "internal_value.h"
#include "lexertk.h"

#include <cstddef>
#include <functional>
#include <string>
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
    CharRange range = { 0, 0 };
    InternalValue value;

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
    virtual InternalValue GetAsValue(const CharRange& range, Token::Type type) = 0;
    virtual Keyword GetKeyword(const CharRange& range) = 0;
    virtual char GetCharAt(size_t pos) = 0;
};

class Lexer
{
public:
    using TokensList = std::vector<Token>;
    // `storage` lends a token buffer whose capacity the lexer reuses; ReleaseTokens returns it
    Lexer(std::function<lexertk::token()> tokenizer, LexerHelper* helper, TokensList storage = {})
        : m_tokenizer(std::move(tokenizer))
        , m_tokens(std::move(storage))
        , m_helper(helper)
    {
        m_tokens.clear();
    }

    bool Preprocess();
    [[nodiscard]] const TokensList& GetTokens() const
    {
        return m_tokens;
    }

    [[nodiscard]] auto GetHelper() const { return m_helper; }

    TokensList ReleaseTokens() { return std::move(m_tokens); }

private:
    bool ProcessNumber(const lexertk::token& token, Token& newToken);
    bool ProcessSymbolOrKeyword(const lexertk::token& token, Token& newToken);
    bool ProcessString(const lexertk::token& token, Token& newToken);

    std::function<lexertk::token()> m_tokenizer;
    TokensList m_tokens;
    LexerHelper* m_helper;
};

class LexScanner
{
public:
    struct State
    {
        Lexer::TokensList::const_iterator m_begin;
        Lexer::TokensList::const_iterator m_end;
        Lexer::TokensList::const_iterator m_cur;
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

    explicit LexScanner(const Lexer& lexer)
        : m_helper(lexer.GetHelper())
    {
        m_state.m_begin = lexer.GetTokens().begin();
        m_state.m_end = lexer.GetTokens().end();
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

    // The token's source text, for keyword tokens that also serve as names (is none)
    [[nodiscard]] std::string GetAsString(const Token& tok) const
    {
        return m_helper->GetAsString(tok.range);
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
