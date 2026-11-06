#ifndef JINJA2CPP_SRC_PUBLIC_HOST_OBJECT_H
#define JINJA2CPP_SRC_PUBLIC_HOST_OBJECT_H

#include "internal_value.h"

#include <jinja2cpp/host_object.h>
#include <jinja2cpp/value.h>

#include <memory>

// The engine's side of the public host object (jinja2cpp/host_object.h, docs/tasks/0357 B1/B2): one bridge class
// implements the engine's private interface (host_object.h, IEngineHostObject) over a public IHostObject, converting
// Value and InternalValue at each operation. A host object is a GenericMap whose accessor is the bridge
// (MakeHostValue); the same object serves as the engine's accessor, so a host value entering a render costs no
// allocation and the value the engine hands back is the host's own GenericMap

namespace jinja2
{
inline namespace v2
{
struct UndefinedValue;

// A value the host handed over as a value of the engine (internal_value.cpp)
InternalValue Value2IntValue(Value&& val);

// An undefined value of the engine as MakeUndefined describes it
Value UndefinedToHost(const UndefinedValue& undefined);

// A value of the engine as a public host object or a callable that keeps undefined values gets it: an undefined value
// is MakeUndefined's, anything else as IntValue2Value gives it
Value ToHostValue(const InternalValue& value);

// The call of `object` as a callable of the engine (a UserCallable declared with ArgInfo::VarArgs and
// ArgInfo::VarKwArgs that keeps undefined values), so that its arguments are lent the way those of a host's callable
// are. Defined with the call scope in internal_value.cpp
InternalValue MakeHostObjectCallable(std::shared_ptr<const IHostObject> object);
} // namespace v2
} // namespace jinja2

#endif // JINJA2CPP_SRC_PUBLIC_HOST_OBJECT_H
