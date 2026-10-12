// Built against the installed package by check_package.cmake (docs/tasks/0068)
#include <jinja2cpp/filesystem_handler.h>
#include <jinja2cpp/template.h>
#include <jinja2cpp/template_env.h>

#include <iostream>
#include <memory>
#include <string>

// The library's choices reach the consumer through the package
static_assert(!nsel_USES_STD_EXPECTED, "Result<T> must be expected-lite's own type at every standard");
#if defined(EXPECT_SHARED) != defined(JINJA2CPP_LINK_AS_SHARED)
#error "JINJA2CPP_LINK_AS_SHARED does not match the installed library's link type"
#endif

int main(int argc, char** argv)
{
    if (argc < 2)
    {
        return 3;
    }
    jinja2::TemplateEnv env;
    env.AddFilesystemHandler(std::string(), std::make_shared<jinja2::RealFileSystem>(argv[1]));
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
