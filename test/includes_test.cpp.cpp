#include <iostream>
#include <string>

#include "test_tools.h"

// Test cases are taken from the pandor/Jinja2 tests

class IncludeTest : public TemplateEnvFixture
{
protected:
    void SetUp() override
    {
        TemplateEnvFixture::SetUp();
        
        AddFile("header", "[{{ foo }}|{{ 23 }}]");
        AddFile("o_printer", "({{ o }})");
    }
};

TEST_F(IncludeTest, TestContextInclude)
{
    jinja2::ValuesMap params{{"foo", 42}};
    
    auto result = Render(R"({% include "header" %})", params);
    EXPECT_EQ("[42|23]", result);
    result = Render(R"({% include "header" with context %})", params);
    EXPECT_EQ("[42|23]", result);
    result = Render(R"({% include "header" without context %})", params);
    EXPECT_EQ("[|23]", result);
    result = Render(R"({% include "header" ignore missing with context %})", params);
    EXPECT_EQ("[42|23]", result);
    result = Render(R"({% include "header" ignore missing without context %})", params);
    EXPECT_EQ("[|23]", result);
}

TEST_F(IncludeTest, TestChoiceIncludes)
{
    jinja2::ValuesMap params{{"foo", 42}};
    
    auto result = Render(R"({% include ["missing", "header"] %})", params);
    EXPECT_EQ("[42|23]", result);
    
    result = Render(R"({% include ["missing", "missing2"] ignore missing %})", params);
    EXPECT_EQ("", result);

#if 0
    t = test_env.from_string('{% include ["missing", "missing2"] %}')
    pytest.raises(TemplateNotFound, t.render)
    try:
        t.render()
    except TemplatesNotFound as e:
        assert e.templates == ['missing', 'missing2']
        assert e.name == 'missing2'
    else:
        assert False, 'thou shalt raise'

    def test_includes(t, **ctx):
        ctx['foo'] = 42
        assert t.render(ctx) == '[42|23]'

    t = test_env.from_string('{% include ["missing", "header"] %}')
    test_includes(t)
    t = test_env.from_string('{% include x %}')
    test_includes(t, x=['missing', 'header'])
    t = test_env.from_string('{% include [x, "header"] %}')
    test_includes(t, x='missing')
    t = test_env.from_string('{% include x %}')
    test_includes(t, x='header')
    t = test_env.from_string('{% include x %}')
    test_includes(t, x='header')
    t = test_env.from_string('{% include [x] %}')
    test_includes(t, x='header')
#endif
}
