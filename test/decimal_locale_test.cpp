#include "test_tools.h"

#include <jinja2cpp/template.h>
#include <jinja2cpp/value.h>

#include <clocale>
#include <cmath>
#include <string>

// Decimal numbers are read with '.' as the decimal point whatever LC_NUMERIC says
// (docs/tasks/0333). A host that calls setlocale(LC_ALL, "") under de_DE used to get 1.5
// read as 1; the fixture switches to a decimal-comma locale and skips without one
class DecimalCommaLocaleTest : public BasicTemplateRenderer
{
protected:
    void SetUp() override
    {
        const char* current = std::setlocale(LC_NUMERIC, nullptr);
        m_saved = current ? current : "C";
        for (const char* name : { "de_DE.UTF-8", "de_DE.utf8", "fr_FR.UTF-8", "ru_RU.UTF-8", "de-DE", "German_Germany.1252" })
        {
            if (std::setlocale(LC_NUMERIC, name) && std::string(std::localeconv()->decimal_point) == ",")
            {
                return;
            }
        }
        std::setlocale(LC_NUMERIC, m_saved.c_str());
        GTEST_SKIP() << "no decimal-comma locale is installed";
    }

    void TearDown() override { std::setlocale(LC_NUMERIC, m_saved.c_str()); }

private:
    std::string m_saved;
};

MULTISTR_TEST(DecimalCommaLocaleTest, FloatLiterals, "{{ 1.5 }}|{{ 1_000.25 }}|{{ 1e400 }}|{{ 2.5e-3 }}|{{ 0.1 + 0.2 }}", "1.5|1000.25|inf|0.0025|0.30000000000000004")
{
}

MULTISTR_TEST(DecimalCommaLocaleTest,
              FloatAndRoundFilters,
              "{{ '2.5'|float }}|{{ ' -1_0.5e1 '|float }}|{{ '1e999'|float }}|{{ '1,5'|float }}|{{ 2.675|round(2) }}|{{ 1234.5|round(-2) }}|{{ 0.125|round(2, 'floor') "
              "}}|{{ 18446744073709551615|float }}",
              "2.5|-105.0|inf|0.0|2.67|1200.0|0.12|1.8446744073709552e+19")
{
}

TEST_F(DecimalCommaLocaleTest, MetadataNumbers)
{
    jinja2::Template tpl;
    ASSERT_TRUE(tpl.Load(R"({% meta %}{"x": 1.25, "y": -2.5e2, "z": 1e999}{% endmeta %})").has_value());
    auto metadata = tpl.GetMetadata();
    ASSERT_TRUE(metadata.has_value());
    EXPECT_EQ(1.25, metadata.value()["x"].As<double>());
    EXPECT_EQ(-250.0, metadata.value()["y"].As<double>());
    EXPECT_TRUE(std::isinf(metadata.value()["z"].As<double>()));
}
