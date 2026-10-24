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
// The generated version.h is installed with the headers (docs/tasks/0069), and its major
// version is the one in the inline ABI namespace v2 (docs/tasks/0076)
static_assert(JINJA2CPP_VERSION_MAJOR == 2, "jinja2cpp/version.h is stale or missing, or the ABI namespace tag needs a new major");

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
    if (out.value() != "Hello pkg! [1, \"s\"] a42b")
    {
        return 2;
    }
    // The built-in big-integer backend matches jinja2cpp_BIGINT (docs/tasks/0272)
    const auto* backend = jinja2::GetBuiltinBigIntBackend();
    jinja2::Template big;
    if (!big.Load("{{ 2 ** 64 }}"))
    {
        return 5;
    }
    const auto product = big.RenderAsString({});
#ifdef EXPECT_BIGINT_BOOST
    const bool asBuilt = backend != nullptr && std::string(backend->name) == "boost-cpp_int" && product.has_value() && product.value() == "18446744073709551616";
#else
    const bool asBuilt = backend == nullptr && !product.has_value() && product.error().GetCode() == jinja2::ErrorCode::IntegerOutOfRange;
#endif
    if (!asBuilt)
    {
        std::cerr << "big-integer backend " << (backend ? backend->name : "none") << " does not match the package\n";
        return 6;
    }
    return 0;
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
