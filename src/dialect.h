#ifndef JINJA2CPP_SRC_DIALECT_H
#define JINJA2CPP_SRC_DIALECT_H

namespace jinja2
{
inline namespace v2
{
namespace detail
{

// The behaviour a template is loaded and rendered with besides Settings. Not public: a preset
// switches it (ConfigureForChatTemplates, docs/tasks/0360), and it stays Jinja2 otherwise, so
// the core compiles the same with or without the preset
enum class Dialect : unsigned char
{
    // Python Jinja2's Environment
    Jinja2,
    // transformers' apply_chat_template environment: an ImmutableSandboxedEnvironment whose
    // `tojson` is json.dumps (keys in insertion order, no HTML escaping, ensure_ascii, indent,
    // separators, sort_keys) and which knows the `{% generation %}` tag. The list and dict
    // methods that change their receiver are unsafe attributes there
    Transformers
};

} // namespace detail
} // namespace v2
} // namespace jinja2

#endif // JINJA2CPP_SRC_DIALECT_H
