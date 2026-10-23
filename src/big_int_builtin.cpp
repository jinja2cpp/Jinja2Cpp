// The backend every TemplateEnv starts with, chosen by -DJINJA2CPP_WITH_BIGINT (docs/tasks/0272).
// The one file of the library that may include Boost.Multiprecision (docs/tasks/0230).
#include <jinja2cpp/big_int_backend.h>

#ifdef JINJA2CPP_WITH_BIGINT_BOOST
#include <jinja2cpp/big_int/boost_cpp_int.h>
#endif

namespace jinja2
{
inline namespace v2
{
const BigIntBackend* GetBuiltinBigIntBackend() noexcept
{
#ifdef JINJA2CPP_WITH_BIGINT_BOOST
    return BoostCppIntBackend();
#else
    return nullptr;
#endif
}
} // namespace v2
} // namespace jinja2
