// Renders the Hugging Face chat templates in test/chat_templates/templates with the
// conversations of conversations.py and compares each result with what transformers renders
// (expected/, written by generate.py). See test/chat_templates/README.md and docs/tasks/0360.

#include <jinja2cpp/binding/nlohmann_json.h>
#include <jinja2cpp/json_adapter.h>
#include <jinja2cpp/string_helpers.h>
#include <jinja2cpp/template.h>
#include <jinja2cpp/template_env.h>
#include <jinja2cpp/user_callable.h>
#include <jinja2cpp/value.h>

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

#include <cctype>
#include <cstdlib>
#include <ctime>
#include <exception>
#include <fstream>
#include <ios>
#include <istream>
#include <map>
#include <memory>
#include <ostream>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

#ifndef JINJA2CPP_CHAT_TEMPLATES_DIR
#error "JINJA2CPP_CHAT_TEMPLATES_DIR must point to test/chat_templates"
#endif

// Keeps the key order of the files: transformers' tojson does not sort keys
using Json = nlohmann::ordered_json;

namespace
{

std::string ReadFile(const std::string& path)
{
    std::ifstream in(path, std::ios::binary);
    std::ostringstream os;
    os << in.rdbuf();
    return os.str();
}

std::string Dir()
{
    return JINJA2CPP_CHAT_TEMPLATES_DIR;
}

// One template rendered with one conversation
struct ChatCase
{
    std::string id; // "<template>.<conversation>"
    std::string templateFile;
    std::string conversation;
    Json expected; // {"output": ...} or {"error": type, "message": ...}

    friend std::ostream& operator<<(std::ostream& os, const ChatCase& c) { return os << c.id; }
};

const Json& Contexts()
{
    static const Json contexts = [] {
        auto text = ReadFile(Dir() + "/expected/conversations.json");
        return text.empty() ? Json::object() : Json::parse(text);
    }();
    return contexts;
}

std::vector<std::string> TemplateFiles()
{
    std::vector<std::string> files;
    auto text = ReadFile(Dir() + "/SOURCES.json");
    if (!text.empty())
    {
        const auto doc = Json::parse(text); // the range-for below must not iterate a temporary
        for (const auto& entry : doc["templates"])
        {
            files.push_back(entry["file"].get<std::string>());
        }
    }
    return files;
}

std::string Stem(const std::string& file)
{
    return file.substr(0, file.rfind('.'));
}

std::vector<ChatCase> LoadCases()
{
    std::vector<ChatCase> cases;
    for (const auto& file : TemplateFiles())
    {
        auto text = ReadFile(Dir() + "/expected/" + Stem(file) + ".json");
        if (text.empty())
        {
            continue; // reported by ChatTemplateRegistry.ExpectationsPresent
        }
        auto doc = Json::parse(text);
        for (auto it = doc["renders"].begin(); it != doc["renders"].end(); ++it)
        {
            // A template's own conversation "<template>:<name>" is listed as "<template>.<name>"
            const auto& conv = it.key();
            const auto colon = conv.find(':');
            const auto shortName = colon == std::string::npos ? conv : conv.substr(colon + 1);
            cases.push_back(ChatCase{ Stem(file) + "." + shortName, file, conv, it.value() });
        }
    }
    return cases;
}

// divergences.txt: "<id> <kind> <task> <reason>"; a "wide." prefix limits a line to the wide run
struct Divergence
{
    std::string kind; // output, rejects, accepts or message
    std::string task;
    std::string reason;
};

const std::map<std::string, Divergence>& Divergences()
{
    static const std::map<std::string, Divergence> result = [] {
        std::map<std::string, Divergence> m;
        std::istringstream in(ReadFile(Dir() + "/divergences.txt"));
        std::string line;
        while (std::getline(in, line))
        {
            if (!line.empty() && line.back() == '\r')
            {
                line.pop_back();
            }
            std::istringstream ls(line);
            std::string id;
            Divergence d;
            if (!(ls >> id) || id[0] == '#')
            {
                continue;
            }
            ls >> d.kind >> d.task;
            std::getline(ls >> std::ws, d.reason);
            m.emplace(id, d);
        }
        return m;
    }();
    return result;
}

// The arguments of the globals arrive narrow or wide, as the template is
std::string Narrow(const jinja2::Value& v)
{
    const auto& data = v.GetData();
    if (const auto* s = v.GetIf<std::string>())
    {
        return *s;
    }
    if (const auto* s = std::get_if<std::string_view>(&data))
    {
        return std::string(*s);
    }
    if (const auto* s = v.GetIf<std::wstring>())
    {
        return jinja2::ConvertString<std::string>(*s);
    }
    if (const auto* s = std::get_if<std::wstring_view>(&data))
    {
        return jinja2::ConvertString<std::string>(*s);
    }
    throw std::runtime_error("expected a string");
}

// transformers' strftime_now reads datetime.now(); the suite and generate.py (FIXED_NOW) read
// 2025-03-14 09:26:53, a Friday
std::tm FixedNow()
{
    std::tm t{};
    t.tm_year = 2025 - 1900;
    t.tm_mon = 2;
    t.tm_mday = 14;
    t.tm_hour = 9;
    t.tm_min = 26;
    t.tm_sec = 53;
    t.tm_wday = 5;
    t.tm_yday = 72;
    return t;
}

// The environment transformers' apply_chat_template renders with, as far as the public API
// expresses it today. Jinja2's own tojson stays: the transformers one comes with the preset
// (docs/tasks/0360), and divergences.txt lists the renders that differ until then.
void ConfigureEnv(jinja2::TemplateEnv& env)
{
    jinja2::Settings settings;
    settings.trimBlocks = true;
    settings.lstripBlocks = true;
    settings.extensions.loopControls = true;
    env.SetSettings(settings);
    env.AddGlobal("raise_exception",
                  jinja2::UserCallable{ [](const jinja2::UserCallableParams& p) -> jinja2::Value { throw std::runtime_error(Narrow(p["message"])); },
                                        { { "message", true } } });
    env.AddGlobal("strftime_now",
                  jinja2::UserCallable{ [](const jinja2::UserCallableParams& p) -> jinja2::Value {
                                           const auto format = Narrow(p["format"]);
                                           const auto now = FixedNow();
                                           std::string out(256, '\0');
                                           out.resize(std::strftime(out.data(), out.size(), format.c_str(), &now));
                                           return out;
                                       },
                                        { { "format", true } } });
}

struct Result
{
    bool ok = false;
    std::string text; // output, or error description
};

template<typename CharT>
Result Render(const ChatCase& c)
{
    using Tpl = std::conditional_t<std::is_same_v<CharT, char>, jinja2::Template, jinja2::TemplateW>;
    jinja2::TemplateEnv env;
    ConfigureEnv(env);

    jinja2::ValuesMap params;
    const auto& ctx = Contexts()[c.conversation];
    for (auto it = ctx.begin(); it != ctx.end(); ++it)
    {
        auto value = jinja2::ToValue(it.value());
        if (!value)
        {
            return { false, "context: " + value.error().ToString() };
        }
        params[it.key()] = std::move(*value);
    }

    try
    {
        Tpl tpl(&env);
        const auto source = ReadFile(Dir() + "/templates/" + c.templateFile);
        auto load = [&] {
            if constexpr (std::is_same_v<CharT, char>)
            {
                return tpl.Load(source, c.templateFile);
            }
            else
            {
                return tpl.Load(jinja2::ConvertString<std::wstring>(source), c.templateFile);
            }
        }();
        if (!load)
        {
            return { false, jinja2::ToString(load.error().ToString()) };
        }
        auto rendered = tpl.RenderAsString(params);
        if (!rendered)
        {
            return { false, jinja2::ToString(rendered.error().ToString()) };
        }
        return { true, jinja2::ToString(*rendered) };
    }
    catch (const std::exception& e)
    {
        return { false, std::string("exception: ") + e.what() };
    }
}

// How the C++ result relates to transformers'; empty when they agree
std::string Compare(const Json& expected, const Result& r)
{
    if (expected.contains("output"))
    {
        if (!r.ok)
        {
            return "rejects";
        }
        return r.text == expected["output"].get<std::string>() ? "" : "output";
    }
    if (r.ok)
    {
        return "accepts";
    }
    // raise_exception's message is the template's own text, so it must come through
    if (expected["error"] == "TemplateError" && expected.contains("message") && r.text.find(expected["message"].get<std::string>()) == std::string::npos)
    {
        return "message";
    }
    return {};
}

std::string Describe(const Json& expected, const Result& r)
{
    std::ostringstream os;
    if (expected.contains("output"))
    {
        os << "transformers:\n"
           << expected["output"].get<std::string>() << "\n";
    }
    else
    {
        os << "transformers: " << expected["error"].get<std::string>() << (expected.contains("message") ? ": " + expected["message"].get<std::string>() : "") << "\n";
    }
    os << (r.ok ? "Jinja2C++:\n" : "Jinja2C++ error: ") << r.text << "\n";
    return os.str();
}

// With JINJA2CPP_CHAT_TEMPLATES_RESULTS=<file> set, every case that diverges appends
// "<id> <kind>" to that file ("wide.<id>" for the wide run), the raw material of divergences.txt
void RecordDivergence(const std::string& id, const std::string& kind)
{
    static std::unique_ptr<std::ofstream> out = []() -> std::unique_ptr<std::ofstream> {
#ifdef _MSC_VER
        // std::getenv is C4996 under /W4 /WX
        char* path = nullptr;
        _dupenv_s(&path, nullptr, "JINJA2CPP_CHAT_TEMPLATES_RESULTS");
        std::unique_ptr<char, decltype(&std::free)> owner(path, &std::free);
#else
        const char* path = std::getenv("JINJA2CPP_CHAT_TEMPLATES_RESULTS");
#endif
        if (!path || !*path)
            return nullptr;
        return std::make_unique<std::ofstream>(path, std::ios::binary | std::ios::app);
    }();
    if (out)
    {
        *out << id << ' ' << kind << std::endl;
    }
}

template<typename CharT>
void Check(const ChatCase& c)
{
    const auto r = Render<CharT>(c);
    const auto kind = Compare(c.expected, r);
    const bool wide = std::is_same_v<CharT, wchar_t>;
    if (!kind.empty())
    {
        RecordDivergence(wide ? "wide." + c.id : c.id, kind);
    }
    auto listed = Divergences().find(wide ? "wide." + c.id : c.id);
    if (listed == Divergences().end())
    {
        listed = Divergences().find(c.id);
    }
    if (listed == Divergences().end())
    {
        EXPECT_EQ(kind, "") << Describe(c.expected, r);
        return;
    }
    // A listed case that now matches must leave the list, so the list stays a true picture
    EXPECT_NE(kind, "") << c.id << " now matches transformers; remove it from test/chat_templates/divergences.txt";
    EXPECT_TRUE(kind.empty() || kind == listed->second.kind) << c.id << " is listed as " << listed->second.kind << " but diverges as " << kind << "\n"
                                                             << Describe(c.expected, r);
}

class ChatTemplateTest : public ::testing::TestWithParam<ChatCase>
{
};
class ChatTemplateWideTest : public ::testing::TestWithParam<ChatCase>
{
};

TEST_P(ChatTemplateTest, MatchesTransformers)
{
    Check<char>(GetParam());
}

TEST_P(ChatTemplateWideTest, MatchesTransformers)
{
    Check<wchar_t>(GetParam());
}

std::string CaseName(const ::testing::TestParamInfo<ChatCase>& info)
{
    std::string name = info.param.id;
    for (auto& ch : name)
    {
        if (!std::isalnum(static_cast<unsigned char>(ch)))
        {
            ch = '_';
        }
    }
    return name;
}

} // namespace

INSTANTIATE_TEST_SUITE_P(ChatTemplates, ChatTemplateTest, ::testing::ValuesIn(LoadCases()), CaseName);
INSTANTIATE_TEST_SUITE_P(ChatTemplatesWide, ChatTemplateWideTest, ::testing::ValuesIn(LoadCases()), CaseName);

TEST(ChatTemplateRegistry, ExpectationsPresent)
{
    EXPECT_FALSE(Contexts().empty()) << "missing expected/conversations.json; run test/chat_templates/generate.py";
    const auto files = TemplateFiles();
    EXPECT_FALSE(files.empty()) << "missing test/chat_templates/SOURCES.json";
    for (const auto& file : files)
    {
        EXPECT_FALSE(ReadFile(Dir() + "/expected/" + Stem(file) + ".json").empty()) << "missing expected/" << Stem(file) << ".json; run generate.py";
    }
}

TEST(ChatTemplateRegistry, DivergencesNameExistingCases)
{
    std::set<std::string> ids;
    for (const auto& c : LoadCases())
    {
        ids.insert(c.id);
    }
    static const std::set<std::string> kinds = { "output", "rejects", "accepts", "message" };
    for (const auto& [id, d] : Divergences())
    {
        const auto plain = id.compare(0, 5, "wide.") == 0 ? id.substr(5) : id;
        EXPECT_TRUE(ids.count(plain)) << id << " in divergences.txt names no case";
        EXPECT_TRUE(kinds.count(d.kind)) << id << ": unknown kind " << d.kind;
        EXPECT_FALSE(d.task.empty() || d.reason.empty()) << id << ": task and reason are required";
    }
}
