// Built by check_package.cmake against one installed component (docs/tasks/0311), picked by
// CONSUMER_<COMPONENT>: the component brings its header and its library's include path
#include <jinja2cpp/template.h>
#include <jinja2cpp/value.h>

#if defined(CONSUMER_NLOHMANN_JSON)
#include <jinja2cpp/binding/nlohmann_json.h>
#elif defined(CONSUMER_BOOST_JSON)
#include <jinja2cpp/binding/boost_json.h>
#elif defined(CONSUMER_BOOST_MULTIPRECISION)
#include <jinja2cpp/big_int/boost_cpp_int.h>
#include <jinja2cpp/template_env.h>
#else
#error "no CONSUMER_<COMPONENT> defined"
#endif

#include <iostream>
#include <string>

int main()
{
#if defined(CONSUMER_BOOST_MULTIPRECISION)
    jinja2::TemplateEnv env;
    env.SetBigIntBackend(jinja2::BoostCppIntBackend());
    jinja2::Template tpl(&env);
    if (!tpl.Load("{{ 2 ** 70 }}"))
    {
        return 1;
    }
    auto out = tpl.RenderAsString({});
    const std::string expected = "1180591620717411303424";
#else
#if defined(CONSUMER_NLOHMANN_JSON)
    const auto data = nlohmann::json::parse(R"({"name": "json", "items": [1, 2, 3]})");
#else
    const auto data = boost::json::parse(R"({"name": "json", "items": [1, 2, 3]})");
#endif
    jinja2::Template tpl;
    if (!tpl.Load("{{ j.name }}: {{ j['items'] | sum }}"))
    {
        return 1;
    }
    auto copy = jinja2::ToValue(data);
    if (!copy)
    {
        return 2;
    }
    auto outCopy = tpl.RenderAsString({ { "j", *copy } });
    auto out = tpl.RenderAsString({ { "j", jinja2::Reflect(data) } });
    if (!outCopy || !out || outCopy.value() != out.value())
    {
        return 3;
    }
    const std::string expected = "json: 6";
#endif
    if (!out)
    {
        return 4;
    }
    std::cout << out.value() << "\n";
    return out.value() == expected ? 0 : 5;
}
