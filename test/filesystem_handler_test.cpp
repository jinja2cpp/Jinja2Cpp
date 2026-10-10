#include <jinja2cpp/filesystem_handler.h>
#include <jinja2cpp/template_env.h>
#include <jinja2cpp/value.h>

#include <gtest/gtest.h>

#include <chrono>
#include <cstddef>
#include <fstream>
#include <map>
#include <optional>
#include <string>
#include <thread>
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
