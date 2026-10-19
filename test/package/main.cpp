// Built against the installed package by check_package.cmake (docs/tasks/0068)
// The umbrella header is installed and self-contained (docs/tasks/0076)
#include <jinja2cpp/jinja2cpp.h>

#include <exception>
#include <iostream>
#include <memory>
#include <string>

// The library's choices reach the consumer through the package
static_assert(nsel_USES_STD_EXPECTED == 0, "Result<T> must be expected-lite's own type at every standard");
#if defined(EXPECT_SHARED) != defined(JINJA2CPP_LINK_AS_SHARED)
#error "JINJA2CPP_LINK_AS_SHARED does not match the installed library's link type"
#endif
// The generated version.h is installed with the headers (docs/tasks/0069)
static_assert(JINJA2CPP_VERSION >= 10302, "jinja2cpp/version.h is stale or missing");

namespace
{
int Run(const char* templateDir)
{
    jinja2::TemplateEnv env;
    env.AddFilesystemHandler(std::string(), std::make_shared<jinja2::RealFileSystem>(templateDir));
    jinja2::Template tpl(&env);
    // tojson: the JSON binding; format: fmt; the file loader: Boost.Filesystem
    auto loaded = tpl.Load("{% include 'hello.j2' %} {{ x | tojson }} {{ 'a%sb' | format(y) }}");
    if (!loaded)
    {
        std::cerr << loaded.error() << "\n";
        return 1;
    }
    auto out = tpl.RenderAsString({ { "name", "pkg" }, { "x", jinja2::ValuesList{ 1, "s" } }, { "y", 42 } });
    if (!out)
    {
        std::cerr << out.error() << "\n";
        return 1;
    }
    std::cout << out.value() << "\n";
    return out.value() == "Hello pkg! [1, \"s\"] a42b" ? 0 : 2;
}
} // namespace

int main(int argc, char** argv)
{
    if (argc < 2)
    {
        return 3;
    }
    try
    {
        return Run(argv[1]);
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << "\n";
        return 4;
    }
}
