#include "expression_evaluator.h"
#include "internal_value.h"
#include "markup.h"
#include "python_format.h"
#include "undefined.h"
#include "value_visitors.h"

#include <jinja2cpp/string_helpers.h>
#include <jinja2cpp/template_env.h>
#include <jinja2cpp/value.h>

#include <cctype>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <memory>
#include <optional>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

// The default globals of a Jinja2 environment: range, dict, cycler, joiner, namespace and lipsum

using namespace std::string_literals;

namespace jinja2
{
namespace
{
// jinja2.constants.LOREM_IPSUM_WORDS
const char LoremIpsumText[] =
    "a ac accumsan ad adipiscing aenean aliquam aliquet amet ante aptent arcu at auctor augue "
    "bibendum blandit class commodo condimentum congue consectetuer consequat conubia convallis "
    "cras cubilia cum curabitur curae cursus dapibus diam dictum dictumst dignissim dis dolor "
    "donec dui duis egestas eget eleifend elementum elit enim erat eros est et etiam eu euismod "
    "facilisi facilisis fames faucibus felis fermentum feugiat fringilla fusce gravida habitant "
    "habitasse hac hendrerit hymenaeos iaculis id imperdiet in inceptos integer interdum ipsum "
    "justo lacinia lacus laoreet lectus leo libero ligula litora lobortis lorem luctus maecenas "
    "magna magnis malesuada massa mattis mauris metus mi molestie mollis montes morbi mus nam "
    "nascetur natoque nec neque netus nibh nisi nisl non nonummy nostra nulla nullam nunc odio "
    "orci ornare parturient pede pellentesque penatibus per pharetra phasellus placerat platea "
    "porta porttitor posuere potenti praesent pretium primis proin pulvinar purus quam quis "
    "quisque rhoncus ridiculus risus rutrum sagittis sapien scelerisque sed sem semper senectus "
    "sit sociis sociosqu sodales sollicitudin suscipit suspendisse taciti tellus tempor tempus "
    "tincidunt torquent tortor tristique turpis ullamcorper ultrices ultricies urna ut varius "
    "vehicula vel velit venenatis vestibulum vitae vivamus viverra volutpat vulputate";

const std::vector<std::string>& LoremIpsumWords()
{
    static const std::vector<std::string> words = [] {
        std::vector<std::string> result;
        std::istringstream stream(LoremIpsumText);
        for (std::string word; stream >> word;)
        {
            result.push_back(word);
        }
        return result;
    }();
    return words;
}

Callable MakeFunction(Callable::ExpressionCallable fn)
{
    return Callable(Callable::GlobalFunc, std::move(fn));
}

ParsedArguments ParseArgs(const std::initializer_list<ArgumentInfo>& argsInfo, const CallParams& params, const char* fnName)
{
    bool isSucceeded = true;
    auto args = helpers::ParseCallParams(argsInfo, params, isSucceeded);
    if (!isSucceeded || !args.extraPosArgs.empty() || !args.extraKwArgs.empty())
    {
        throw std::runtime_error(std::string(fnName) + "() got unexpected arguments");
    }
    return args;
}

std::string KeyToString(const InternalValue& key)
{
    if (GetIf<std::string>(&key))
    {
        return AsString(key);
    }

    // Mapping keys are strings (task 0036): store other keys by their printed form
    std::string result;
    Apply<visitors::ValueRenderer<char>>(key, result);
    return result;
}

// range([start,] stop[, step]); unlike Python, the arguments can also be passed by name
InternalValue CallRange(const CallParams& params, RenderContext&)
{
    // Unlike Python, a call without stop renders empty instead of failing (forloop_test)
    bool isSucceeded = true;
    auto args = helpers::ParseCallParams({ { "start" }, { "stop", true }, { "step" } }, params, isSucceeded);
    if (!isSucceeded)
    {
        return InternalValue();
    }
    if (!args.extraPosArgs.empty() || !args.extraKwArgs.empty())
    {
        throw std::runtime_error("range expected at most 3 arguments");
    }
    int64_t start = ConvertToInt(args["start"]);
    int64_t stop = ConvertToInt(args["stop"]);
    int64_t step = IsEmpty(args["step"]) ? 1 : ConvertToInt(args["step"]);
    if (step == 0)
    {
        throw std::runtime_error("range() arg 3 must not be zero");
    }

    return ListAdapter::CreateRange(start, stop, step);
}

// The items of dict(mapping_or_pairs, **kwargs), into `result`
template<typename Map>
void CollectDictItems(const CallParams& params, const char* fnName, Map& result)
{
    if (params.posParams.size() > 1)
    {
        throw std::runtime_error(std::string(fnName) + " expected at most 1 argument, got " + std::to_string(params.posParams.size()));
    }

    if (!params.posParams.empty())
    {
        const auto& source = params.posParams.front();
        if (const auto* map = GetIf<MapAdapter>(&source))
        {
            for (auto& key : map->GetKeys())
            {
                result[key] = map->GetValueByName(key);
            }
        }
        else if (const auto* list = GetIf<ListAdapter>(&source))
        {
            for (const auto& item : *list)
            {
                if (const auto* pair = GetIf<KeyValuePair>(&item))
                {
                    result[pair->key] = pair->value;
                    continue;
                }
                // Any iterable of two items is a pair, a two-character string included
                bool isConverted = false;
                auto itemList = ConvertToList(item, isConverted, false);
                if (!isConverted)
                {
                    throw std::runtime_error("cannot convert dictionary update sequence element to a sequence");
                }
                auto pair = itemList.ToValueList();
                if (pair.size() != 2)
                {
                    throw std::runtime_error("dictionary update sequence element has wrong length; 2 is required");
                }
                result[KeyToString(pair[0])] = pair[1];
            }
        }
        else if (!IsEmpty(source))
        {
            throw std::runtime_error("dict() argument is not a mapping or a sequence of pairs");
        }
    }

    for (const auto& [name, value] : params.kwParams)
    {
        result[name] = value;
    }
}

// dict(mapping_or_pairs, **kwargs)
InternalValue CallDict(const CallParams& params, RenderContext&)
{
    InternalValueMap result;
    CollectDictItems(params, "dict", result);
    return CreateMapAdapter(std::move(result));
}

// namespace(mapping_or_pairs, **kwargs): takes its arguments as dict() does
InternalValue CallNamespace(const CallParams& params, RenderContext&)
{
    InternalDict result;
    CollectDictItems(params, "namespace", result);
    return CreateNamespaceAdapter(std::move(result));
}

// cycler(*items): next(), reset(), current, items, pos
InternalValue CallCycler(const CallParams& params, RenderContext&)
{
    if (!params.kwParams.empty())
    {
        throw std::runtime_error("cycler() got an unexpected keyword argument '" + params.kwParams.begin()->first + "'");
    }
    if (params.posParams.empty())
    {
        throw std::runtime_error("at least one item has to be provided");
    }

    struct State
    {
        InternalValueList items;
        size_t pos = 0;
    };
    auto state = std::make_shared<State>();
    state->items = params.posParams;

    InternalValueMap cycler;
    cycler["items"] = ListAdapter::CreateAdapter(InternalValueList(state->items)).MarkAsTuple();
    cycler["pos"] = MakeDynamicProperty([state](const CallParams&, RenderContext&) { return InternalValue(static_cast<int64_t>(state->pos)); });
    cycler["current"] = MakeDynamicProperty([state](const CallParams&, RenderContext&) { return state->items[state->pos]; });
    cycler["next"] = MakeFunction([state](const CallParams&, RenderContext&) {
        auto result = state->items[state->pos];
        state->pos = (state->pos + 1) % state->items.size();
        return result;
    });
    cycler["reset"] = MakeFunction([state](const CallParams&, RenderContext&) {
        state->pos = 0;
        return InternalValue(EmptyValue());
    });
    return CreateMapAdapter(std::move(cycler));
}

// joiner(sep=', '): a callable that returns '' the first time, then sep
InternalValue CallJoiner(const CallParams& params, RenderContext&)
{
    auto args = ParseArgs({ { "sep", false, ", "s } }, params, "joiner");
    auto used = std::make_shared<bool>(false);
    InternalValueMap joiner;
    joiner["operator()"] = MakeFunction([used, sep = args["sep"]](const CallParams&, RenderContext&) {
        if (*used)
        {
            return sep;
        }
        *used = true;
        return InternalValue(std::string());
    });
    return CreateMapAdapter(std::move(joiner));
}

std::string Capitalize(std::string word)
{
    if (!word.empty())
    {
        word[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(word[0])));
    }
    return word;
}

// Unsigned arithmetic: to - from overflows int64_t for the widest bounds
int64_t LipsumRandRange(std::minstd_rand& random, int64_t from, int64_t to)
{
    auto span = static_cast<uint64_t>(to) - static_cast<uint64_t>(from);
    return static_cast<int64_t>(static_cast<uint64_t>(from) + (random() % span));
}

// A random lorem ipsum word other than `last`
const std::string* PickLipsumWord(std::minstd_rand& random, const std::string* last)
{
    const auto& lorem = LoremIpsumWords();
    const std::string* picked = nullptr;
    do
    {
        picked = &lorem[random() % lorem.size()];
    } while (picked == last);
    return picked;
}

// One paragraph of minWords to maxWords words, with commas and full stops at random
std::string LipsumParagraph(std::minstd_rand& random, int64_t minWords, int64_t maxWords)
{
    bool nextCapitalized = true;
    int64_t lastComma = 0;
    int64_t lastFullstop = 0;
    const std::string* last = nullptr;
    std::string text;

    auto words = LipsumRandRange(random, minWords, maxWords);
    for (int64_t idx = 0; idx < words; ++idx)
    {
        last = PickLipsumWord(random, last);

        std::string word = *last;
        if (nextCapitalized)
        {
            word = Capitalize(std::move(word));
            nextCapitalized = false;
        }
        if (idx - LipsumRandRange(random, 3, 8) > lastComma)
        {
            lastComma = idx;
            lastFullstop += 2;
            word += ',';
        }
        if (idx - LipsumRandRange(random, 10, 20) > lastFullstop)
        {
            lastComma = lastFullstop = idx;
            word += '.';
            nextCapitalized = true;
        }
        if (!text.empty())
        {
            text += ' ';
        }
        text += word;
    }

    if (!text.empty() && text.back() == ',')
    {
        text.back() = '.';
    }
    else if (text.empty() || text.back() != '.')
    {
        text += '.';
    }
    return text;
}

// lipsum(n=5, html=True, min=20, max=100): the algorithm of jinja2.utils.generate_lorem_ipsum.
// The text is random there too, so only its shape is comparable; the generator is seeded
// per render so the output is reproducible.
InternalValue CallLipsum(const CallParams& params, std::minstd_rand& random)
{
    auto args = ParseArgs({ { "n", false, static_cast<int64_t>(5) }, { "html", false, true }, { "min", false, static_cast<int64_t>(20) }, { "max", false, static_cast<int64_t>(100) } },
                          params,
                          "lipsum");
    auto count = ConvertToInt(args["n"]);
    bool html = ConvertToBool(args["html"]);
    auto minWords = ConvertToInt(args["min"]);
    auto maxWords = ConvertToInt(args["max"]);
    if (minWords >= maxWords)
    {
        throw std::runtime_error("lipsum(): empty range for the number of words");
    }

    std::string result;
    for (int64_t paragraph = 0; paragraph < count; ++paragraph)
    {
        auto text = LipsumParagraph(random, minWords, maxWords);
        if (!result.empty())
        {
            result += html ? "\n" : "\n\n";
        }
        result += html ? "<p>" + text + "</p>" : text;
    }
    return InternalValue(std::move(result));
}

// The gettext functions of jinja2.ext.i18n, newstyle: (context, singular, plural, count) as the
// function takes them, then the format variables as keyword arguments
struct GettextFunction
{
    const char* name;
    bool hasContext;
    bool hasPlural;
};

// The count of ngettext: null translations pick the singular form when n == 1
bool IsOne(const InternalValue& n)
{
    if (const auto* i = GetIf<int64_t>(&n))
    {
        return *i == 1;
    }
    if (const auto* d = GetIf<double>(&n))
    {
        return *d == 1.0;
    }
    if (const auto* b = GetIf<bool>(&n))
    {
        return *b;
    }
    return false;
}

// _make_new_gettext and friends: translates the message, then formats it with the keyword
// arguments (`num` defaults to the count); under autoescape the result is Markup and the
// variables are escaped
InternalValue CallGettext(const GettextFunction& fn, const CallParams& params, RenderContext& context)
{
    size_t argsCount = 1U + (fn.hasContext ? 1U : 0U) + (fn.hasPlural ? 2U : 0U);
    if (params.posParams.size() != argsCount)
    {
        throw std::runtime_error(std::string(fn.name) + "() takes " + std::to_string(argsCount) + " positional arguments but " + std::to_string(params.posParams.size()) + " were given");
    }

    InternalDict variables = params.kwParams;

    InternalValue translated;
    auto* env = context.GetEnv();
    auto userFn = env ? env->FindGettextCallable(fn.name) : std::optional<UserCallable>();
    if (userFn)
    {
        auto callable = visitors::InputValueConvertor::ConvertUserCallable(*userFn);
        CallParams rawParams;
        rawParams.posParams = params.posParams;
        translated = GetIf<Callable>(&callable)->GetExpressionCallable()(rawParams, context);
    }
    else
    {
        size_t message = fn.hasContext ? 1 : 0;
        translated = fn.hasPlural && !IsOne(params.posParams[message + 2]) ? params.posParams[message + 1] : params.posParams[message];
    }
    if (fn.hasPlural)
    {
        variables.try_emplace("num", params.posParams[argsCount - 1]);
    }

    auto* callback = context.GetRendererCallback();
    bool isWide = false;
    bool isString = ApplyStringConverter(translated, [&isWide](auto str) {
        isWide = sizeof(str[0]) != sizeof(char);
        return true;
    });
    if (!isString)
    {
        throw std::runtime_error("unsupported operand type(s) for %: '"s + Apply<visitors::PythonTypeNameGetter>(translated) + "' and 'dict'");
    }

    InternalValue values = CreateMapAdapter(std::move(variables));
    if (context.IsAutoescape())
    {
        values = EscapeFormatArgs(values, callback);
    }
    auto formatted = PythonPercentFormat(ApplyStringConverter(translated, [](auto str) { return ConvertString<std::string>(str); }), values);
    InternalValue result = isWide ? TargetString(ConvertString<std::wstring>(formatted)) : TargetString(std::move(formatted));
    result.SetMarkup(context.IsAutoescape());
    return result;
}

// jinja2.ext._gettext_alias: `_` calls whatever `gettext` is at the call site
InternalValue CallGettextAlias(const CallParams& params, RenderContext& context)
{
    bool found = false;
    const auto* gettext = context.FindValue("gettext", found);
    return CallExpression::CallValue(context, found ? gettext->second : MakeUndefined(context, "gettext"), params);
}
} // namespace

namespace
{
void SetupI18nGlobals(InternalValueMap& globalParams)
{
    static const GettextFunction functions[] = { { "gettext", false, false }, { "ngettext", false, true }, { "pgettext", true, false }, { "npgettext", true, true } };
    for (const auto& fn : functions)
    {
        globalParams.emplace(fn.name, MakeFunction([&fn](const CallParams& params, RenderContext& context) { return CallGettext(fn, params, context); }));
    }
    globalParams.emplace("_", MakeFunction(CallGettextAlias));
}

void SetupGlobals(InternalValueMap& globalParams)
{
    globalParams.emplace("range", MakeFunction(CallRange));
    globalParams.emplace("dict", MakeFunction(CallDict));
    globalParams.emplace("cycler", MakeFunction(CallCycler));
    globalParams.emplace("joiner", MakeFunction(CallJoiner));
    globalParams.emplace("namespace", MakeFunction(CallNamespace));
    globalParams.emplace("lipsum", MakeFunction([](const CallParams& params, RenderContext& context) {
                             // Every render has a callback (TemplateImpl::Render)
                             return CallLipsum(params, context.GetRendererCallback()->GetRandomEngine());
                         }));
}
} // namespace

const InternalValueMap& GetBuiltinGlobals(bool withI18n)
{
    // Built once and only read afterwards, so renders on any thread share them; the state of
    // the stateful globals (cycler, joiner, lipsum's generator) lives in their results or in
    // the render
    static const auto globals = [] {
        InternalValueMap result;
        SetupGlobals(result);
        return result;
    }();
    static const auto globalsWithI18n = [] {
        InternalValueMap result;
        SetupGlobals(result);
        SetupI18nGlobals(result);
        return result;
    }();
    return withI18n ? globalsWithI18n : globals;
}
} // namespace jinja2
