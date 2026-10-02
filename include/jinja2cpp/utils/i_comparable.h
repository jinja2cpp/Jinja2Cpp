#ifndef JINJA2CPP_ICOMPARABLE_H
#define JINJA2CPP_ICOMPARABLE_H

#include <jinja2cpp/config.h>

namespace jinja2
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

} // namespace jinja2

#endif // JINJA2CPP_ICOMPARABLE_H
