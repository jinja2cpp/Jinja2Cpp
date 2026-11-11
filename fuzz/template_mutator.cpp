// Structure-aware mutation of templates (docs/tasks/0095); see template_mutator.h.
#include "template_mutator.h"

#include "fuzz_common.h"

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <random>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace jinja2_fuzz
{
namespace
{

enum class PieceKind
{
    Text,
    Expression,
    Statement,
    Comment,
};

// A piece of the template, its delimiters included
struct Piece
{
    PieceKind kind = PieceKind::Text;
    std::string text;
};

// Statements that take a body and an end tag. `set` does only without `=` (`{% set x %}...{% endset %}`)
constexpr std::string_view BlockStatements[] = { "for", "if", "macro", "call", "filter", "block", "with", "raw", "trans", "autoescape", "set" };

// Expressions over the names of MakeContext(), the support templates and the host-surface context (fuzz_common.h,
// untrusted_targets.cpp), and over the names the snippets below define
constexpr std::string_view ExpressionPool[] = {
    "x",
    "x + y * 2",
    "neg // 2",
    "x ** 3",
    "y|round(1)",
    "s|upper",
    "s[::-1]",
    "s.split()|join('-')",
    "'%s=%d'|format(s, x)",
    "s ~ x",
    "l|sort|join(',')",
    "l[1:]",
    "dup|unique|list",
    "l|batch(2)|list",
    "words|sort(case_sensitive=true)",
    "d|dictsort",
    "d.items()|list",
    "d['a']",
    "users|map(attribute='name')|join",
    "users|selectattr('age', 'gt', 26)|list",
    "users|groupby('age')|list",
    "users|sum(attribute='age')",
    "range(3)|list",
    "e|first|default('none')",
    "n is none",
    "t and not n",
    "x if t else s",
    "html",
    "html|e",
    "html|safe",
    "{'k': l, 'n': n}",
    "(1, 'a', [x])",
    "loop.index",
    "loop.cycle('a', 'b')",
    "caller()",
    "caller(1)",
    "super()",
    "self.content()",
    "m(1, 2, 3, k=4)",
    "varargs",
    "kwargs",
    "item",
    "v",
    "mac.m(x)",
    "ns.c",
    "cap",
    "rec.inner.name",
    "recs|map(attribute='id')|list",
    "add(1, b=x)",
    "concat(1, s)",
    "kw(a=1)",
    "lookup('x')",
    "make(3).title",
    "fail(1)",
    "ints|sort",
    "gen|list",
    "s|twice",
    "x is positive",
};

// A statement with a body: the text before it, and the text after it (its end tag first)
struct Wrapper
{
    std::string_view open;
    std::string_view close;
};

constexpr Wrapper Wrappers[] = {
    { "{% if t %}", "{% endif %}" },
    { "{% if not t %}{% elif x > 2 %}", "{% else %}{{ s }}{% endif %}" },
    { "{% for item in l %}", "{% endfor %}" },
    { "{% for k, v in d|dictsort if v > 1 %}", "{% else %}empty{% endfor %}" },
    { "{% for item in users recursive %}", "{{ loop(item.children|default([])) }}{% endfor %}" },
    { "{% with a = x, b = s %}", "{% endwith %}" },
    { "{% macro mm(a, b=1) %}", "{% endmacro %}{{ mm(x) }}" },
    { "{% macro mm() %}", "{% endmacro %}{% call(q) mm() %}{{ q }}{% endcall %}" },
    { "{% call(q) m(1, 2) %}", "{% endcall %}" },
    { "{% filter upper|trim %}", "{% endfilter %}" },
    { "{% block content %}", "{% endblock %}" },
    { "{% set cap %}", "{% endset %}{{ cap }}" },
    { "{% set cap | upper %}", "{% endset %}" },
    { "{% autoescape true %}", "{% endautoescape %}" },
    { "{% raw %}", "{% endraw %}" },
    { "{% trans %}", "{% endtrans %}" },
};

constexpr std::string_view Snippets[] = {
    "{% extends 'base' %}",
    "{% include 'self' ignore missing %}",
    "{% include ['missing', 'header'] %}",
    "{% import 'macros' as mac %}",
    "{% from 'macros' import m, v %}",
    "{% set x = x + 1 %}",
    "{% set ns = namespace(c=0) %}",
    "{% set ns.c = ns.c + 1 %}",
    "{% do l.append(x) %}",
    "{% break %}",
    "{% continue %}",
    "{% else %}",
    "{{ m(1) }}",
    "{# note #}",
    "{%- if t -%}a{%- endif -%}",
    "{{- s -}}",
};

struct Splitter
{
    const Delimiters& delims;

    std::vector<Piece> Split(std::string_view body) const
    {
        std::vector<Piece> pieces;
        std::string text;
        std::size_t pos = 0;
        while (pos < body.size())
        {
            PieceKind kind = PieceKind::Text;
            std::string_view end;
            if (StartsWith(body, pos, delims.commentStart))
            {
                kind = PieceKind::Comment;
                end = delims.commentEnd;
            }
            else if (StartsWith(body, pos, delims.blockStart))
            {
                kind = PieceKind::Statement;
                end = delims.blockEnd;
            }
            else if (StartsWith(body, pos, delims.variableStart))
            {
                kind = PieceKind::Expression;
                end = delims.variableEnd;
            }
            if (kind == PieceKind::Text)
            {
                text.push_back(body[pos++]);
                continue;
            }
            if (!text.empty())
            {
                pieces.push_back({ PieceKind::Text, std::move(text) });
                text.clear();
            }
            const std::size_t startLength = Start(kind).size();
            const std::size_t close = body.find(end, pos + startLength);
            const std::size_t stop = close == std::string_view::npos ? body.size() : close + end.size();
            pieces.push_back({ kind, std::string(body.substr(pos, stop - pos)) });
            pos = stop;
        }
        if (!text.empty())
        {
            pieces.push_back({ PieceKind::Text, std::move(text) });
        }
        return pieces;
    }

    std::string_view Start(PieceKind kind) const
    {
        switch (kind)
        {
        case PieceKind::Expression:
            return delims.variableStart;
        case PieceKind::Statement:
            return delims.blockStart;
        case PieceKind::Comment:
            return delims.commentStart;
        case PieceKind::Text:
            break;
        }
        return {};
    }

    std::string_view End(PieceKind kind) const
    {
        switch (kind)
        {
        case PieceKind::Expression:
            return delims.variableEnd;
        case PieceKind::Statement:
            return delims.blockEnd;
        case PieceKind::Comment:
            return delims.commentEnd;
        case PieceKind::Text:
            break;
        }
        return {};
    }

    // The text between the delimiters, whitespace-control markers left out
    std::string_view Inner(const Piece& piece) const
    {
        std::string_view inner = piece.text;
        const auto start = Start(piece.kind);
        const auto end = End(piece.kind);
        inner.remove_prefix(std::min(start.size(), inner.size()));
        if (inner.size() >= end.size() && inner.substr(inner.size() - end.size()) == end)
        {
            inner.remove_suffix(end.size());
        }
        if (!inner.empty() && (inner.front() == '-' || inner.front() == '+'))
        {
            inner.remove_prefix(1);
        }
        if (!inner.empty() && (inner.back() == '-' || inner.back() == '+'))
        {
            inner.remove_suffix(1);
        }
        return inner;
    }

    // The statement's name (`for`, `endfor`), or empty
    std::string Keyword(const Piece& piece) const
    {
        if (piece.kind != PieceKind::Statement)
        {
            return {};
        }
        const auto inner = Inner(piece);
        std::size_t pos = 0;
        while (pos < inner.size() && std::isspace(static_cast<unsigned char>(inner[pos])) != 0)
        {
            ++pos;
        }
        std::string word;
        while (pos < inner.size() && (std::isalpha(static_cast<unsigned char>(inner[pos])) != 0 || inner[pos] == '_'))
        {
            word.push_back(inner[pos++]);
        }
        return word;
    }

    // The name of the block statement a piece opens, or empty
    std::string Opens(const Piece& piece) const
    {
        auto word = Keyword(piece);
        if (word == "set" && Inner(piece).find('=') != std::string_view::npos)
        {
            return {};
        }
        for (const auto name : BlockStatements)
        {
            if (word == name)
            {
                return word;
            }
        }
        return {};
    }

    // The name of the block statement a piece ends, or empty
    std::string Closes(const Piece& piece) const
    {
        auto word = Keyword(piece);
        if (word.size() > 3 && word.compare(0, 3, "end") == 0)
        {
            return word.substr(3);
        }
        return {};
    }

    std::string Convert(std::string_view text) const { return WithDelimiters<char>(text, delims); }

    static bool StartsWith(std::string_view text, std::size_t pos, std::string_view prefix)
    {
        return !prefix.empty() && text.compare(pos, prefix.size(), prefix) == 0;
    }
};

class Mutation
{
public:
    Mutation(unsigned seed, const Delimiters& delims, ByteMutator bytes)
        : m_rng(seed)
        , m_splitter{ delims }
        , m_bytes(bytes)
    {
    }

    std::size_t Pick(std::size_t count)
    {
        return count == 0 ? 0 : std::uniform_int_distribution<std::size_t>(0, count - 1)(m_rng);
    }

    template<typename T, std::size_t N>
    const T& PickOf(const T (&items)[N])
    {
        return items[Pick(N)];
    }

    const Splitter& Split() const { return m_splitter; }

    // Nesting depth before each piece, and after the last one
    std::vector<int> Depths(const std::vector<Piece>& pieces) const
    {
        std::vector<int> depths(pieces.size() + 1, 0);
        int depth = 0;
        for (std::size_t i = 0; i < pieces.size(); ++i)
        {
            depths[i] = depth;
            if (!m_splitter.Opens(pieces[i]).empty())
            {
                ++depth;
            }
            else if (!m_splitter.Closes(pieces[i]).empty())
            {
                depth = std::max(0, depth - 1);
            }
        }
        depths[pieces.size()] = depth;
        return depths;
    }

    // A range [first, last) of whole statements: it neither closes a statement opened before it nor leaves one open
    std::pair<std::size_t, std::size_t> BalancedRange(const std::vector<Piece>& pieces)
    {
        const auto depths = Depths(pieces);
        const std::size_t first = Pick(pieces.size() + 1);
        std::vector<std::size_t> lasts;
        for (std::size_t last = first; last <= pieces.size(); ++last)
        {
            if (depths[last] < depths[first])
            {
                break;
            }
            if (depths[last] == depths[first])
            {
                lasts.push_back(last);
            }
        }
        return { first, lasts[Pick(lasts.size())] };
    }

    std::string Expression(const std::vector<Piece>& own, const std::vector<Piece>* other)
    {
        // An expression of the input or the crossover partner half of the time, when they have one
        std::vector<std::string_view> found;
        for (const auto* pieces : { &own, other })
        {
            if (pieces)
            {
                for (const auto& piece : *pieces)
                {
                    if (piece.kind == PieceKind::Expression)
                    {
                        found.push_back(m_splitter.Inner(piece));
                    }
                }
            }
        }
        if (!found.empty() && Pick(2) == 0)
        {
            return std::string(found[Pick(found.size())]);
        }
        return std::string(PickOf(ExpressionPool));
    }

    Piece ExpressionPiece(std::string_view expression) const
    {
        const auto& delims = m_splitter.delims;
        std::string text(delims.variableStart);
        text.append(" ").append(expression).append(" ").append(delims.variableEnd);
        return { PieceKind::Expression, std::move(text) };
    }

    // The byte mutator over one piece, which may grow by `room` bytes
    void MutateBytes(Piece& piece, std::size_t room)
    {
        std::string buffer = piece.text.empty() ? std::string(" ") : piece.text;
        const std::size_t size = buffer.size();
        buffer.resize(size + room);
        const std::size_t newSize = m_bytes(reinterpret_cast<std::uint8_t*>(buffer.data()), size, buffer.size());
        buffer.resize(std::min(newSize, buffer.size()));
        piece.text = std::move(buffer);
    }

    // The end tags of the statements left open, innermost first
    std::vector<Piece> Unclosed(const std::vector<Piece>& pieces) const
    {
        std::vector<std::string> open;
        for (const auto& piece : pieces)
        {
            if (auto name = m_splitter.Opens(piece); !name.empty())
            {
                open.push_back(std::move(name));
            }
            else if (auto closed = m_splitter.Closes(piece); !closed.empty())
            {
                const auto match = std::find(open.rbegin(), open.rend(), closed);
                if (match != open.rend())
                {
                    open.erase(std::next(match).base(), open.end());
                }
            }
        }
        std::vector<Piece> ends;
        for (auto name = open.rbegin(); name != open.rend(); ++name)
        {
            ends.push_back({ PieceKind::Statement, m_splitter.Convert("{% end" + *name + " %}") });
        }
        return ends;
    }

    // One structural mutation of `pieces`; false when it did not apply
    bool Structural(std::vector<Piece>& pieces, std::size_t room)
    {
        auto insert = [&pieces](std::size_t at, std::vector<Piece> added) {
            pieces.insert(pieces.begin() + static_cast<std::ptrdiff_t>(at), std::make_move_iterator(added.begin()), std::make_move_iterator(added.end()));
        };
        auto converted = [this](std::string_view text) { return m_splitter.Split(m_splitter.Convert(text)); };
        switch (Pick(9))
        {
        case 0: // the byte mutator inside one piece
            if (pieces.empty())
            {
                return false;
            }
            MutateBytes(pieces[Pick(pieces.size())], room);
            return true;
        case 1: // duplicate a run of whole statements somewhere else
        {
            const auto [first, last] = BalancedRange(pieces);
            if (first == last)
            {
                return false;
            }
            std::vector<Piece> copy(pieces.begin() + static_cast<std::ptrdiff_t>(first), pieces.begin() + static_cast<std::ptrdiff_t>(last));
            insert(Pick(pieces.size() + 1), std::move(copy));
            return true;
        }
        case 2: // swap two pieces
            if (pieces.size() < 2)
            {
                return false;
            }
            std::swap(pieces[Pick(pieces.size())], pieces[Pick(pieces.size())]);
            return true;
        case 3: // delete a run of whole statements, or one piece
        {
            if (pieces.empty())
            {
                return false;
            }
            auto [first, last] = BalancedRange(pieces);
            if (first == last || Pick(3) == 0)
            {
                first = Pick(pieces.size());
                last = first + 1;
            }
            pieces.erase(pieces.begin() + static_cast<std::ptrdiff_t>(first), pieces.begin() + static_cast<std::ptrdiff_t>(last));
            return true;
        }
        case 4: // replace an expression, or a text piece, with an expression
        {
            std::vector<std::size_t> targets;
            for (std::size_t i = 0; i < pieces.size(); ++i)
            {
                if (pieces[i].kind == PieceKind::Expression || pieces[i].kind == PieceKind::Text)
                {
                    targets.push_back(i);
                }
            }
            auto piece = ExpressionPiece(Expression(pieces, nullptr));
            if (targets.empty())
            {
                pieces.push_back(std::move(piece));
            }
            else
            {
                pieces[targets[Pick(targets.size())]] = std::move(piece);
            }
            return true;
        }
        case 5: // wrap a run of whole statements in a statement with a body
        {
            const auto [first, last] = BalancedRange(pieces);
            const auto& wrapper = PickOf(Wrappers);
            insert(last, converted(wrapper.close));
            insert(first, converted(wrapper.open));
            return true;
        }
        case 6: // insert a statement
            insert(Pick(pieces.size() + 1), converted(PickOf(Snippets)));
            return true;
        case 7: // insert an expression
            insert(Pick(pieces.size() + 1), { ExpressionPiece(Expression(pieces, nullptr)) });
            return true;
        default: // close the statements left open
        {
            auto ends = Unclosed(pieces);
            if (ends.empty())
            {
                return false;
            }
            // At the end, or the innermost one somewhere after it was opened
            if (Pick(2) == 0)
            {
                insert(pieces.size(), std::move(ends));
            }
            else
            {
                insert(pieces.size() - Pick(std::min<std::size_t>(pieces.size(), 3) + 1), { std::move(ends.front()) });
            }
            return true;
        }
        }
    }

    // pieces1 with a run of pieces2 spliced in, or one of its expressions replaced with one of pieces2
    std::vector<Piece> Cross(const std::vector<Piece>& pieces1, const std::vector<Piece>& pieces2)
    {
        std::vector<Piece> result;
        switch (Pick(3))
        {
        case 0: // splice a run of whole statements of the other input in
        {
            const auto [first, last] = BalancedRange(pieces2);
            const std::size_t at = Pick(pieces1.size() + 1);
            result.assign(pieces1.begin(), pieces1.begin() + static_cast<std::ptrdiff_t>(at));
            result.insert(result.end(), pieces2.begin() + static_cast<std::ptrdiff_t>(first), pieces2.begin() + static_cast<std::ptrdiff_t>(last));
            result.insert(result.end(), pieces1.begin() + static_cast<std::ptrdiff_t>(at), pieces1.end());
            break;
        }
        case 1: // the head of one, the tail of the other
        {
            const std::size_t head = Pick(pieces1.size() + 1);
            const std::size_t tail = Pick(pieces2.size() + 1);
            result.assign(pieces1.begin(), pieces1.begin() + static_cast<std::ptrdiff_t>(head));
            result.insert(result.end(), pieces2.begin() + static_cast<std::ptrdiff_t>(tail), pieces2.end());
            break;
        }
        default: // an expression of the other input in place of one of this
        {
            result = pieces1;
            std::vector<std::size_t> targets;
            for (std::size_t i = 0; i < result.size(); ++i)
            {
                if (result[i].kind == PieceKind::Expression)
                {
                    targets.push_back(i);
                }
            }
            auto piece = ExpressionPiece(Expression({}, &pieces2));
            if (targets.empty())
            {
                result.insert(result.begin() + static_cast<std::ptrdiff_t>(Pick(result.size() + 1)), std::move(piece));
            }
            else
            {
                result[targets[Pick(targets.size())]] = std::move(piece);
            }
            break;
        }
        }
        return result;
    }

private:
    std::mt19937 m_rng;
    Splitter m_splitter;
    ByteMutator m_bytes;
};

const Delimiters& InputDelimiters(const std::uint8_t* data, std::size_t size, const InputLayout& layout)
{
    if (layout.settingsOffset >= 0 && static_cast<std::size_t>(layout.settingsOffset) < size)
    {
        return DelimitersOf(data[layout.settingsOffset]);
    }
    return DelimiterSet(0);
}

std::size_t Join(const std::uint8_t* prefix, std::size_t prefixLength, const std::vector<Piece>& pieces, std::uint8_t* out, std::size_t maxSize)
{
    std::size_t size = prefixLength;
    for (const auto& piece : pieces)
    {
        size += piece.text.size();
    }
    if (size > maxSize || size == 0)
    {
        return 0;
    }
    std::memmove(out, prefix, prefixLength);
    std::size_t pos = prefixLength;
    for (const auto& piece : pieces)
    {
        std::memcpy(out + pos, piece.text.data(), piece.text.size());
        pos += piece.text.size();
    }
    return size;
}

std::size_t TextSize(const std::vector<Piece>& pieces)
{
    std::size_t size = 0;
    for (const auto& piece : pieces)
    {
        size += piece.text.size();
    }
    return size;
}

} // namespace

bool StructureMutationEnabled()
{
    static const bool enabled = [] {
        const char* mode = std::getenv("JINJA2CPP_FUZZ_MUTATOR");
        return !mode || std::string_view(mode) != "bytes";
    }();
    return enabled;
}

std::size_t MutateTemplate(std::uint8_t* data, std::size_t size, std::size_t maxSize, unsigned seed, const InputLayout& layout, ByteMutator bytes)
{
    auto byteMutation = [&] { return size == 0 ? 0 : bytes(data, size, maxSize); };
    if (!StructureMutationEnabled() || size < layout.prefixLength || maxSize <= layout.prefixLength)
    {
        return byteMutation();
    }
    Mutation mutation(seed, InputDelimiters(data, size, layout), bytes);
    // A quarter of the time the byte mutator over the whole input, prefix bytes included
    if (mutation.Pick(4) == 0)
    {
        return byteMutation();
    }
    const std::string original(reinterpret_cast<const char*>(data) + layout.prefixLength, size - layout.prefixLength);
    for (int attempt = 0; attempt < 4; ++attempt)
    {
        auto pieces = mutation.Split().Split(original);
        // Up to three mutations stacked, like libFuzzer's own
        const std::size_t count = 1 + mutation.Pick(3);
        bool changed = false;
        for (std::size_t i = 0; i < count; ++i)
        {
            const std::size_t used = layout.prefixLength + TextSize(pieces);
            changed = mutation.Structural(pieces, used < maxSize ? maxSize - used : 0) || changed;
        }
        if (!changed)
        {
            continue;
        }
        std::string text;
        for (const auto& piece : pieces)
        {
            text += piece.text;
        }
        if (text == original || layout.prefixLength + text.size() > maxSize || layout.prefixLength + text.size() == 0)
        {
            continue;
        }
        std::memcpy(data + layout.prefixLength, text.data(), text.size());
        return layout.prefixLength + text.size();
    }
    return byteMutation();
}

std::size_t CrossOverTemplates(const std::uint8_t* data1,
                               std::size_t size1,
                               const std::uint8_t* data2,
                               std::size_t size2,
                               std::uint8_t* out,
                               std::size_t maxOutSize,
                               unsigned seed,
                               const InputLayout& layout)
{
    if (!StructureMutationEnabled() || size1 < layout.prefixLength || size2 < layout.prefixLength)
    {
        return 0;
    }
    // Both split with the delimiters of the first input, whose prefix the result keeps
    Mutation mutation(seed, InputDelimiters(data1, size1, layout), nullptr);
    const auto body = [&layout](const std::uint8_t* data, std::size_t size) {
        return std::string_view(reinterpret_cast<const char*>(data) + layout.prefixLength, size - layout.prefixLength);
    };
    const auto pieces1 = mutation.Split().Split(body(data1, size1));
    const auto pieces2 = mutation.Split().Split(body(data2, size2));
    return Join(data1, layout.prefixLength, mutation.Cross(pieces1, pieces2), out, maxOutSize);
}

} // namespace jinja2_fuzz
