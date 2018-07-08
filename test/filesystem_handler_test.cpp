#include <gtest/gtest.h>

#include <jinja2cpp/filesystem_handler.h>

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
            if (readSize < buffSize)
                break;
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
    EXPECT_FALSE((bool)testStream);
    auto test1Stream = fs.OpenStream("test1.j2tpl");
    EXPECT_TRUE((bool)test1Stream);
    EXPECT_EQ(test1Content, ReadFile(test1Stream));
    auto test2Stream = fs.OpenStream("test2.j2tpl");
    EXPECT_TRUE((bool)test2Stream);
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
    EXPECT_FALSE((bool)testStream);
    auto test1Stream = fs.OpenWStream("test1.j2tpl");
    EXPECT_TRUE((bool)test1Stream);
    EXPECT_EQ(test1Content, ReadFile(test1Stream));
    auto test2Stream = fs.OpenWStream("test2.j2tpl");
    EXPECT_TRUE((bool)test2Stream);
    EXPECT_EQ(test2Content, ReadFile(test2Stream));
}

TEST_F(FilesystemHandlerTest, RealFS_NarrowReading)
{
    const std::string test1Content =
R"(Hello World!
)";
    jinja2::RealFileSystem fs;
    auto testStream = fs.OpenStream("===incorrect====.j2tpl");
    EXPECT_FALSE((bool)testStream);
    auto test1Stream = fs.OpenStream("test_data/simple_template1.j2tpl");
    EXPECT_TRUE((bool)test1Stream);
    EXPECT_EQ(test1Content, ReadFile(test1Stream));
}

TEST_F(FilesystemHandlerTest, RealFS_RootHandling)
{
    const std::string test1Content =
R"(Hello World!
)";
    jinja2::RealFileSystem fs;

    auto test1Stream = fs.OpenStream("test_data/simple_template1.j2tpl");
    EXPECT_TRUE((bool)test1Stream);
    EXPECT_EQ(test1Content, ReadFile(test1Stream));
    fs.SetRootFolder("./test_data");
    auto test2Stream = fs.OpenStream("simple_template1.j2tpl");
    EXPECT_TRUE((bool)test2Stream);
    EXPECT_EQ(test1Content, ReadFile(test2Stream));
    fs.SetRootFolder("./test_data/");
    auto test3Stream = fs.OpenStream("simple_template1.j2tpl");
    EXPECT_TRUE((bool)test3Stream);
    EXPECT_EQ(test1Content, ReadFile(test3Stream));
}

TEST_F(FilesystemHandlerTest, RealFS_WideReading)
{
    const std::wstring test1Content =
LR"(Hello World!
)";
    jinja2::RealFileSystem fs;
    auto testStream = fs.OpenWStream("===incorrect====.j2tpl");
    EXPECT_FALSE((bool)testStream);
    auto test1Stream = fs.OpenWStream("test_data/simple_template1.j2tpl");
    EXPECT_TRUE((bool)test1Stream);
    EXPECT_EQ(test1Content, ReadFile(test1Stream));
}
