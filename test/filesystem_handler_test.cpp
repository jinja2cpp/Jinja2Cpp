#include <jinja2cpp/filesystem_handler.h>
#include <jinja2cpp/template_env.h>
#include <jinja2cpp/value.h>

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <ios>
#include <iterator>
#include <map>
#include <memory>
#include <optional>
#include <random>
#include <sstream>
#include <string>
#include <system_error>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

class FilesystemHandlerTest : public testing::Test
{
public:
    template<typename CharT>
    std::basic_string<CharT> ReadFile(jinja2::FileStreamPtr<CharT>& stream)
    {
        std::basic_string<CharT> result;
        constexpr size_t buffSize = 0x10000;
        CharT buff[buffSize];

        if (!stream)
            return result;

        while (stream->good() && !stream->eof())
        {
            stream->read(buff, buffSize);
            auto readSize = stream->gcount();
            result.append(buff, buff + readSize);
            if (static_cast<size_t>(readSize) < buffSize)
            {
                break;
            }
        }

        return result;
    }
};

TEST_F(FilesystemHandlerTest, MemoryFS_Narrow2NarrowReading)
{
    const std::string test1Content = R"(
Line1
Line2
Line3
)";
    const std::string test2Content = R"(
Line6
Line7
Line8
)";
    jinja2::MemoryFileSystem fs;
    fs.AddFile("test1.j2tpl", test1Content);
    fs.AddFile("test2.j2tpl", test2Content);

    auto testStream = fs.OpenStream("test.j2tpl");
    EXPECT_FALSE(static_cast<bool>(testStream));
    auto test1Stream = fs.OpenStream("test1.j2tpl");
    EXPECT_TRUE(static_cast<bool>(test1Stream));
    EXPECT_EQ(test1Content, ReadFile(test1Stream));
    auto test2Stream = fs.OpenStream("test2.j2tpl");
    EXPECT_TRUE(static_cast<bool>(test2Stream));
    EXPECT_EQ(test2Content, ReadFile(test2Stream));
}

TEST_F(FilesystemHandlerTest, MemoryFS_Wide2WideReading)
{
    const std::wstring test1Content = LR"(
Line1
Line2
Line3
)";
    const std::wstring test2Content = LR"(
Line6
Line7
Line8
)";
    jinja2::MemoryFileSystem fs;
    fs.AddFile("test1.j2tpl", test1Content);
    fs.AddFile("test2.j2tpl", test2Content);

    auto testStream = fs.OpenWStream("test.j2tpl");
    EXPECT_FALSE(static_cast<bool>(testStream));
    auto test1Stream = fs.OpenWStream("test1.j2tpl");
    EXPECT_TRUE(static_cast<bool>(test1Stream));
    EXPECT_EQ(test1Content, ReadFile(test1Stream));
    auto test2Stream = fs.OpenWStream("test2.j2tpl");
    EXPECT_TRUE(static_cast<bool>(test2Stream));
    EXPECT_EQ(test2Content, ReadFile(test2Stream));
}

TEST_F(FilesystemHandlerTest, RealFS_NarrowReading)
{
    const std::string test1Content =
R"(Hello World!
)";
    jinja2::RealFileSystem fs;
    auto testStream = fs.OpenStream("===incorrect====.j2tpl");
    EXPECT_FALSE(static_cast<bool>(testStream));
    auto test1Stream = fs.OpenStream("test_data/simple_template1.j2tpl");
    EXPECT_TRUE(static_cast<bool>(test1Stream));
    EXPECT_EQ(test1Content, ReadFile(test1Stream));
}

TEST_F(FilesystemHandlerTest, RealFS_RootHandling)
{
    const std::string test1Content =
R"(Hello World!
)";
    jinja2::RealFileSystem fs;

    auto test1Stream = fs.OpenStream("test_data/simple_template1.j2tpl");
    EXPECT_TRUE(static_cast<bool>(test1Stream));
    EXPECT_EQ(test1Content, ReadFile(test1Stream));
    fs.SetRootFolder("./test_data");
    auto test2Stream = fs.OpenStream("simple_template1.j2tpl");
    EXPECT_TRUE(static_cast<bool>(test2Stream));
    EXPECT_EQ(test1Content, ReadFile(test2Stream));
    fs.SetRootFolder("./test_data/");
    auto test3Stream = fs.OpenStream("simple_template1.j2tpl");
    EXPECT_TRUE(static_cast<bool>(test3Stream));
    EXPECT_EQ(test1Content, ReadFile(test3Stream));
}

TEST_F(FilesystemHandlerTest, RealFS_WideReading)
{
    const std::wstring test1Content =
LR"(Hello World!
)";
    jinja2::RealFileSystem fs;
    auto testStream = fs.OpenWStream("===incorrect====.j2tpl");
    EXPECT_FALSE(static_cast<bool>(testStream));
    auto test1Stream = fs.OpenWStream("test_data/simple_template1.j2tpl");
    EXPECT_TRUE(static_cast<bool>(test1Stream));
    EXPECT_EQ(test1Content, ReadFile(test1Stream));
}

TEST_F(FilesystemHandlerTest, TestDefaultCaching)
{
    const std::string test1Content = R"(
Line1
Line2
Line3)";
    const std::string test2Content = R"(
Line6
Line7
Line8
)";
    jinja2::MemoryFileSystem fs;
    fs.AddFile("test1.j2tpl", test1Content);

    jinja2::TemplateEnv env;

    env.AddFilesystemHandler("", fs);
    auto tpl1 = env.LoadTemplate("test1.j2tpl").value();
    EXPECT_EQ(test1Content, tpl1.RenderAsString({}).value());

    fs.AddFile("test1.j2tpl", test2Content);
    auto tpl2 = env.LoadTemplate("test1.j2tpl").value();
    EXPECT_EQ(test1Content, tpl2.RenderAsString({}).value());
}

TEST_F(FilesystemHandlerTest, TestNoCaching)
{
    const std::string test1Content = R"(
Line1
Line2
Line3)";
    const std::string test2Content = R"(
Line6
Line7
Line8)";
    jinja2::MemoryFileSystem fs;
    fs.AddFile("test1.j2tpl", test1Content);

    jinja2::TemplateEnv env;
    env.GetSettings().cacheSize = 0;

    env.AddFilesystemHandler("", fs);
    auto tpl1 = env.LoadTemplate("test1.j2tpl").value();
    EXPECT_EQ(test1Content, tpl1.RenderAsString({}).value());

    fs.AddFile("test1.j2tpl", test2Content);
    auto tpl2 = env.LoadTemplate("test1.j2tpl").value();
    EXPECT_EQ(test2Content, tpl2.RenderAsString({}).value());
}

TEST_F(FilesystemHandlerTest, TestDefaultRFSCaching)
{
    const std::string test1Content = R"(
Line1
Line2
Line3)";
    const std::string test2Content = R"(
Line6
Line7
Line8
)";
    const std::string fileName = "test_data/cached_content.j2tpl";

    jinja2::RealFileSystem fs;
    {
        std::ofstream os(fileName);
        os << test1Content;
    }

    jinja2::TemplateEnv env;
    env.GetSettings().autoReload = false;

    env.AddFilesystemHandler("", fs);
    auto tpl1 = env.LoadTemplate(fileName).value();
    EXPECT_EQ(test1Content, tpl1.RenderAsString({}).value());

    {
        std::ofstream os(fileName);
        os << test2Content;
    }

    auto tpl2 = env.LoadTemplate(fileName).value();
    EXPECT_EQ(test1Content, tpl2.RenderAsString({}).value());
}

TEST_F(FilesystemHandlerTest, TestRFSCachingReload)
{
    const std::string test1Content = R"(
Line1
Line2
Line3)";
    const std::string test2Content = R"(
Line6
Line7
Line8)";
    const std::string fileName = "test_data/cached_content.j2tpl";

    jinja2::RealFileSystem fs;
    {
        std::ofstream os(fileName);
        os << test1Content;
    }

    jinja2::TemplateEnv env;

    env.AddFilesystemHandler("", fs);
    auto tpl1 = env.LoadTemplate(fileName).value();
    EXPECT_EQ(test1Content, tpl1.RenderAsString({}).value());

    std::this_thread::sleep_for(std::chrono::seconds(2));

    {
        std::ofstream os(fileName);
        os << test2Content;
    }

    auto tpl2 = env.LoadTemplate(fileName).value();
    EXPECT_EQ(test2Content, tpl2.RenderAsString({}).value());
}

TEST_F(FilesystemHandlerTest, TestNoRFSCaching)
{
    const std::string test1Content = R"(
Line1
Line2
Line3)";
    const std::string test2Content = R"(
Line6
Line7
Line8)";
    const std::string fileName = "test_data/cached_content.j2tpl";

    jinja2::RealFileSystem fs;
    {
        std::ofstream os(fileName);
        os << test1Content;
    }

    jinja2::TemplateEnv env;
    env.GetSettings().cacheSize = 0;

    env.AddFilesystemHandler("", fs);
    auto tpl1 = env.LoadTemplate(fileName).value();
    EXPECT_EQ(test1Content, tpl1.RenderAsString({}).value());

    {
        std::ofstream os(fileName);
        os << test2Content;
    }

    auto tpl2 = env.LoadTemplate(fileName).value();
    EXPECT_EQ(test2Content, tpl2.RenderAsString({}).value());
}


namespace
{
// Counts how often the environment reaches the filesystem, per file name
class CountingFileSystem : public jinja2::MemoryFileSystem
{
public:
    jinja2::CharFileStreamPtr OpenStream(const std::string& name) const override
    {
        ++opens[name];
        return MemoryFileSystem::OpenStream(name);
    }
    std::optional<std::chrono::system_clock::time_point> GetLastModificationDate(const std::string& name) const override
    {
        ++dateChecks[name];
        auto p = modified.find(name);
        return p == modified.end() ? MemoryFileSystem::GetLastModificationDate(name) : p->second;
    }
    // Replaces the file and moves its modification date forward, so autoReload sees the change
    void Touch(const std::string& name, std::string content)
    {
        AddFile(name, std::move(content));
        auto& date = modified[name];
        date = date ? *date + std::chrono::seconds(1) : std::chrono::system_clock::now();
    }

    mutable std::map<std::string, int> opens;
    mutable std::map<std::string, int> dateChecks;
    std::map<std::string, std::optional<std::chrono::system_clock::time_point>> modified;
};
} // namespace

// A render resolves each template name once (docs/tasks/0105): an include in a loop does not go back to the
// environment, or with caching off to the filesystem, on every iteration
TEST_F(FilesystemHandlerTest, IncludeInLoopLoadsOncePerRender)
{
    CountingFileSystem fs;
    fs.AddFile("main.j2", "{% for i in range(5) %}{% include ['missing.j2', 'item.j2'] %}{% endfor %}");
    fs.AddFile("item.j2", "[{{ i }}]");

    jinja2::TemplateEnv env;
    env.GetSettings().cacheSize = 0;
    env.AddFilesystemHandler("", fs);

    auto tpl = env.LoadTemplate("main.j2").value();
    EXPECT_EQ("[0][1][2][3][4]", tpl.RenderAsString({}).value());
    EXPECT_EQ(1, fs.opens["item.j2"]);
    EXPECT_EQ(1, fs.opens["missing.j2"]);

    // The next render loads again and sees the new content
    fs.AddFile("item.j2", "<{{ i }}>");
    EXPECT_EQ("<0><1><2><3><4>", tpl.RenderAsString({}).value());
    EXPECT_EQ(2, fs.opens["item.j2"]);
    EXPECT_EQ(2, fs.opens["missing.j2"]);
}

// With autoReload the cached template is checked for changes once per render, not once per include
TEST_F(FilesystemHandlerTest, AutoReloadChecksOncePerRender)
{
    CountingFileSystem fs;
    fs.AddFile("main.j2", "{% extends 'base.j2' %}{% block b %}{% for i in range(5) %}{% include 'item.j2' %}{% endfor %}{% endblock %}");
    fs.AddFile("base.j2", "<{% block b %}{% endblock %}>");
    fs.AddFile("item.j2", "{{ i }}");

    jinja2::TemplateEnv env;
    env.GetSettings().autoReload = true;
    env.AddFilesystemHandler("", fs);

    auto tpl = env.LoadTemplate("main.j2").value();
    EXPECT_EQ("<01234>", tpl.RenderAsString({}).value());
    EXPECT_EQ("<01234>", tpl.RenderAsString({}).value());
    EXPECT_EQ(1, fs.opens["item.j2"]);
    EXPECT_EQ(1, fs.opens["base.j2"]);
    // Recorded once when first loaded and cached, then checked once by the second render
    EXPECT_EQ(2, fs.dateChecks["item.j2"]);
    EXPECT_EQ(2, fs.dateChecks["base.j2"]);
}

// One loaded template rendered from several threads, each render resolving its includes in the shared environment
TEST_F(FilesystemHandlerTest, IncludeFromManyThreads)
{
    jinja2::MemoryFileSystem fs;
    fs.AddFile("main.j2", "{% extends 'base.j2' %}{% block b %}{% for i in range(20) %}{% include 'item.j2' %}{% endfor %}{% endblock %}");
    fs.AddFile("base.j2", "<{% block b %}{% endblock %}>");
    fs.AddFile("item.j2", "{{ i }},");

    jinja2::TemplateEnv env;
    env.AddFilesystemHandler("", fs);
    auto tpl = env.LoadTemplate("main.j2").value();

    const std::string expected = "<0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,>";
    std::vector<std::thread> threads;
    std::vector<int> failures(4);
    threads.reserve(failures.size());
    for (int& failure : failures)
    {
        threads.emplace_back([&tpl, &expected, &failure] {
            for (int n = 0; n != 100; ++n)
            {
                auto result = tpl.RenderAsString({});
                if (!result || result.value() != expected)
                {
                    ++failure;
                }
            }
        });
    }
    for (auto& th : threads)
    {
        th.join();
    }
    EXPECT_EQ(std::vector<int>(4), failures);
}

// Two imports of one file with caching off: the second used to release the template the first one's macros live in
TEST_F(FilesystemHandlerTest, ImportSameFileTwiceWithoutCache)
{
    jinja2::MemoryFileSystem fs;
    fs.AddFile("main.j2", "{% import 'm.j2' as a %}{% import 'm.j2' as b %}{{ a.f(1) }}{{ b.f(2) }}");
    fs.AddFile("m.j2", "{% macro f(x) %}[{{ x }}]{% endmacro %}");

    jinja2::TemplateEnv env;
    env.GetSettings().cacheSize = 0;
    env.AddFilesystemHandler("", fs);

    auto tpl = env.LoadTemplate("main.j2").value();
    EXPECT_EQ("[1][2]", tpl.RenderAsString({}).value());
}

// TemplateLookup::EveryUse looks the template up each time, as Jinja2 does: a file reloaded during a render is seen
// by the next include, and with caching off every include reads the file
TEST_F(FilesystemHandlerTest, EveryUseLooksUpOnEveryInclude)
{
    CountingFileSystem fs;
    fs.AddFile("main.j2", "{% for i in range(5) %}{% include ['missing.j2', 'item.j2'] %}{% endfor %}");
    fs.AddFile("item.j2", "[{{ i }}]");

    jinja2::TemplateEnv env;
    env.GetSettings().cacheSize = 0;
    env.GetSettings().templateLookup = jinja2::TemplateLookup::EveryUse;
    env.AddFilesystemHandler("", fs);

    auto tpl = env.LoadTemplate("main.j2").value();
    EXPECT_EQ("[0][1][2][3][4]", tpl.RenderAsString({}).value());
    EXPECT_EQ(5, fs.opens["item.j2"]);
    EXPECT_EQ(5, fs.opens["missing.j2"]);
}

// An include of a constant name keeps what it loaded for the rest of the render (docs/tasks/0154): with
// OncePerRender each render opens the file once, with EveryUse each use opens it
TEST_F(FilesystemHandlerTest, ConstantIncludeLoadsOncePerRender)
{
    for (auto lookup : { jinja2::TemplateLookup::OncePerRender, jinja2::TemplateLookup::EveryUse })
    {
        CountingFileSystem fs;
        fs.AddFile("main.j2", "{% for i in range(4) %}{% include 'item.j2' %}{% include 'gone.j2' ignore missing %}{% endfor %}");
        fs.AddFile("item.j2", "[{{ i }}]");

        jinja2::TemplateEnv env;
        env.GetSettings().cacheSize = 0;
        env.GetSettings().templateLookup = lookup;
        env.AddFilesystemHandler("", fs);

        auto tpl = env.LoadTemplate("main.j2").value();
        const bool everyUse = lookup == jinja2::TemplateLookup::EveryUse;
        EXPECT_EQ("[0][1][2][3]", tpl.RenderAsString({}).value());
        EXPECT_EQ("[0][1][2][3]", tpl.RenderAsString({}).value());
        EXPECT_EQ(everyUse ? 8 : 2, fs.opens["item.j2"]) << (everyUse ? "EveryUse" : "OncePerRender");
    }
}

// A constant name that is missing is not remembered as loaded: `ignore missing` renders nothing in every iteration and
// without it every render fails (Python: TemplateNotFound), also the second one
TEST_F(FilesystemHandlerTest, ConstantMissingIncludeInLoop)
{
    for (auto lookup : { jinja2::TemplateLookup::OncePerRender, jinja2::TemplateLookup::EveryUse })
    {
        CountingFileSystem fs;
        fs.AddFile("ignored.j2", "{% for i in range(3) %}[{% include 'x' ignore missing %}]{% endfor %}");
        fs.AddFile("failing.j2", "{% for i in range(3) %}{% include 'x' %}{% endfor %}");

        jinja2::TemplateEnv env;
        env.GetSettings().cacheSize = 0;
        env.GetSettings().templateLookup = lookup;
        env.AddFilesystemHandler("", fs);

        auto ignored = env.LoadTemplate("ignored.j2").value();
        EXPECT_EQ("[][][]", ignored.RenderAsString({}).value());
        EXPECT_EQ("[][][]", ignored.RenderAsString({}).value());

        auto failing = env.LoadTemplate("failing.j2").value();
        for (int render = 0; render != 2; ++render)
        {
            auto result = failing.RenderAsString({});
            ASSERT_FALSE(result.has_value()) << render;
            EXPECT_EQ(jinja2::ErrorCode::TemplateNotFound, result.error().GetCode()) << render;
        }
    }
}

// `with context` and `without context` includes of a constant name in a loop: each iteration renders with its own
// variables (Python: [|0K][|1K][|2K]), and the file is still opened once per render with OncePerRender
TEST_F(FilesystemHandlerTest, ConstantIncludeContextVariantsInLoop)
{
    for (auto lookup : { jinja2::TemplateLookup::OncePerRender, jinja2::TemplateLookup::EveryUse })
    {
        CountingFileSystem fs;
        fs.AddFile("main.j2", "{% for i in range(3) %}[{% include 'w.j2' without context %}|{% include 'w.j2' with context %}]{% endfor %}");
        fs.AddFile("w.j2", "{{ i }}{{ k }}");

        jinja2::TemplateEnv env;
        env.GetSettings().cacheSize = 0;
        env.GetSettings().templateLookup = lookup;
        env.AddFilesystemHandler("", fs);

        auto tpl = env.LoadTemplate("main.j2").value();
        const bool everyUse = lookup == jinja2::TemplateLookup::EveryUse;
        EXPECT_EQ("[|0K][|1K][|2K]", tpl.RenderAsString({ { "k", "K" } }).value());
        EXPECT_EQ("[|0K][|1K][|2K]", tpl.RenderAsString({ { "k", "K" } }).value());
        EXPECT_EQ(everyUse ? 12 : 2, fs.opens["w.j2"]) << (everyUse ? "EveryUse" : "OncePerRender");
    }
}

// A template reloaded during a render is kept until the render ends, also when the environment does not cache it:
// the macros imported from the old one still run its code (0118 P4b)
TEST_F(FilesystemHandlerTest, ReloadedTemplateKeptForTheRender)
{
    CountingFileSystem fs;
    fs.AddFile("main.j2", "{% import 'mod.j2' as a %}{{ touch() }}{% import 'mod.j2' as b %}{{ a.f() }}{{ b.f() }}");
    fs.Touch("mod.j2", "{% macro f() %}A{% endmacro %}");

    jinja2::TemplateEnv env;
    env.GetSettings().cacheSize = 0;
    env.GetSettings().autoReload = true;
    env.GetSettings().templateLookup = jinja2::TemplateLookup::EveryUse;
    env.AddFilesystemHandler("", fs);
    env.AddGlobal("touch", jinja2::UserCallable([&fs](const jinja2::UserCallableParams&) {
                      fs.Touch("mod.j2", "{% macro f() %}B{% endmacro %}");
                      return jinja2::Value(std::string());
                  },
                                                {}));

    auto tpl = env.LoadTemplate("main.j2").value();
    EXPECT_EQ("AB", tpl.RenderAsString({}).value());
}

TEST_F(FilesystemHandlerTest, TemplateChangedDuringRender)
{
    for (auto lookup : { jinja2::TemplateLookup::OncePerRender, jinja2::TemplateLookup::EveryUse })
    {
        CountingFileSystem fs;
        fs.AddFile("main.j2", "{% include 'item.j2' %}{{ touch() }}{% include 'item.j2' %}");
        fs.Touch("item.j2", "A");

        jinja2::TemplateEnv env;
        env.GetSettings().autoReload = true;
        env.GetSettings().templateLookup = lookup;
        env.AddFilesystemHandler("", fs);
        env.AddGlobal("touch", jinja2::UserCallable([&fs](const jinja2::UserCallableParams&) {
                          fs.Touch("item.j2", "B");
                          return jinja2::Value(std::string());
                      },
                                                    {}));

        auto tpl = env.LoadTemplate("main.j2").value();
        const bool everyUse = lookup == jinja2::TemplateLookup::EveryUse;
        EXPECT_EQ(everyUse ? "AB" : "AA", tpl.RenderAsString({}).value()) << (everyUse ? "EveryUse" : "OncePerRender");
        // The next render sees the change either way
        EXPECT_EQ("BB", tpl.RenderAsString({}).value());
    }
}

namespace
{
// A scratch tree for RealFileSystem name rules:
//   <base>/outside.j2, <base>/root/{a.j2, sub/b.j2, dir/}, <base>/root/link.j2 -> ../outside.j2
class RealFsNamesTest : public FilesystemHandlerTest
{
protected:
    void SetUp() override
    {
        namespace fs = std::filesystem;
        m_base = fs::temp_directory_path() / ("jinja2cpp_names_" + std::to_string(std::random_device{}()));
        fs::create_directories(m_base / "root" / "sub");
        fs::create_directories(m_base / "root" / "dir");
        WriteFile(m_base / "outside.j2", "outside");
        WriteFile(m_base / "root" / "a.j2", "a");
        WriteFile(m_base / "root" / "sub" / "b.j2", "b");
        std::error_code ec;
        fs::create_symlink(fs::path("..") / "outside.j2", m_base / "root" / "link.j2", ec);
        m_hasSymlink = !ec;
        m_root = (m_base / "root").string();
    }

    void TearDown() override
    {
        std::error_code ec;
        std::filesystem::remove_all(m_base, ec);
    }

    static void WriteFile(const std::filesystem::path& path, const std::string& content)
    {
        std::ofstream os(path);
        os << content;
    }

    std::filesystem::path m_base;
    std::string m_root;
    bool m_hasSymlink = false;
};
} // namespace

// Python Jinja2's FileSystemLoader: a ".." piece is not found, "/" and "." pieces are skipped,
// only regular files are found
TEST_F(RealFsNamesTest, NameRules)
{
    const std::string outsideAbsolute = (m_base / "outside.j2").generic_string();
    const std::vector<std::pair<std::string, std::optional<std::string>>> rows = {
        { "a.j2", "a" },
        { "./a.j2", "a" },
        { "/a.j2", "a" },
        { "sub/b.j2", "b" },
        { "sub//b.j2", "b" },
        { "sub/./b.j2", "b" },
        { "../outside.j2", std::nullopt },
        { "sub/../a.j2", std::nullopt },
        { "sub/../../outside.j2", std::nullopt },
        { "..", std::nullopt },
        { "..\\outside.j2", std::nullopt },
        { R"(sub\..\..\outside.j2)", std::nullopt },
        { outsideAbsolute, std::nullopt },
        { "", std::nullopt },
        { ".", std::nullopt },
        { "sub", std::nullopt },
        { "dir", std::nullopt },
        { "missing.j2", std::nullopt },
        { std::string("a.j2\0", 5), std::nullopt },
    };

    jinja2::RealFileSystem fs(m_root);
    for (const auto& [name, expected] : rows)
    {
        SCOPED_TRACE("name: '" + name + "'");
        auto narrow = fs.OpenStream(name);
        auto bytes = fs.OpenByteStream(name);
        auto wide = fs.OpenWStream(name);
        auto modified = fs.GetLastModificationDate(name);
        EXPECT_EQ(expected.has_value(), static_cast<bool>(narrow));
        EXPECT_EQ(expected.has_value(), static_cast<bool>(bytes));
        EXPECT_EQ(expected.has_value(), static_cast<bool>(wide));
        EXPECT_EQ(expected.has_value(), modified.has_value());
        if (expected)
        {
            EXPECT_EQ(*expected, ReadFile(narrow));
            EXPECT_EQ(*expected, ReadFile(bytes));
            EXPECT_EQ(std::wstring(expected->begin(), expected->end()), ReadFile(wide));
        }
    }
}

TEST_F(RealFsNamesTest, FullFilePath)
{
    jinja2::RealFileSystem fs(m_root);
    EXPECT_EQ(m_root + "/a.j2", fs.GetFullFilePath("a.j2"));
    EXPECT_EQ(m_root + "/sub/b.j2", fs.GetFullFilePath("/sub//./b.j2"));
    // An absolute name stays under the root (on Windows its drive ':' or '\\' is not allowed)
#ifdef _WIN32
    EXPECT_EQ("", fs.GetFullFilePath(m_root + "/a.j2"));
#else
    EXPECT_EQ(m_root + m_root + "/a.j2", fs.GetFullFilePath(m_root + "/a.j2"));
#endif
    EXPECT_EQ("", fs.GetFullFilePath("../outside.j2"));
    EXPECT_EQ("", fs.GetFullFilePath("sub/../a.j2"));

    fs.SetRootFolder(m_root + "/");
    EXPECT_EQ(m_root + "/a.j2", fs.GetFullFilePath("a.j2"));
    fs.SetRootFolder(std::string());
    EXPECT_EQ("a.j2", fs.GetFullFilePath("/a.j2"));
}

// std::filesystem's file clock has a different epoch in each standard library: the date is the
// file's modification time in whole seconds, the same as ConfinedFileSystem's fstat/GetFileTime
TEST_F(RealFsNamesTest, LastModificationDate)
{
    namespace fs = std::filesystem;
    using std::chrono::hours;
    using std::chrono::minutes;
    using std::chrono::system_clock;
    const jinja2::RealFileSystem real(m_root);
    const jinja2::ConfinedFileSystem confined(m_root);
    const auto now = system_clock::now();

    auto modified = real.GetLastModificationDate("a.j2");
    ASSERT_TRUE(modified.has_value());
    EXPECT_EQ(confined.GetLastModificationDate("a.j2"), modified);
    EXPECT_EQ(modified, real.GetLastModificationDate("a.j2"));
    EXPECT_LT(*modified, now + minutes(1));
    EXPECT_GT(*modified, now - minutes(10));
    EXPECT_EQ(std::chrono::system_clock::duration::zero(), modified->time_since_epoch() % std::chrono::seconds(1));

    // Ten years back: before the epoch of no supported file clock, but far from the file's own time
    std::error_code ec;
    fs::last_write_time(m_base / "root" / "sub" / "b.j2", fs::file_time_type::clock::now() - hours(24 * 3653), ec);
    ASSERT_FALSE(ec) << ec.message();
    modified = real.GetLastModificationDate("sub/b.j2");
    ASSERT_TRUE(modified.has_value());
    EXPECT_EQ(confined.GetLastModificationDate("sub/b.j2"), modified);
    EXPECT_LT(*modified, now - hours(24 * 3652));
    EXPECT_GT(*modified, now - hours(24 * 3654));
}

TEST_F(RealFsNamesTest, SymlinksAreFollowed)
{
    if (!m_hasSymlink)
    {
        GTEST_SKIP() << "symbolic links cannot be created here";
    }
    // Python's FileSystemLoader follows links, wherever they point
    jinja2::RealFileSystem fs(m_root);
    auto stream = fs.OpenStream("link.j2");
    ASSERT_TRUE(static_cast<bool>(stream));
    EXPECT_EQ("outside", ReadFile(stream));
}

TEST_F(RealFsNamesTest, LoadTemplateNotFound)
{
    jinja2::TemplateEnv env;
    env.AddFilesystemHandler("", std::make_shared<jinja2::RealFileSystem>(m_root));
    EXPECT_EQ("a", env.LoadTemplate("a.j2").value().RenderAsString({}).value());

    for (const std::string name : { "../outside.j2", "sub/../../outside.j2", "missing.j2", "dir" })
    {
        SCOPED_TRACE(name);
        auto result = env.LoadTemplate(name);
        ASSERT_FALSE(result);
        EXPECT_EQ(jinja2::ErrorCode::FileNotFound, result.error().GetCode());
        std::ostringstream os;
        os << result.error();
        EXPECT_EQ(std::string::npos, os.str().find(m_base.string()));
        EXPECT_EQ(std::string::npos, os.str().find(m_base.generic_string()));
    }
}

TEST_F(RealFsNamesTest, CachedTemplateWithRemovedFile)
{
    jinja2::TemplateEnv env;
    env.GetSettings().autoReload = true;
    env.AddFilesystemHandler("", std::make_shared<jinja2::RealFileSystem>(m_root));
    EXPECT_EQ("a", env.LoadTemplate("a.j2").value().RenderAsString({}).value());

    std::filesystem::remove(m_base / "root" / "a.j2");
    // The modification date is unknown now: the cached template is kept, nothing throws
    EXPECT_EQ("a", env.LoadTemplate("a.j2").value().RenderAsString({}).value());
}

namespace
{
// A scratch tree for ConfinedFileSystem:
//   <base>/outside/canary.j2 ("CANARY"), <base>/root/{a.j2, sub/b.j2, dir/}
class ConfinedFsTest : public FilesystemHandlerTest
{
protected:
    void SetUp() override
    {
        namespace fs = std::filesystem;
        m_base = fs::temp_directory_path() / ("jinja2cpp_confined_" + std::to_string(std::random_device{}()));
        fs::create_directories(m_base / "root" / "sub");
        fs::create_directories(m_base / "root" / "dir");
        fs::create_directories(m_base / "outside");
        WriteFile(m_base / "outside" / "canary.j2", "CANARY");
        WriteFile(m_base / "root" / "a.j2", "a");
        WriteFile(m_base / "root" / "sub" / "b.j2", "b");
        m_root = (m_base / "root").string();
    }

    void TearDown() override
    {
        std::error_code ec;
        std::filesystem::remove_all(m_base, ec);
    }

    static void WriteFile(const std::filesystem::path& path, const std::string& content)
    {
        std::ofstream os(path);
        os << content;
    }

    // Creates a symbolic link <root>/<name> with the target text as given
    bool Link(const std::string& name, const std::string& target)
    {
        std::error_code ec;
        std::filesystem::create_symlink(target, m_base / "root" / name, ec);
        return !ec;
    }

    std::filesystem::path m_base;
    std::string m_root;
};
} // namespace

TEST_F(ConfinedFsTest, PlainFilesRender)
{
    jinja2::TemplateEnv env;
    env.AddFilesystemHandler("", std::make_shared<jinja2::ConfinedFileSystem>(m_root));
    EXPECT_EQ("a", env.LoadTemplate("a.j2").value().RenderAsString({}).value());
    EXPECT_EQ("b", env.LoadTemplate("sub/b.j2").value().RenderAsString({}).value());
    EXPECT_EQ("b", env.LoadTemplate("/./sub//b.j2").value().RenderAsString({}).value());
    auto wideTpl = env.LoadTemplateW("sub/b.j2");
    ASSERT_TRUE(wideTpl);
    EXPECT_EQ(L"b", wideTpl.value().RenderAsString({}).value());
}

// On every platform, Windows' final-path check included
TEST_F(ConfinedFsTest, FilesAndNewlines)
{
    {
        std::ofstream os(m_base / "root" / "crlf.j2", std::ios::binary);
        os << "a\r\nb\r";
    }
    const jinja2::ConfinedFileSystem fs(m_root);
    auto read = [](auto stream) {
        using CharT = typename std::decay_t<decltype(*stream)>::char_type;
        return std::basic_string<CharT>(std::istreambuf_iterator<CharT>(*stream), std::istreambuf_iterator<CharT>());
    };
#ifdef _WIN32
    EXPECT_EQ("a\nb\r", read(fs.OpenStream("crlf.j2")));
    EXPECT_EQ(L"a\nb\r", read(fs.OpenWStream("crlf.j2")));
#else
    EXPECT_EQ("a\r\nb\r", read(fs.OpenStream("crlf.j2")));
    EXPECT_EQ(L"a\r\nb\r", read(fs.OpenWStream("crlf.j2")));
#endif
    EXPECT_EQ("a\r\nb\r", read(fs.OpenByteStream("crlf.j2")));
    EXPECT_EQ("b", read(fs.OpenStream("sub/b.j2")));
    EXPECT_TRUE(fs.GetLastModificationDate("sub/b.j2"));
    for (const char* name : { "dir", "sub", "", "../outside/canary.j2", "missing.j2" })
    {
        EXPECT_FALSE(fs.OpenStream(name)) << name;
        EXPECT_FALSE(fs.OpenWStream(name)) << name;
        EXPECT_FALSE(fs.GetLastModificationDate(name)) << name;
    }
}

#ifndef _WIN32

#include <sys/stat.h>

TEST_F(ConfinedFsTest, NameRules)
{
    const std::string outsideAbsolute = (m_base / "outside" / "canary.j2").generic_string();
    const std::vector<std::pair<std::string, std::optional<std::string>>> rows = {
        { "a.j2", "a" },
        { "./a.j2", "a" },
        { "/a.j2", "a" },
        { "sub/b.j2", "b" },
        { "sub//b.j2", "b" },
        { "../outside/canary.j2", std::nullopt },
        { "sub/../a.j2", std::nullopt },
        { "..", std::nullopt },
        { outsideAbsolute, std::nullopt },
        { "", std::nullopt },
        { ".", std::nullopt },
        { "sub", std::nullopt },
        { "dir", std::nullopt },
        { "missing.j2", std::nullopt },
        { "a.j2/x", std::nullopt },
        { std::string("a.j2\0", 5), std::nullopt },
    };

    jinja2::ConfinedFileSystem fs(m_root);
    for (const auto& [name, expected] : rows)
    {
        SCOPED_TRACE("name: '" + name + "'");
        auto narrow = fs.OpenStream(name);
        auto bytes = fs.OpenByteStream(name);
        auto wide = fs.OpenWStream(name);
        auto modified = fs.GetLastModificationDate(name);
        EXPECT_EQ(expected.has_value(), static_cast<bool>(narrow));
        EXPECT_EQ(expected.has_value(), static_cast<bool>(bytes));
        EXPECT_EQ(expected.has_value(), static_cast<bool>(wide));
        EXPECT_EQ(expected.has_value(), modified.has_value());
        if (expected)
        {
            EXPECT_EQ(*expected, ReadFile(narrow));
            EXPECT_EQ(*expected, ReadFile(bytes));
            EXPECT_EQ(std::wstring(expected->begin(), expected->end()), ReadFile(wide));
        }
    }
}

TEST_F(ConfinedFsTest, RootAndEquality)
{
    jinja2::ConfinedFileSystem fs(m_root);
    EXPECT_EQ(m_root, fs.GetRootFolder());
    EXPECT_TRUE(fs.IsEqual(jinja2::ConfinedFileSystem(m_root)));
    EXPECT_FALSE(fs.IsEqual(jinja2::ConfinedFileSystem(m_root + "/sub")));
    EXPECT_FALSE(fs.IsEqual(jinja2::RealFileSystem(m_root)));
    EXPECT_FALSE(jinja2::ConfinedFileSystem("/nonexistent_root_for_test").OpenStream("a.j2"));
    fs.SetRootFolder(m_root + "/sub");
    EXPECT_TRUE(fs.OpenStream("b.j2"));
    EXPECT_FALSE(fs.OpenStream("a.j2"));
}

TEST_F(ConfinedFsTest, LinksInsideRootAreFollowed)
{
    if (!Link("l.j2", "a.j2") || !Link("sub/up.j2", "../a.j2") || !Link("dirlink", "sub") || !Link("chain1", "chain2") || !Link("chain2", "sub/../a.j2"))
    {
        GTEST_SKIP() << "symbolic links cannot be created here";
    }
    jinja2::ConfinedFileSystem fs(m_root);
    for (const std::string name : { "l.j2", "sub/up.j2", "dirlink/b.j2", "chain1" })
    {
        SCOPED_TRACE(name);
        auto stream = fs.OpenStream(name);
        ASSERT_TRUE(static_cast<bool>(stream));
        EXPECT_FALSE(ReadFile(stream).empty());
        EXPECT_TRUE(fs.GetLastModificationDate(name).has_value());
    }

    jinja2::TemplateEnv env;
    env.AddFilesystemHandler("", std::make_shared<jinja2::ConfinedFileSystem>(m_root));
    EXPECT_EQ("a", env.LoadTemplate("sub/up.j2").value().RenderAsString({}).value());
    EXPECT_EQ("b", env.LoadTemplate("dirlink/b.j2").value().RenderAsString({}).value());
}

TEST_F(ConfinedFsTest, LinksLeadingOutsideAreNotFound)
{
    const std::string insideAbsolute = (m_base / "root" / "a.j2").string();
    const std::string outsideAbsolute = (m_base / "outside" / "canary.j2").string();
    if (!Link("out.j2", "../outside/canary.j2") || !Link("sub/out2.j2", "../../outside/canary.j2") || !Link("abs_inside.j2", insideAbsolute) || !Link("abs_outside.j2", outsideAbsolute) || !Link("outdir", "../outside") || !Link("sub/deep", "../../outside") || !Link("climb", "sub/../../outside/canary.j2") || !Link("loop1", "loop2") || !Link("loop2", "loop1") || !Link("self", "self") || !Link("dir_link", "dir"))
    {
        GTEST_SKIP() << "symbolic links cannot be created here";
    }
    jinja2::ConfinedFileSystem fs(m_root);
    for (const std::string name : { "out.j2", "sub/out2.j2", "abs_inside.j2", "abs_outside.j2", "outdir/canary.j2", "sub/deep/canary.j2", "climb", "loop1", "self", "dir_link" })
    {
        SCOPED_TRACE(name);
        EXPECT_FALSE(fs.OpenStream(name));
        EXPECT_FALSE(fs.OpenWStream(name));
        EXPECT_FALSE(fs.OpenByteStream(name));
        EXPECT_FALSE(fs.GetLastModificationDate(name).has_value());
    }

    jinja2::TemplateEnv env;
    env.AddFilesystemHandler("", std::make_shared<jinja2::ConfinedFileSystem>(m_root));
    auto result = env.LoadTemplate("out.j2");
    ASSERT_FALSE(result);
    EXPECT_EQ(jinja2::ErrorCode::FileNotFound, result.error().GetCode());
    std::ostringstream os;
    os << result.error();
    EXPECT_EQ(std::string::npos, os.str().find(m_base.string()));

    // RealFileSystem follows the same link
    jinja2::RealFileSystem real(m_root);
    auto stream = real.OpenStream("out.j2");
    ASSERT_TRUE(static_cast<bool>(stream));
    EXPECT_EQ("CANARY", ReadFile(stream));
}

TEST_F(ConfinedFsTest, RootItselfMayBeALink)
{
    std::error_code ec;
    std::filesystem::create_directory_symlink(m_base / "root", m_base / "rootlink", ec);
    if (ec)
    {
        GTEST_SKIP() << "symbolic links cannot be created here";
    }
    jinja2::ConfinedFileSystem fs((m_base / "rootlink").string());
    auto stream = fs.OpenStream("sub/b.j2");
    ASSERT_TRUE(static_cast<bool>(stream));
    EXPECT_EQ("b", ReadFile(stream));
}

TEST_F(ConfinedFsTest, FifoIsNotOpened)
{
    if (::mkfifo((m_base / "root" / "pipe.j2").c_str(), 0600) != 0)
    {
        GTEST_SKIP() << "FIFOs cannot be created here";
    }
    jinja2::ConfinedFileSystem fs(m_root);
    EXPECT_FALSE(fs.OpenStream("pipe.j2"));
    EXPECT_FALSE(fs.OpenWStream("pipe.j2"));
    EXPECT_FALSE(fs.GetLastModificationDate("pipe.j2").has_value());
}

TEST_F(ConfinedFsTest, WideStreamDecodesUtf8AcrossChunks)
{
    // Multi-byte characters straddle the internal chunk boundaries at many offsets
    std::string content;
    std::wstring expected;
    for (int i = 0; i < 5000; ++i)
    {
        content += "x\xD0\xB6\xE2\x82\xAC";
        expected += L"xж€";
    }
    content += "\xE2\x82"; // an incomplete sequence at the very end
    expected += L"�";
    WriteFile(m_base / "root" / "u.j2", content);

    jinja2::ConfinedFileSystem fs(m_root);
    auto wide = fs.OpenWStream("u.j2");
    ASSERT_TRUE(static_cast<bool>(wide));
    std::wstring got((std::istreambuf_iterator<wchar_t>(*wide)), std::istreambuf_iterator<wchar_t>());
    EXPECT_EQ(expected, got);
    auto narrow = fs.OpenStream("u.j2");
    EXPECT_EQ(content, ReadFile(narrow));
}

TEST_F(ConfinedFsTest, SwappingALinkNeverReachesOutside)
{
    ASSERT_TRUE(Link("l", "a.j2"));
    jinja2::ConfinedFileSystem fs(m_root);
    std::atomic<bool> stop{ false };
    std::thread swapper(
        [&] {
            const auto tmp = m_base / "root" / "l.tmp";
            const auto target = m_base / "root" / "l";
            bool outside = true;
            while (!stop.load())
            {
                std::error_code ec;
                std::filesystem::remove(tmp, ec);
                std::filesystem::create_symlink(outside ? "../outside/canary.j2" : "a.j2", tmp, ec);
                if (!ec)
                {
                    std::filesystem::rename(tmp, target, ec);
                }
                outside = !outside;
            }
        });

    int found = 0;
    for (int i = 0; i < 2000; ++i)
    {
        auto stream = fs.OpenStream("l");
        if (!stream)
        {
            continue;
        }
        ++found;
        EXPECT_NE("CANARY", ReadFile(stream));
    }
    stop = true;
    swapper.join();
    EXPECT_GT(found, 0);
}

#endif // !_WIN32
