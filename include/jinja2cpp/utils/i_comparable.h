#ifndef JINJA2CPP_ICOMPARABLE_H
#define JINJA2CPP_ICOMPARABLE_H

#include <jinja2cpp/fwd.h> // IWYU pragma: export
#include <jinja2cpp/config.h>

JINJA2CPP_SUPPRESS_DLL_INTERFACE_WARNINGS_BEGIN

namespace jinja2
{
inline namespace v2
{

/*!
 * \brief Equality comparison for the extension interfaces (list and map accessors, enumerators, filesystem handlers)
 *
 * Every extension interface derives from it virtually. The default `IsEqual` is identity: an object equals only
 * itself. Override it when two distinct objects can represent the same data (for instance, two accessors over the
 * same container).
 */
struct JINJA2CPP_EXPORT IComparable
{
    virtual ~IComparable() = default;

    /*!
     * \brief Compares this object with another one
     *
     * @return `true` if the objects are equal. The default implementation compares addresses.
     */
    [[nodiscard]] virtual bool IsEqual(const IComparable& other) const { return this == &other; }
};

} // namespace v2
} // namespace jinja2

JINJA2CPP_SUPPRESS_DLL_INTERFACE_WARNINGS_END

#endif // JINJA2CPP_ICOMPARABLE_H
