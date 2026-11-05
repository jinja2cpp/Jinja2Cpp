// A translation unit written against the Jinja2C++ 1.x API, modelled on snippets of our tests
// (values, user callables, Settings, error info, string helpers). It uses every deprecated 1.x
// name at least once. scripts/jinja2cpp_migrate.py migrates a copy of it; the result must equal
// legacy_api_2x.cpp and build without a deprecation warning (docs/tasks/0077,
// test/migration/run_migration_test.py). Do not "fix" this file: the old names are the point.
#include <jinja2cpp/error_handler.h>
#include <jinja2cpp/error_info.h>
#include <jinja2cpp/string_helpers.h>
#include <jinja2cpp/template.h>
#include <jinja2cpp/template_env.h>
#include <jinja2cpp/user_callable.h>
#include <jinja2cpp/value.h>

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

using namespace jinja2;

namespace
{
// A call in a template is reported once per instantiation
template<typename T>
T Fetch(const Value& value)
{
    return value.get<T>();
}

template<typename T>
T FetchCopy(Value& value)
{
    return value.get<T>();
}

Settings MakeSettings()
{
    Settings settings;
    settings.m_defaultMetadataType = "json";
    settings.extensions.Do = true;
    settings.useLineStatements = true;
    return settings;
}

std::string MetadataType(const Settings* settings)
{
    return settings->m_defaultMetadataType;
}

// Two calls on one line, then a call split over several lines
bool IsText(const Value& value)
{
    return value.isString() || value.isWString();
}

std::size_t CountItems(const Value& value)
{
    const auto& items = value
                            .asList();
    return items.size();
}

UserCallable::UserCallableFunctionPtr MakeGreeter()
{
    return [](const UserCallableParams& params) -> Value { return params["name"].asString() + "!"; };
}

jinja2::UserCallable::UserCallableFunctionPtr MakeQualifiedGreeter()
{
    return MakeGreeter();
}

nonstd::expected<std::string, jinja2::ErrorInfo> RenderText(const std::string& source)
{
    Template tpl;
    auto loaded = tpl.Load(source);
    if (!loaded)
    {
        return loaded.get_unexpected();
    }
    return tpl.RenderAsString(ValuesMap{});
}
} // namespace

int RunLegacyApi()
{
    int checks = 0;

    Value text("text");
    Value wide(L"wide");
    Value list(ValuesList{ 1, 2 });
    Value map(ValuesMap{ { "k", 1 } });
    EmptyValue noneMarker;
    Value none = noneMarker;
    const Value& constText = text;

    checks += text.isString() ? 1 : 0;
    checks += wide.isWString() ? 1 : 0;
    checks += list.isList() ? 1 : 0;
    checks += map.isMap() ? 1 : 0;
    checks += none.isEmpty() ? 1 : 0;

    text.asString() += "!";
    checks += static_cast<int>(constText.asString().size());
    wide.asWString() += L"!";
    checks += static_cast<int>(wide.asWString().size());
    list.asList().emplace_back(3);
    checks += static_cast<int>(list.asList().size());
    map.asMap()["n"] = 2;
    checks += static_cast<int>(map.asMap().size());

    checks += static_cast<int>(Fetch<std::string>(text).size());
    checks += static_cast<int>(Fetch<int64_t>(Value(5)));
    checks += static_cast<int>(FetchCopy<std::string>(text).size());
    checks += static_cast<int>(text.get<std::string>().size());
    checks += text.getPtr<std::string>() ? 1 : 0;
    checks += constText.getPtr<int64_t>() ? 1 : 0;
    checks += static_cast<int>(text.data().index() + constText.data().index());

    checks += static_cast<int>(AsString(std::string("a")).size());
    checks += static_cast<int>(AsString(std::wstring(L"b")).size());
    checks += static_cast<int>(AsString(std::string_view("c")).size());
    checks += static_cast<int>(AsString(std::wstring_view(L"d")).size());
    checks += static_cast<int>(AsString(text).size());
    checks += static_cast<int>(AsWString(std::wstring(L"a")).size());
    checks += static_cast<int>(AsWString(std::string("b")).size());
    checks += static_cast<int>(AsWString(std::string_view("c")).size());
    checks += static_cast<int>(AsWString(std::wstring_view(L"d")).size());
    checks += static_cast<int>(AsWString(wide).size());

    checks += IsText(text) ? 1 : 0;
    checks += static_cast<int>(CountItems(list));

    UserCallable greeter;
    greeter.callable = MakeQualifiedGreeter();
    checks += greeter.callable ? 1 : 0;

    Settings settings = MakeSettings();
    checks += static_cast<int>(MetadataType(&settings).size());

    ErrorInfoTpl<char> info;
    auto rendered = RenderText("{{ 1 + }}");
    if (!rendered)
    {
        info = rendered.error();
    }
    checks += info.GetCode() == ErrorCode::Unspecified ? 0 : 1;
    return checks;
}
