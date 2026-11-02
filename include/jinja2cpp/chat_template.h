#ifndef JINJA2CPP_CHAT_TEMPLATE_H
#define JINJA2CPP_CHAT_TEMPLATE_H

// Rendering Hugging Face chat templates as transformers' apply_chat_template does (docs/tasks/0360).
// Installed and built only with the CMake option JINJA2CPP_WITH_CHAT_TEMPLATES (default ON);
// JINJA2CPP_HAS_CHAT_TEMPLATES in config.h tells whether it is.

#include "config.h"
#include "template_env.h"

#include <chrono>
#include <functional>

#if !JINJA2CPP_HAS_CHAT_TEMPLATES
#error "Jinja2C++ was built without chat templates (JINJA2CPP_WITH_CHAT_TEMPLATES=OFF)"
#endif

namespace jinja2
{
inline namespace v2
{

//! Options of \ref ConfigureForChatTemplates. Later knobs (special tokens, say) are new fields
struct ChatTemplateOptions
{
    //! The clock `strftime_now` reads. Empty: `std::chrono::system_clock::now`. The time is shown in local time, as
    //! Python's `datetime.now()`
    std::function<std::chrono::system_clock::time_point()> now;
};

/*!
 * \brief Configures `env` as transformers' `apply_chat_template` configures its Jinja2 environment
 *
 * Tracks transformers \ref JINJA2CPP_CHAT_TEMPLATES_TRANSFORMERS. Templates loaded in `env` afterwards (call it before
 * loading any) get:
 * - the settings `trimBlocks`, `lstripBlocks` and `extensions.loopControls` on, `autoescape` off, and
 *   `limits.maxRange` at most 100000 (the sandbox's `MAX_RANGE`); the other settings stay as they are;
 * - transformers' `tojson(x, ensure_ascii=False, indent=None, separators=None, sort_keys=False)` in place of Jinja2's:
 *   keys in insertion order, no HTML escaping, a plain string;
 * - the `{% generation %}...{% endgeneration %}` tag, which renders its body (Jinja2C++ does not compute the
 *   assistant token mask);
 * - read-only lists and dicts, as in `ImmutableSandboxedEnvironment`: `append`, `pop`, `update` and the other methods
 *   that change their receiver are unsafe attributes, undefined whose use fails with the sandbox's message;
 * - the globals `raise_exception(message)`, which fails the render with ErrorCode::TemplateRuntimeError and the
 *   message, and `strftime_now(format)`, the time of `options.now` formatted as Python's `strftime` does in the "C"
 *   locale.
 *
 * Templates loaded before keep what they were loaded with. It is not the rest of `apply_chat_template`: the host builds
 * the context (`messages`, `tools`, `add_generation_prompt`, `bos_token`, ...), and it is no security sandbox beyond what
 * is listed here: untrusted templates also need \ref UntrustedLimits.
 *
 * @param env     The environment to configure
 * @param options The clock of `strftime_now`
 */
JINJA2CPP_EXPORT void ConfigureForChatTemplates(TemplateEnv& env, ChatTemplateOptions options = {});

} // namespace v2
} // namespace jinja2

#endif // JINJA2CPP_CHAT_TEMPLATE_H
