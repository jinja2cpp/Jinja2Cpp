// A translation unit written against the Jinja2C++ 1.x API, modelled on snippets of our tests
// (values, user callables, Settings, error info, string helpers). It uses every deprecated 1.x
// name at least once. scripts/jinja2cpp_migrate.py migrates a copy of it; the result must equal
// legacy_api_2x.cpp and build without a deprecation warning (docs/tasks/0077,
// test/migration/run_migration_test.py). Do not "fix" this file: the old names are the point.
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
    return value.As<T>();
}

template<typename T>
T FetchCopy(Value& value)
{
    return value.As<T>();
}

Settings MakeSettings()
{
    Settings settings;
    settings.defaultMetadataType = "json";
    settings.extensions.doStatement = true;
    settings.lineStatementPrefix = "#";
    return settings;
}

std::string MetadataType(const Settings* settings)
{
    return settings->defaultMetadataType;
}

// Two calls on one line, then a call split over several lines
bool IsText(const Value& value)
{
    return value.IsString() || value.IsWString();
}

std::size_t CountItems(const Value& value)
{
    const auto& items = value
                            .AsList();
    return items.size();
}

UserCallable::Function MakeGreeter()
{
    return [](const UserCallableParams& params) -> Value { return params["name"].AsString() + "!"; };
}

jinja2::UserCallable::Function MakeQualifiedGreeter()
{
    return MakeGreeter();
}

jinja2::Result<std::string> RenderText(const std::string& source)
{
    Template tpl;
    auto loaded = tpl.Load(source);
    if (!loaded)
    {
        return nonstd::make_unexpected(loaded.error());
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
    NoneValue noneMarker;
    Value none = noneMarker;
    const Value& constText = text;

    checks += text.IsString() ? 1 : 0;
    checks += wide.IsWString() ? 1 : 0;
    checks += list.IsList() ? 1 : 0;
    checks += map.IsMap() ? 1 : 0;
    checks += none.IsNone() ? 1 : 0;

    text.AsString() += "!";
    checks += static_cast<int>(constText.AsString().size());
    wide.AsWString() += L"!";
    checks += static_cast<int>(wide.AsWString().size());
    list.AsList().emplace_back(3);
    checks += static_cast<int>(list.AsList().size());
    map.AsMap()["n"] = 2;
    checks += static_cast<int>(map.AsMap().size());

    checks += static_cast<int>(Fetch<std::string>(text).size());
    checks += static_cast<int>(Fetch<int64_t>(Value(5)));
    checks += static_cast<int>(FetchCopy<std::string>(text).size());
    checks += static_cast<int>(text.As<std::string>().size());
    checks += text.GetIf<std::string>() ? 1 : 0;
    checks += constText.GetIf<int64_t>() ? 1 : 0;
    checks += static_cast<int>(text.GetData().index() + constText.GetData().index());

    checks += static_cast<int>(ToString(std::string("a")).size());
    checks += static_cast<int>(ToString(std::wstring(L"b")).size());
    checks += static_cast<int>(ToString(std::string_view("c")).size());
    checks += static_cast<int>(ToString(std::wstring_view(L"d")).size());
    checks += static_cast<int>(ToString(text).size());
    checks += static_cast<int>(ToWString(std::wstring(L"a")).size());
    checks += static_cast<int>(ToWString(std::string("b")).size());
    checks += static_cast<int>(ToWString(std::string_view("c")).size());
    checks += static_cast<int>(ToWString(std::wstring_view(L"d")).size());
    checks += static_cast<int>(ToWString(wide).size());

    checks += IsText(text) ? 1 : 0;
    checks += static_cast<int>(CountItems(list));

    UserCallable greeter;
    greeter.callable = MakeQualifiedGreeter();
    checks += greeter.callable ? 1 : 0;

    Settings settings = MakeSettings();
    checks += static_cast<int>(MetadataType(&settings).size());

    BasicErrorInfo<char> info;
    auto rendered = RenderText("{{ 1 + }}");
    if (!rendered)
    {
        info = rendered.error();
    }
    checks += info.GetCode() == ErrorCode::Unspecified ? 0 : 1;
    return checks;
}
