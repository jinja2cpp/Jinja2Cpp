#include "call_stop.h"
#include "engine_errors.h"
#include "expression_evaluator.h"
#include "filters.h"
#include "function_base.h"
#include "host_object.h"
#include "internal_value.h"
#include "markup.h"
#include "python_format.h"
#include "recursion_guard.h"
#include "render_context.h"
#include "template_dict.h"
#include "template_env_impl.h"
#include "undefined.h"
#include "value_visitors.h"

#include <cctype>
#include <fmt/format.h>
#include <jinja2cpp/string_helpers.h>
#include <jinja2cpp/template_env.h>
#include <jinja2cpp/value.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <memory>
#include <optional>
#include <random>
#include <sstream>
#include <string>
#include <utility>
#include <string_view>
#include <vector>

// The default globals of a Jinja2 environment: range, dict, cycler, joiner, namespace and lipsum

using namespace std::string_literals;

namespace jinja2
{
inline namespace v2
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

// A global that is a class in Jinja2, printed as Python prints the class (docs/tasks/0305)
Callable MakeClass(Callable::ExpressionCallable fn, const char* repr)
{
    auto result = MakeFunction(std::move(fn));
    result.SetRepr(repr);
    return result;
}

ParsedArguments ParseArgs(const std::initializer_list<ArgumentInfo>& argsInfo, const CallParams& params, const char* fnName)
{
    bool isSucceeded = true;
    auto args = helpers::ParseCallParams(argsInfo, params, isSucceeded);
    if (!isSucceeded || !args.extraPosArgs.empty() || !args.extraKwArgs.empty())
    {
        throw TemplateRuntimeError(std::string(fnName) + "() got unexpected arguments");
    }
    return args;
}

// Python's range and randrange take integers (bool among them) and nothing else; a plain
// undefined is a missing argument, left to the caller
void CheckIntegerArgument(const InternalValue& arg)
{
    if (const auto* info = GetUndefinedInfo(arg))
    {
        throw TemplateRuntimeError(std::string("'") + UndefinedClassName(*info) + "' object cannot be interpreted as an integer");
    }
    // An int outside int64 is an integer, which ConvertToInt then rejects with OverflowError
    if (!arg.IsUndefined() && !GetIf<int64_t>(&arg) && !GetIf<bool>(&arg) && arg.Kind() != ValueKind::BigInt)
    {
        throw TemplateRuntimeError(std::string("'") + Apply<visitors::PythonTypeNameGetter>(arg) + "' object cannot be interpreted as an integer");
    }
}

// range([start,] stop[, step])
InternalValue CallRange(const CallParams& params, RenderContext&)
{
    if (!params.kwParams.empty())
    {
        throw TemplateRuntimeError("range() takes no keyword arguments");
    }
    // Unlike Python, a call without stop renders empty instead of failing (forloop_test)
    bool isSucceeded = true;
    auto args = helpers::ParseCallParams({ { "start" }, { "stop", true }, { "step" } }, params, isSucceeded);
    if (!isSucceeded)
    {
        return InternalValue();
    }
    if (!args.extraPosArgs.empty() || !args.extraKwArgs.empty())
    {
        throw TemplateRuntimeError("range expected at most 3 arguments");
    }
    // A missing argument is a plain undefined. Each is looked up once (docs/tasks/0286)
    const auto startArg = args["start"];
    const auto stopArg = args["stop"];
    const auto stepArg = args["step"];
    for (const auto* arg : { &startArg, &stopArg, &stepArg })
    {
        CheckIntegerArgument(*arg);
    }
    int64_t start = ConvertToInt(startArg);
    int64_t stop = ConvertToInt(stopArg);
    int64_t step = IsEmpty(stepArg) ? 1 : ConvertToInt(stepArg);
    if (step == 0)
    {
        throw TemplateRuntimeError("range() arg 3 must not be zero");
    }
    // Its length, as Python counts it for the sandbox's MAX_RANGE
    const auto span = [](int64_t from, int64_t to) { return static_cast<uint64_t>(to) - static_cast<uint64_t>(from); };
    uint64_t length = 0;
    if (step > 0 && start < stop)
    {
        length = ((span(start, stop) - 1) / static_cast<uint64_t>(step)) + 1;
    }
    else if (step < 0 && start > stop)
    {
        length = ((span(stop, start) - 1) / (0 - static_cast<uint64_t>(step))) + 1;
    }
    CheckRangeSize(length);

    return ListAdapter::CreateRange(start, stop, step);
}

// Item `index` of dict(pairs) into `result`
void CollectDictPair(const InternalValue& item, size_t index, TemplateDict& result)
{
    if (const auto* pair = GetIf<KeyValuePair>(&item))
    {
        result.Set(pair->key, pair->value);
        return;
    }
    // Any iterable of two items is a pair, a two-character string included
    bool isConverted = false;
    auto itemList = ConvertToList(item, isConverted, false);
    // Built only when it fails: dict(pairs) allocates nothing per pair for the message
    const auto element = [index] { return "dictionary update sequence element #" + std::to_string(index); };
    if (!isConverted)
    {
        throw TemplateRuntimeError(TemplateRuntimeError::PythonClass::TypeError, "cannot convert " + element() + " to a sequence");
    }
    auto pair = itemList.ToValueList();
    if (pair.size() != 2)
    {
        throw TemplateRuntimeError(TemplateRuntimeError::PythonClass::ValueError, element() + " has length " + std::to_string(pair.size()) + "; 2 is required");
    }
    result.Set(pair[0], pair[1]);
}

// The items of dict(mapping_or_pairs, **kwargs), into `result`
void CollectDictItems(const CallParams& params, const char* fnName, TemplateDict& result)
{
    if (params.posParams.size() > 1)
    {
        throw TemplateRuntimeError(TemplateRuntimeError::PythonClass::TypeError, std::string(fnName) + " expected at most 1 argument, got " + std::to_string(params.posParams.size()));
    }

    if (!params.posParams.empty())
    {
        const auto& source = params.posParams.front();
        // Python's dict() asks an undefined value for its keys(), which fails whatever its policy
        CheckUndefinedUse(source, UndefinedUse::Call);
        auto map = AsMap(source);
        // None and an object such as namespace() are not iterable; a host sequence (a Python list) is
        bool isHostSequence = false;
        ListAdapter hostItems;
        if (map && map->GetObjectClass() && map->IsHost())
        {
            hostItems = ConvertToList(source, isHostSequence, false);
        }
        if (source.IsNone() || (map && map->GetObjectClass() && !isHostSequence))
        {
            throw TemplateRuntimeError(TemplateRuntimeError::PythonClass::TypeError, "'"s + Apply<visitors::PythonTypeNameGetter>(source) + "' object is not iterable");
        }
        if (isHostSequence)
        {
            size_t index = 0;
            hostItems.ForEach([&](InternalValue&& item) {
                const InternalValue pairItem = std::move(item);
                CollectDictPair(pairItem, index++, result);
                return true;
            });
        }
        else if (map)
        {
            for (auto& [key, value] : map->GetEntries())
            {
                result.Set(key, std::move(value));
            }
        }
        else if (auto list = AsList(source))
        {
            size_t index = 0;
            for (const auto& item : *list)
            {
                CollectDictPair(item, index++, result);
            }
        }
        else if (const auto text = GetAsSameString(std::string(), source))
        {
            // Python iterates a string: each item is one character, no pair
            if (!text->empty())
            {
                throw TemplateRuntimeError(TemplateRuntimeError::PythonClass::ValueError, "dictionary update sequence element #0 has length 1; 2 is required");
            }
        }
        else if (!IsEmpty(source))
        {
            // A number or a bool: Python iterates what is not a mapping
            throw TemplateRuntimeError(TemplateRuntimeError::PythonClass::TypeError, "'"s + Apply<visitors::PythonTypeNameGetter>(source) + "' object is not iterable");
        }
    }

    for (const auto& [name, value] : params.kwParams)
    {
        result.SetString(name, value);
    }
}

// dict(mapping_or_pairs, **kwargs)
InternalValue CallDict(const CallParams& params, RenderContext&)
{
    TemplateDict result;
    CollectDictItems(params, "dict", result);
    return CreateMapAdapter(std::move(result));
}

// namespace(mapping_or_pairs, **kwargs): takes its arguments as dict() does
InternalValue CallNamespace(const CallParams& params, RenderContext&)
{
    TemplateDict result;
    CollectDictItems(params, "namespace", result);
    return CreateNamespaceAdapter(std::move(result));
}

// A method that takes no arguments but self, such as Joiner.__call__
void CheckNoArguments(const CallParams& params, const char* method)
{
    if (!params.posParams.empty())
    {
        throw TemplateRuntimeError(std::string(method) + "() takes 1 positional argument but " + std::to_string(params.posParams.size() + 1) + " were given");
    }
    if (!params.kwParams.empty())
    {
        throw TemplateRuntimeError(std::string(method) + "() got an unexpected keyword argument '" + params.kwParams.begin()->first + "'");
    }
}

// A cycler's position and items, kept as a list [pos, *items] that its methods and properties
// hold as their receiver (and a joiner's state likewise) (BoundMethodCall): the cycle collector sees the items there, which
// a lambda's captures would hide (docs/tasks/0251). Empty once the collector freed the cycler
InternalValueList* CyclerState(const InternalValue& state)
{
    const auto list = AsList(state);
    return list ? list->GetMutableItems() : nullptr;
}

size_t CyclerPos(const InternalValueList& state)
{
    return static_cast<size_t>(*GetIf<int64_t>(state.data()));
}

InternalValue CyclerPosProperty(const InternalValue& self, const CallParams&, RenderContext&)
{
    const auto* state = CyclerState(self);
    return state && !state->empty() ? (*state)[0] : InternalValue();
}

InternalValue CyclerCurrent(const InternalValue& self, const CallParams&, RenderContext&)
{
    const auto* state = CyclerState(self);
    return state && state->size() > 1 ? (*state)[CyclerPos(*state) + 1] : InternalValue();
}

InternalValue CyclerNext(const InternalValue& self, const CallParams& callParams, RenderContext&)
{
    CheckNoArguments(callParams, "Cycler.next");
    auto* state = CyclerState(self);
    if (!state || state->size() < 2)
    {
        return InternalValue();
    }
    const auto pos = CyclerPos(*state);
    auto result = (*state)[pos + 1];
    (*state)[0] = static_cast<int64_t>((pos + 1) % (state->size() - 1));
    return result;
}

InternalValue CyclerReset(const InternalValue& self, const CallParams& callParams, RenderContext&)
{
    CheckNoArguments(callParams, "Cycler.reset");
    auto* state = CyclerState(self);
    if (state && !state->empty())
    {
        (*state)[0] = int64_t{ 0 };
    }
    return InternalValue(NoneValue());
}

// The cycler or joiner a method of it is bound to, as the method's repr prints it:
// <bound method Cycler.next of <jinja2.utils.Cycler object at 0x...>> (docs/tasks/0401). It
// holds the object's address only to print it, set once the object is made
struct UtilsObjectReceiver final : IBoundReceiver
{
    explicit UtilsObjectReceiver(const char* name)
        : className(name)
    {
    }

    [[nodiscard]] const char* GetClassName() const override { return className; }
    [[nodiscard]] std::string Repr() const override { return fmt::format("<jinja2.utils.{} object at {}>", className, address); }

    const char* className;
    const void* address = nullptr;
};

// A method of a cycler or joiner, which `state` is the receiver of
Callable MakeUtilsMethod(const InternalValue& state, BoundMethodCall::Method method, const char* name, const std::shared_ptr<const UtilsObjectReceiver>& receiver)
{
    auto result = MakeFunction(BoundMethodCall{ state, method });
    if (const auto list = AsList(state))
    {
        result.SetBoundMethod(list->GetIdentity(), name, receiver);
    }
    return result;
}

// cycler(*items): next(), reset(), current, items, pos
InternalValue CallCycler(const CallParams& params, RenderContext&)
{
    if (!params.kwParams.empty())
    {
        throw TemplateRuntimeError("cycler() got an unexpected keyword argument '" + params.kwParams.begin()->first + "'");
    }
    if (params.posParams.empty())
    {
        throw TemplateRuntimeError("at least one item has to be provided");
    }

    InternalValueList stateItems;
    stateItems.reserve(params.posParams.size() + 1);
    stateItems.emplace_back(int64_t{ 0 });
    stateItems.insert(stateItems.end(), params.posParams.begin(), params.posParams.end());
    const InternalValue state = ListAdapter::CreateAdapter(std::move(stateItems));

    InternalValueMap cycler;
    cycler["items"] = ListAdapter::CreateAdapter(InternalValueList(params.posParams)).MarkAsTuple();
    cycler["pos"] = MakeDynamicProperty(BoundMethodCall{ state, &CyclerPosProperty });
    cycler["current"] = MakeDynamicProperty(BoundMethodCall{ state, &CyclerCurrent });
    auto receiver = std::make_shared<UtilsObjectReceiver>("Cycler");
    cycler["next"] = MakeUtilsMethod(state, &CyclerNext, "next", receiver);
    cycler["reset"] = MakeUtilsMethod(state, &CyclerReset, "reset", receiver);
    auto result = CreateObjectAdapter(std::move(cycler), "Cycler");
    receiver->address = result.GetIdentity();
    return result;
}

// joiner(sep=', '): a callable that returns '' the first time, then sep
// A joiner's state, the list [used, sep] its call holds as its receiver, where the cycle
// collector sees the separator (docs/tasks/0251)
InternalValue CallJoinerObject(const InternalValue& self, const CallParams& callParams, RenderContext&)
{
    CheckNoArguments(callParams, "Joiner.__call__");
    auto* state = CyclerState(self);
    if (!state || state->size() != 2)
    {
        return InternalValue();
    }
    if (ConvertToBool((*state)[0]))
    {
        return (*state)[1];
    }
    (*state)[0] = true;
    return InternalValue(std::string());
}

InternalValue CallJoiner(const CallParams& params, RenderContext&)
{
    auto args = ParseArgs({ { "sep", false, ", "s } }, params, "joiner");
    const InternalValue state = ListAdapter::CreateAdapter(InternalValueList{ InternalValue(false), args["sep"] });
    InternalValueMap joiner;
    joiner["sep"] = args["sep"];
    auto receiver = std::make_shared<UtilsObjectReceiver>("Joiner");
    joiner["operator()"] = MakeUtilsMethod(state, &CallJoinerObject, "__call__", receiver);
    // j.__call__, and j['__call__'] as Jinja2's getitem falls back to the attribute (docs/tasks/0401)
    joiner["__call__"] = joiner["operator()"];
    auto result = CreateObjectAdapter(std::move(joiner), "Joiner");
    receiver->address = result.GetIdentity();
    return result;
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
        // The paragraph grows word by word, so the cap stops min/max of billions of words
        // before they are built (docs/tasks/0263)
        CheckSequenceSize(uint64_t{ text.size() } + (text.empty() ? 0 : 1) + word.size(), "a lipsum text");
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
    // n goes to range(), min and max to randrange() once a paragraph is made
    CheckIntegerArgument(args["n"]);
    auto count = ConvertToInt(args["n"]);
    if (count > 0)
    {
        CheckIntegerArgument(args["min"]);
        CheckIntegerArgument(args["max"]);
    }
    bool html = ConvertToBool(args["html"]);
    auto minWords = ConvertToInt(args["min"]);
    auto maxWords = ConvertToInt(args["max"]);
    if (count > 0 && minWords >= maxWords)
    {
        throw TemplateRuntimeError("lipsum(): empty range for the number of words");
    }

    std::string result;
    for (int64_t paragraph = 0; paragraph < count; ++paragraph)
    {
        auto text = LipsumParagraph(random, minWords, maxWords);
        CheckSequenceSize(uint64_t{ result.size() } + (result.empty() ? 0 : (html ? 1 : 2)) + text.size() + (html ? 7 : 0),
                          "a lipsum text");
        if (!result.empty())
        {
            result += html ? "\n" : "\n\n";
        }
        result += html ? "<p>" + text + "</p>" : text;
    }
    // html=True makes Markup, as Jinja2's generate_lorem_ipsum does
    InternalValue text{ std::string(std::move(result)) };
    if (html)
    {
        text.SetMarkup();
    }
    return text;
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

// The installed callable of a newstyle gettext function, called through Context.call: one that raises StopIteration
// gives undefined (docs/tasks/0500), which `rv % variables` then refuses as Jinja2's Undefined does; under autoescape
// the function first makes it Markup, as str() of the undefined value
InternalValue CallTranslation(const UserCallable& userFn, const CallParams& params, RenderContext& context)
{
    auto callable = visitors::InputValueConvertor::ConvertUserCallable(userFn);
    CallParams rawParams;
    rawParams.posParams = params.posParams;
    HomeCall::Snapshot snapshot(context);
    try
    {
        return GetIf<Callable>(&callable)->GetExpressionCallable()(rawParams, context);
    }
    catch (...)
    {
        detail::KeepFailure();
    }
    if (!detail::KeptIsStop())
    {
        detail::RethrowKept();
    }
    detail::DropKept();
    snapshot.Undo();
    const auto undefined = detail::MakeStopUndefined(context);
    if (!context.IsAutoescape())
    {
        CheckUndefinedUse(undefined, UndefinedUse::Arithmetic);
    }
    return MakeMarkup(undefined, context.GetRendererCallback());
}

// _make_new_gettext and friends: translates the message, then formats it with the keyword
// arguments (`num` defaults to the count); under autoescape the result is Markup and the
// variables are escaped
InternalValue CallGettext(const GettextFunction& fn, const CallParams& params, RenderContext& context, const std::optional<UserCallable>& userFn)
{
    size_t argsCount = 1U + (fn.hasContext ? 1U : 0U) + (fn.hasPlural ? 2U : 0U);
    if (params.posParams.size() != argsCount)
    {
        throw TemplateRuntimeError(std::string(fn.name) + "() takes " + std::to_string(argsCount) + " positional arguments but " + std::to_string(params.posParams.size()) + " were given");
    }

    InternalDict variables = params.kwParams;

    InternalValue translated;
    if (userFn)
    {
        translated = CallTranslation(*userFn, params, context);
    }
    else
    {
        size_t message = fn.hasContext ? 1 : 0;
        if (fn.hasPlural)
        {
            // `n == 1`, which a strict undefined refuses
            CheckUndefinedUse(params.posParams[message + 2], UndefinedUse::Operator);
        }
        translated = fn.hasPlural && !IsOne(params.posParams[message + 2]) ? params.posParams[message + 1] : params.posParams[message];
    }
    if (fn.hasPlural)
    {
        variables.try_emplace("num", params.posParams[argsCount - 1]);
    }

    bool isWide = false;
    bool isString = ApplyStringConverter(translated, [&isWide](auto str) {
        isWide = sizeof(str[0]) != sizeof(char);
        return true;
    });
    if (!isString)
    {
        throw TemplateRuntimeError("unsupported operand type(s) for %: '"s + Apply<visitors::PythonTypeNameGetter>(translated) + "' and 'dict'");
    }

    InternalValue values = CreateMapAdapter(std::move(variables));
    auto formatted = PythonPercentFormat(ApplyStringConverter(translated, [](auto str) { return ConvertString<std::string>(str); }), values, context.IsAutoescape(),
                                         context.GetRendererCallback()->GetPromptEscaper());
    InternalValue result = isWide ? TargetString(ConvertString<std::wstring>(formatted)) : TargetString(std::move(formatted)); // NOLINT(performance-move-const-arg): a real move into the variant (clang-tidy misreads the converting constructor), task 0600
    result.SetMarkup(context.IsAutoescape());
    return result;
}

// Oldstyle (Jinja2's default, newstyle_gettext off): the global is the installed callable itself, called with the
// arguments of the call and returning what it returns. Without one, gettext.NullTranslations: the message, or the
// plural one when the count is not 1
InternalValue CallOldGettext(const GettextFunction& fn, const CallParams& params, RenderContext& context, const std::optional<UserCallable>& userFn)
{
    if (userFn)
    {
        auto callable = visitors::InputValueConvertor::ConvertUserCallable(*userFn);
        return GetIf<Callable>(&callable)->GetExpressionCallable()(params, context);
    }
    // The parameters of gettext.NullTranslations' methods, by position or by keyword
    std::vector<const char*> names;
    if (fn.hasContext)
    {
        names.push_back("context");
    }
    if (fn.hasPlural)
    {
        names.insert(names.end(), { "msgid1", "msgid2", "n" });
    }
    else
    {
        names.push_back("message");
    }
    const auto typeError = [&fn](const std::string& what) {
        return TemplateRuntimeError(TemplateRuntimeError::PythonClass::TypeError, "NullTranslations."s + fn.name + "() " + what);
    };
    if (params.posParams.size() > names.size())
    {
        throw typeError("takes " + std::to_string(names.size() + 1) + " positional arguments but " + std::to_string(params.posParams.size() + 1) + " were given");
    }
    std::vector<const InternalValue*> args(names.size(), nullptr);
    for (size_t idx = 0; idx < params.posParams.size(); ++idx)
    {
        args[idx] = &params.posParams[idx];
    }
    for (const auto& kwParam : params.kwParams)
    {
        const auto& name = kwParam.first;
        const auto pos = std::find_if(names.begin(), names.end(), [&name](const char* known) { return name == known; });
        if (pos == names.end())
        {
            throw typeError("got an unexpected keyword argument '" + name + "'");
        }
        auto& arg = args[static_cast<size_t>(pos - names.begin())];
        if (arg)
        {
            throw typeError("got multiple values for argument '" + name + "'");
        }
        arg = &kwParam.second;
    }
    for (size_t idx = 0; idx < args.size(); ++idx)
    {
        if (!args[idx])
        {
            throw typeError("missing required argument '"s + names[idx] + "'");
        }
    }
    const size_t message = fn.hasContext ? 1 : 0;
    if (!fn.hasPlural)
    {
        return *args[message];
    }
    // `n == 1`, which a strict undefined refuses
    CheckUndefinedUse(*args[message + 2], UndefinedUse::Operator);
    return IsOne(*args[message + 2]) ? *args[message] : *args[message + 1];
}

// A gettext global of a template loaded with the style `loadNewstyle`: it takes the style of the last
// InstallGettextCallables of the environment, as Jinja2's globals are what the last install_gettext_* put
// (docs/tasks/0509), and the load's when the environment installed none
InternalValue CallGettextGlobal(const GettextFunction& fn, const CallParams& params, RenderContext& context, bool loadNewstyle)
{
    bool newstyle = loadNewstyle;
    std::optional<UserCallable> userFn;
    if (const auto* env = context.GetEnv())
    {
        userFn = detail::TemplateEnvAccess::GetImpl(*env)->FindTranslation(fn.name, newstyle);
    }
    return newstyle ? CallGettext(fn, params, context, userFn) : CallOldGettext(fn, params, context, userFn);
}

// jinja2.ext._gettext_alias: `_` calls whatever `gettext` is at the call site
InternalValue CallGettextAlias(const CallParams& params, RenderContext& context)
{
    const auto gettext = context.FindValue(std::string("gettext"));
    return CallExpression::CallValue(context, gettext ? *gettext : MakeUndefined(context, "gettext"), params);
}
} // namespace

namespace
{
void SetupI18nGlobals(InternalValueMap& globalParams, bool newstyle)
{
    static const GettextFunction functions[] = { { "gettext", false, false }, { "ngettext", false, true }, { "pgettext", true, false }, { "npgettext", true, true } };
    for (const auto& fn : functions)
    {
        globalParams.emplace(fn.name, MakeFunction([&fn, newstyle](const CallParams& params, RenderContext& context) { return CallGettextGlobal(fn, params, context, newstyle); }));
    }
    globalParams.emplace("_", MakeFunction(CallGettextAlias));
}

// The globals that are classes in Jinja2, one object each for every render and every environment, so that a value's
// __class__ is the global itself: {}.__class__ == dict (docs/tasks/0476)
struct ClassGlobal
{
    const char* global;
    // Python's __name__ and __module__
    const char* name;
    const char* module;
    Callable value;
};

Callable MakeClassGlobal(Callable::ExpressionCallable fn, const char* repr, const char* name, const char* module)
{
    auto result = MakeClass(std::move(fn), repr);
    result.SetAttributes(std::make_shared<const InternalValueMap>(InternalValueMap{
        { "__name__", InternalValue(std::string(name)) }, { "__qualname__", InternalValue(std::string(name)) }, { "__module__", InternalValue(std::string(module)) } }));
    return result;
}

const std::vector<ClassGlobal>& ClassGlobals()
{
    static const auto classes = [] {
        // Made once, maybe in a render with a memory budget, for every render after it
        const UncountedMemoryScope uncounted;
        return std::vector<ClassGlobal>{
            { "range", "range", "builtins", MakeClassGlobal(CallRange, "<class 'range'>", "range", "builtins") },
            { "dict", "dict", "builtins", MakeClassGlobal(CallDict, "<class 'dict'>", "dict", "builtins") },
            { "cycler", "Cycler", "jinja2.utils", MakeClassGlobal(CallCycler, "<class 'jinja2.utils.Cycler'>", "Cycler", "jinja2.utils") },
            { "joiner", "Joiner", "jinja2.utils", MakeClassGlobal(CallJoiner, "<class 'jinja2.utils.Joiner'>", "Joiner", "jinja2.utils") },
            { "namespace", "Namespace", "jinja2.utils", MakeClassGlobal(CallNamespace, "<class 'jinja2.utils.Namespace'>", "Namespace", "jinja2.utils") },
        };
    }();
    return classes;
}

void SetupGlobals(InternalValueMap& globalParams)
{
    for (const auto& cls : ClassGlobals())
    {
        globalParams.emplace(cls.global, cls.value);
    }
    // A function, which prints as <function generate_lorem_ipsum at 0x...> (docs/tasks/0368)
    auto lipsum = MakeFunction([](const CallParams& params, RenderContext& context) {
        // Every render has a callback (TemplateImpl::Render)
        return CallLipsum(params, context.GetRendererCallback()->GetRandomEngine());
    });
    lipsum.SetAttributes(std::make_shared<const InternalValueMap>(InternalValueMap{ { "__name__", InternalValue(std::string("generate_lorem_ipsum")) },
                                                                                    { "__qualname__", InternalValue(std::string("generate_lorem_ipsum")) },
                                                                                    { "__module__", InternalValue(std::string("jinja2.utils")) } }));
    globalParams.emplace("lipsum", std::move(lipsum));
}
} // namespace

std::optional<InternalValue> ClassGlobalOf(std::string_view module, std::string_view name)
{
    for (const auto& cls : ClassGlobals())
    {
        if (cls.name == name && cls.module == module)
        {
            return InternalValue(cls.value);
        }
    }
    return std::nullopt;
}

namespace
{
using ErrorClass = TemplateRuntimeError::PythonClass;

std::string ReprOf(const InternalValue& value)
{
    std::string result;
    Apply<visitors::ValueRenderer<char>>(value, result, true);
    return result;
}

[[noreturn]] void ThrowTypeError(const std::string& message)
{
    throw TemplateRuntimeError(ErrorClass::TypeError, message);
}

// What the class `cls` says of more positional arguments than it takes, as CPython words it
void CheckClassArguments(std::string_view cls, const CallParams& params, size_t most)
{
    const auto given = params.posParams.size();
    if (given <= most)
    {
        return;
    }
    if (cls == "int" || cls == "str")
    {
        ThrowTypeError(std::string(cls) + "() takes at most " + std::to_string(most) + " argument" + (most == 1 ? "" : "s") + " (" + std::to_string(given) + " given)");
    }
    ThrowTypeError(std::string(cls) + " expected at most " + std::to_string(most) + " argument" + (most == 1 ? "" : "s") + ", got " + std::to_string(given));
}

// The result of the built-in filter `name` of the value, which is how Python's list(), str() and Markup() read it
InternalValue ApplyBuiltinFilter(std::string_view name, const CallParams& filterParams, const InternalValue& value, RenderContext& context)
{
    return CreateFilter(name, FunctionArgs(filterParams))->Filter(value, context);
}

// The text int() reads in a base: without its space, sign and prefix
struct IntText
{
    std::string_view digits;
    bool negative = false;
    int64_t base = 10;
};

// The base a 0x, 0o or 0b prefix of the text names; 0 for none
int64_t IntPrefixBase(std::string_view text)
{
    if (text.size() < 2 || text.front() != '0')
    {
        return 0;
    }
    // ASCII only, whatever the C locale
    const char letter = text[1] >= 'A' && text[1] <= 'Z' ? static_cast<char>(text[1] - 'A' + 'a') : text[1];
    return letter == 'x' ? 16 : (letter == 'o' ? 8 : (letter == 'b' ? 2 : 0));
}

// Python's int(text, base) of a base of 0 or 2 to 36 first strips the space around the text, then reads a sign and a
// 0x, 0o or 0b prefix of the base (any of them for base 0); nullopt when what is left cannot be digits of the base
std::optional<IntText> SplitIntText(const std::string& raw, int64_t base)
{
    // The ASCII space characters, whatever the C locale
    const auto isSpace = [](char ch) { return ch == ' ' || (ch >= '\t' && ch <= '\r'); };
    size_t pos = 0;
    size_t end = raw.size();
    while (pos < end && isSpace(raw[pos]))
    {
        ++pos;
    }
    while (end > pos && isSpace(raw[end - 1]))
    {
        --end;
    }
    IntText result;
    if (pos < end && (raw[pos] == '+' || raw[pos] == '-'))
    {
        result.negative = raw[pos] == '-';
        ++pos;
    }
    const auto prefixBase = IntPrefixBase(std::string_view(raw.data() + pos, end - pos));
    if (prefixBase != 0 && (base == 0 || base == prefixBase))
    {
        base = prefixBase;
        pos += 2;
        // 0x_ff is an integer
        pos += (pos < end && raw[pos] == '_') ? size_t{ 1 } : size_t{ 0 };
    }
    else if (base == 0)
    {
        base = 10;
        // A decimal with a leading zero is no integer unless it is all zeros
        const std::string_view rest(raw.data() + pos, end - pos);
        if (rest.size() > 1 && rest.front() == '0' && rest.find_first_not_of("0_") != std::string_view::npos)
        {
            return std::nullopt;
        }
    }
    result.digits = std::string_view(raw.data() + pos, end - pos);
    result.base = base;
    return result;
}

// The value of the digit `ch` (0-9, a-z, A-Z), or a number no base has
int64_t DigitValue(char ch)
{
    if (ch >= '0' && ch <= '9')
    {
        return ch - '0';
    }
    if (ch >= 'a' && ch <= 'z')
    {
        return ch - 'a' + 10;
    }
    if (ch >= 'A' && ch <= 'Z')
    {
        return ch - 'A' + 10;
    }
    return 99;
}

// Python's int(text, base): digits with single underscores between them. nullopt when the text is no integer in the
// base; `overflow` is set for one beyond int64, which the caller builds as the int filter does
std::optional<int64_t> ParseIntInBase(const std::string& raw, int64_t base, bool& overflow)
{
    const auto split = SplitIntText(raw, base);
    if (!split)
    {
        return std::nullopt;
    }
    const uint64_t limit = split->negative ? uint64_t{ 9223372036854775808ULL } : uint64_t{ 9223372036854775807ULL };
    const auto ubase = static_cast<uint64_t>(split->base);
    uint64_t magnitude = 0;
    bool afterDigit = false;
    for (const char ch : split->digits)
    {
        if (ch == '_')
        {
            if (!afterDigit)
            {
                return std::nullopt;
            }
            afterDigit = false;
            continue;
        }
        const auto digit = DigitValue(ch);
        if (digit >= split->base)
        {
            return std::nullopt;
        }
        afterDigit = true;
        const auto udigit = static_cast<uint64_t>(digit);
        overflow = overflow || magnitude > (limit - udigit) / ubase;
        if (!overflow)
        {
            magnitude = (magnitude * ubase) + udigit;
        }
    }
    // A trailing underscore, or no digit at all
    if (!afterDigit)
    {
        return std::nullopt;
    }
    return overflow ? int64_t{ 0 } : (split->negative ? static_cast<int64_t>(0 - magnitude) : static_cast<int64_t>(magnitude));
}

// int(text, base) for an explicit base: a string only, which the base reads
InternalValue ConstructIntInBase(const InternalValue& arg, const InternalValue& base, RenderContext& context)
{
    CheckIntegerArgument(base);
    if (!IsStringValue(arg))
    {
        ThrowTypeError("int() can't convert non-string with explicit base");
    }
    const auto baseNumber = ConvertToInt(base);
    if (baseNumber != 0 && (baseNumber < 2 || baseNumber > 36))
    {
        throw TemplateRuntimeError(ErrorClass::ValueError, "int() base must be >= 2 and <= 36, or 0");
    }
    const auto text = filters::PythonNumberText(arg);
    bool overflow = false;
    if (const auto parsed = text ? ParseIntInBase(*text, baseNumber, overflow) : std::nullopt)
    {
        if (!overflow)
        {
            return InternalValue(*parsed);
        }
        // Beyond int64, which the int filter builds, the text being an integer already
        CallParams filterParams;
        filterParams.kwParams.emplace("base", base);
        filterParams.kwParams.emplace("default", InternalValue());
        if (auto result = ApplyBuiltinFilter("int", filterParams, arg, context); !result.IsUndefined())
        {
            return result;
        }
    }
    throw TemplateRuntimeError(ErrorClass::ValueError, "invalid literal for int() with base " + std::to_string(baseNumber) + ": " + ReprOf(arg));
}

// int(x) of a value that is no string
InternalValue ConstructIntOfNumber(const InternalValue& arg, RenderContext& context)
{
    if (const auto* flag = GetIf<bool>(&arg))
    {
        return InternalValue(int64_t{ *flag ? 1 : 0 });
    }
    if (GetIf<int64_t>(&arg) || arg.Kind() == ValueKind::BigInt)
    {
        return arg;
    }
    if (const auto* number = GetIf<double>(&arg))
    {
        if (std::isnan(*number))
        {
            throw TemplateRuntimeError(ErrorClass::ValueError, "cannot convert float NaN to integer");
        }
        if (std::isinf(*number))
        {
            throw TemplateRuntimeError("cannot convert float infinity to integer");
        }
        return ApplyBuiltinFilter("int", CallParams(), arg, context);
    }
    ThrowTypeError(std::string("int() argument must be a string, a bytes-like object or a real number, not '") + Apply<visitors::PythonTypeNameGetter>(arg) + "'");
}

InternalValue ConstructInt(const CallParams& params, RenderContext& context)
{
    CheckClassArguments("int", params, 2);
    for (const auto& kw : params.kwParams)
    {
        if (kw.first != "base" || params.posParams.size() == 2)
        {
            ThrowTypeError("'" + kw.first + "' is an invalid keyword argument for int()");
        }
    }
    if (params.posParams.empty())
    {
        if (!params.kwParams.empty())
        {
            ThrowTypeError("int() missing string argument");
        }
        return InternalValue(int64_t{ 0 });
    }
    const auto& arg = params.posParams.front();
    if (const auto* info = GetUndefinedInfo(arg))
    {
        ThrowUndefined(*info);
    }
    if (params.posParams.size() == 2)
    {
        return ConstructIntInBase(arg, params.posParams[1], context);
    }
    if (!params.kwParams.empty())
    {
        return ConstructIntInBase(arg, params.kwParams.begin()->second, context);
    }
    const auto text = filters::PythonNumberText(arg);
    if (!text)
    {
        return ConstructIntOfNumber(arg, context);
    }
    if (auto result = filters::PythonIntOfText(*text))
    {
        return std::move(*result);
    }
    throw TemplateRuntimeError(ErrorClass::ValueError, "invalid literal for int() with base 10: " + ReprOf(arg));
}

InternalValue ConstructFloat(const CallParams& params, RenderContext& context)
{
    CheckClassArguments("float", params, 1);
    if (!params.kwParams.empty())
    {
        ThrowTypeError("float() takes no keyword arguments");
    }
    if (params.posParams.empty())
    {
        return InternalValue(0.0);
    }
    const auto& arg = params.posParams.front();
    if (const auto* info = GetUndefinedInfo(arg))
    {
        ThrowUndefined(*info);
    }
    if (GetIf<double>(&arg) || GetIf<int64_t>(&arg) || GetIf<bool>(&arg) || arg.Kind() == ValueKind::BigInt)
    {
        return ApplyBuiltinFilter("float", CallParams(), arg, context);
    }
    if (const auto text = filters::PythonNumberText(arg))
    {
        if (auto result = filters::PythonFloatOfText(*text))
        {
            return InternalValue(*result);
        }
        throw TemplateRuntimeError(ErrorClass::ValueError, "could not convert string to float: " + ReprOf(arg));
    }
    ThrowTypeError(std::string("float() argument must be a string or a real number, not '") + Apply<visitors::PythonTypeNameGetter>(arg) + "'");
}

// list(x) and tuple(x): the items of x, which the list filter iterates as Python does
InternalValue ConstructSequence(std::string_view cls, const CallParams& params, RenderContext& context)
{
    CheckClassArguments(cls, params, 1);
    if (!params.kwParams.empty())
    {
        ThrowTypeError(std::string(cls) + "() takes no keyword arguments");
    }
    auto result = params.posParams.empty() ? ListAdapter::CreateAdapter(InternalValueList{}) : ApplyBuiltinFilter("list", CallParams(), params.posParams.front(), context);
    if (cls == "tuple")
    {
        if (const auto list = AsList(result))
        {
            return ListAdapter::CreateAdapter(list->ToValueList()).MarkAsTuple();
        }
    }
    return result;
}

InternalValue ConstructString(const CallParams& params, RenderContext& context)
{
    CheckClassArguments("str", params, 3);
    // str(object='', encoding='utf-8', errors='strict')
    constexpr std::array<std::string_view, 3> names = { "object", "encoding", "errors" };
    std::array<const InternalValue*, 3> args{};
    for (size_t i = 0; i < params.posParams.size(); ++i)
    {
        args[i] = &params.posParams[i];
    }
    for (const auto& [key, value] : params.kwParams)
    {
        size_t index = 0;
        while (index < names.size() && names[index] != key)
        {
            ++index;
        }
        if (index == names.size())
        {
            auto message = "'" + key;
            message += "' is an invalid keyword argument for str()";
            ThrowTypeError(message);
        }
        if (args[index])
        {
            auto message = "argument for str() given by name ('" + key;
            message += "') and position (" + std::to_string(index + 1) + ")";
            ThrowTypeError(message);
        }
        args[index] = &value;
    }
    if (!args[0])
    {
        return InternalValue(std::string());
    }
    if (args[1] || args[2])
    {
        // The engine has no bytes, so only Python's refusals of decoding remain
        if (IsStringValue(*args[0]))
        {
            ThrowTypeError("decoding str is not supported");
        }
        ThrowTypeError(std::string("decoding to str: need a bytes-like object, ") + Apply<visitors::PythonTypeNameGetter>(*args[0]) + " found");
    }
    // str(Markup) is a plain str
    auto result = ApplyBuiltinFilter("string", CallParams(), *args[0], context);
    result.SetMarkup(false);
    return result;
}
} // namespace

namespace
{
InternalValue ConstructBool(const CallParams& params)
{
    CheckClassArguments("bool", params, 1);
    if (!params.kwParams.empty())
    {
        ThrowTypeError("bool() takes no keyword arguments");
    }
    if (params.posParams.empty())
    {
        return InternalValue(false);
    }
    if (const auto* undefined = GetIf<UndefinedValue>(&params.posParams.front()))
    {
        CheckStrictUndefined(*undefined);
    }
    return InternalValue(ConvertToBool(params.posParams.front()));
}

InternalValue ConstructType(const CallParams& params)
{
    if (params.posParams.size() != 1 || !params.kwParams.empty())
    {
        ThrowTypeError("type() takes 1 or 3 arguments");
    }
    if (auto result = MakeClassObject(params.posParams.front()))
    {
        return std::move(*result);
    }
    ThrowTypeError("type() of this value is not supported");
}

// Undefined(hint=None, obj=missing, name=None, exc=UndefinedError) of the class `className`
InternalValue ConstructUndefined(std::string_view className, const CallParams& params)
{
    const std::string cls(className);
    if (params.posParams.size() > 4)
    {
        ThrowTypeError(cls + ".__init__() takes from 1 to 5 positional arguments but " + std::to_string(params.posParams.size() + 1) + " were given");
    }
    constexpr std::array<std::string_view, 4> names = { "hint", "obj", "name", "exc" };
    std::array<const InternalValue*, 4> args{};
    for (size_t i = 0; i < params.posParams.size(); ++i)
    {
        args[i] = &params.posParams[i];
    }
    for (const auto& [key, value] : params.kwParams)
    {
        size_t index = 0;
        while (index < names.size() && names[index] != key)
        {
            ++index;
        }
        if (index == names.size())
        {
            auto message = cls + ".__init__() got an unexpected keyword argument '";
            message += key;
            message += '\'';
            ThrowTypeError(message);
        }
        if (args[index])
        {
            auto message = cls + ".__init__() got multiple values for argument '";
            message += key;
            message += '\'';
            ThrowTypeError(message);
        }
        args[index] = &value;
    }
    std::string hint;
    if (args[0] && IsStringValue(*args[0]))
    {
        hint = AsString(*args[0]);
    }
    if (auto result = MakeUndefinedOfClass(className, std::move(hint), args[1], args[2]))
    {
        return std::move(*result);
    }
    ThrowTypeError("cannot create '" + cls + "' instances");
}

// Markup(base, encoding) with an encoding other than None is str()'s, and the engine has no bytes to decode
[[noreturn]] void RefuseMarkupEncoding(const CallParams& params, const InternalValue& encoding)
{
    if (!IsStringValue(encoding))
    {
        ThrowTypeError(std::string("str() argument 'encoding' must be str, not ") + Apply<visitors::PythonTypeNameGetter>(encoding));
    }
    if (params.posParams.empty() || IsStringValue(params.posParams.front()))
    {
        ThrowTypeError("decoding str is not supported");
    }
    // An object with __html__ is its markup, a str
    if (const auto* host = GetHostObject(params.posParams.front()); !host || !host->IsMarkupLike())
    {
        ThrowTypeError(std::string("decoding to str: need a bytes-like object, ") + Apply<visitors::PythonTypeNameGetter>(params.posParams.front()) + " found");
    }
    ThrowTypeError("decoding str is not supported");
}

InternalValue ConstructMarkup(const CallParams& params, RenderContext& context)
{
    // Markup.__new__(cls, base='', encoding=None, errors='strict'): `base` is positional only
    for (const auto& kw : params.kwParams)
    {
        if (kw.first != "encoding" && kw.first != "errors")
        {
            ThrowTypeError("Markup.__new__() got an unexpected keyword argument '" + kw.first + "'");
        }
    }
    CheckClassArguments("Markup", params, 3);
    // An encoding other than None is str()'s: the engine has no bytes, so a base is never decodable
    const InternalValue* encoding = params.posParams.size() > 1 ? &params.posParams[1] : nullptr;
    for (const auto& kw : params.kwParams)
    {
        if (kw.first == "encoding")
        {
            if (encoding)
            {
                ThrowTypeError("Markup.__new__() got multiple values for argument 'encoding'");
            }
            encoding = &kw.second;
        }
    }
    if (encoding && !encoding->IsNone())
    {
        RefuseMarkupEncoding(params, *encoding);
    }
    return params.posParams.empty() ? InternalValue(std::string()).SetMarkup() : ApplyBuiltinFilter("safe", CallParams(), params.posParams.front(), context);
}

bool IsUndefinedClass(std::string_view className)
{
    return className == "Undefined" || className == "StrictUndefined" || className == "ChainableUndefined" || className == "DebugUndefined";
}
} // namespace

InternalValue CallPythonClass(std::string_view className, const CallParams& params, RenderContext& context)
{
    if (className == "int")
    {
        return ConstructInt(params, context);
    }
    if (className == "float")
    {
        return ConstructFloat(params, context);
    }
    if (className == "str")
    {
        return ConstructString(params, context);
    }
    if (className == "list" || className == "tuple")
    {
        return ConstructSequence(className, params, context);
    }
    if (className == "bool")
    {
        return ConstructBool(params);
    }
    if (className == "NoneType")
    {
        if (!params.posParams.empty() || !params.kwParams.empty())
        {
            ThrowTypeError("NoneType takes no arguments");
        }
        return InternalValue(NoneValue{});
    }
    if (className == "type")
    {
        return ConstructType(params);
    }
    if (className == "Markup")
    {
        return ConstructMarkup(params, context);
    }
    if (IsUndefinedClass(className))
    {
        return ConstructUndefined(className, params);
    }
    ThrowTypeError("cannot create '" + std::string(className) + "' instances");
}

// Jinja2's MarkSafeIfAutoescape node, which an oldstyle `{% trans %}` block wraps its gettext call in: Markup(value)
// under autoescape, the value itself otherwise
const InternalValue& MarkSafeIfAutoescapeFunction()
{
    static const InternalValue fn = [] {
        const UncountedMemoryScope uncounted;
        return InternalValue(MakeFunction([](const CallParams& params, RenderContext& context) {
            const auto& value = params.posParams.front();
            if (!context.IsAutoescape())
            {
                return value;
            }
            if (!IsStringValue(value))
            {
                return MakeMarkup(value, context.GetRendererCallback());
            }
            InternalValue result = value;
            result.SetMarkup();
            return result;
        }));
    }();
    return fn;
}

namespace
{
struct BuiltinGlobals
{
    InternalValueMap plain;
    InternalValueMap withI18n;
    InternalValueMap withOldI18n;
};

BuiltinGlobals MakeBuiltinGlobals()
{
    // Made once, maybe in a render with a memory budget, for every render after it
    const UncountedMemoryScope uncounted;
    BuiltinGlobals result;
    SetupGlobals(result.plain);
    // The same objects in every table, so that `lipsum` is one object with i18n on or off, as the classes are (PythonGlobal)
    result.withI18n = result.plain;
    SetupI18nGlobals(result.withI18n, true);
    result.withOldI18n = result.plain;
    SetupI18nGlobals(result.withOldI18n, false);
    return result;
}

// Out of line, so that each render's GetBuiltinGlobals is one guard test and a select, with no object on its stack
JINJA2CPP_NOINLINE const BuiltinGlobals& BuildBuiltinGlobals()
{
    static const BuiltinGlobals globals = MakeBuiltinGlobals();
    return globals;
}
} // namespace

const InternalValueMap& GetBuiltinGlobals(const Settings::Extensions& extensions)
{
    // Built once and only read afterwards, so renders on any thread share them; the state of
    // the stateful globals (cycler, joiner, lipsum's generator) lives in their results or in
    // the render
    static const BuiltinGlobals& globals = BuildBuiltinGlobals();
    if (!extensions.i18n)
    {
        return globals.plain;
    }
    return extensions.i18nNewstyle ? globals.withI18n : globals.withOldI18n;
}

std::optional<InternalValue> PythonGlobalOf(std::string_view module, std::string_view qualname)
{
    if (auto global = ClassGlobalOf(module, qualname))
    {
        return global;
    }
    if (module == "jinja2.utils" && qualname == "generate_lorem_ipsum")
    {
        static const Settings::Extensions plain{};
        const auto& builtins = GetBuiltinGlobals(plain);
        const auto found = builtins.find("lipsum");
        return found == builtins.end() ? std::nullopt : std::optional<InternalValue>(found->second);
    }
    return std::nullopt;
}
} // namespace v2
} // namespace jinja2
