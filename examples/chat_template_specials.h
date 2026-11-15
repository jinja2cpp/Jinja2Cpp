// Example, not library code (docs/tasks/0394): rendering a Hugging Face chat template in prompt
// mode with the special tokens of the model's tokenizer_config.json.
//
// The library keeps no list of a model's tokens and reads no tokenizer files: the host does, as
// here with nlohmann::json. The test suite compiles and runs this file
// (test/chat_template_preset_test.cpp, ChatTemplateSpecialsExample).
//
//     auto config = nlohmann::json::parse(std::ifstream("tokenizer_config.json"));
//     auto specials = chat_example::ReadSpecials(config);
//     jinja2::TemplateEnv env;
//     chat_example::ConfigureForPrompts(env, specials);
//     jinja2::Template tpl(&env);
//     tpl.Load(config["chat_template"].get<std::string>());
//     jinja2::ValuesMap params = specials.params; // bos_token, eos_token, ...
//     params["messages"] = ...;                  // untrusted text: its special tokens are broken
//     auto prompt = tpl.RenderAsString(params);
#ifndef JINJA2CPP_EXAMPLES_CHAT_TEMPLATE_SPECIALS_H
#define JINJA2CPP_EXAMPLES_CHAT_TEMPLATE_SPECIALS_H

#include <jinja2cpp/chat_template.h>
#include <jinja2cpp/prompt.h>
#include <jinja2cpp/template_env.h>
#include <jinja2cpp/value.h>

#include <nlohmann/json.hpp>

#include <algorithm>
#include <string>
#include <utility>
#include <vector>

namespace chat_example
{

struct Specials
{
    // Every special token of the tokenizer: what the escaper breaks in untrusted text
    std::vector<std::string> tokens;
    // The params transformers passes to the template (bos_token, eos_token, ...), as plain strings
    jinja2::ValuesMap params;
    // Their names, which ChatTemplateOptions::promptSafeParams makes prompt-safe
    std::vector<std::string> names;
};

namespace detail
{
// A token is written as its text or as a serialized AddedToken: {"content": "<s>", ...}
inline const std::string* TokenText(const nlohmann::json& token)
{
    if (token.is_string())
    {
        return token.get_ptr<const std::string*>();
    }
    if (token.is_object())
    {
        const auto content = token.find("content");
        if (content != token.end() && content->is_string())
        {
            return content->get_ptr<const std::string*>();
        }
    }
    return nullptr;
}

inline void AddToken(std::vector<std::string>& tokens, const std::string& text)
{
    if (!text.empty() && std::find(tokens.begin(), tokens.end(), text) == tokens.end())
    {
        tokens.push_back(text);
    }
}
} // namespace detail

// The special tokens of a tokenizer_config.json: the added tokens marked "special", the named
// ones (bos_token, ...) and additional_special_tokens
inline Specials ReadSpecials(const nlohmann::json& config)
{
    Specials result;
    if (!config.is_object())
    {
        return result;
    }
    if (const auto decoder = config.find("added_tokens_decoder"); decoder != config.end() && decoder->is_object())
    {
        for (const auto& token : *decoder)
        {
            const auto special = token.find("special");
            const auto* text = detail::TokenText(token);
            if (text && special != token.end() && special->is_boolean() && special->get<bool>())
            {
                detail::AddToken(result.tokens, *text);
            }
        }
    }
    for (const char* name : { "bos_token", "eos_token", "unk_token", "sep_token", "pad_token", "cls_token", "mask_token" })
    {
        const auto token = config.find(name);
        if (token == config.end())
        {
            continue;
        }
        if (const auto* text = detail::TokenText(*token))
        {
            detail::AddToken(result.tokens, *text);
            result.params[name] = *text;
            result.names.emplace_back(name);
        }
    }
    if (const auto extra = config.find("additional_special_tokens"); extra != config.end() && extra->is_array())
    {
        for (const auto& token : *extra)
        {
            if (const auto* text = detail::TokenText(token))
            {
                detail::AddToken(result.tokens, *text);
            }
        }
    }
    return result;
}

// The chat-template preset in prompt mode: the tokenizer's specials are broken in every value
// that is not prompt-safe, and the named params that carry them on purpose are prompt-safe
// PromptEscaper throws std::invalid_argument when a token contains its separator (a space by
// default): choose another separator in PromptEscapeOptions for such a tokenizer
inline void ConfigureForPrompts(jinja2::TemplateEnv& env, const Specials& specials)
{
    jinja2::ChatTemplateOptions options;
    options.promptSafeParams = specials.names;
    jinja2::ConfigureForChatTemplates(env, std::move(options));
    auto& settings = env.GetSettings();
    settings.autoescape = true;
    settings.autoescapeMode = jinja2::AutoescapeMode::Prompt;
    settings.promptEscaper = jinja2::PromptEscaper({ specials.tokens, {} });
}

} // namespace chat_example

#endif // JINJA2CPP_EXAMPLES_CHAT_TEMPLATE_SPECIALS_H
