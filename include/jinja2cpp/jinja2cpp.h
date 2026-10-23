#ifndef JINJA2CPP_JINJA2CPP_H
#define JINJA2CPP_JINJA2CPP_H

// The whole public API in one include. The JSON bindings (jinja2cpp/binding/) are left
// out: each needs its JSON library, so include the one you use next to this header.

#include <jinja2cpp/big_int_backend.h>       // IWYU pragma: export
#include <jinja2cpp/error_info.h>            // IWYU pragma: export
#include <jinja2cpp/exception.h>             // IWYU pragma: export
#include <jinja2cpp/filesystem_handler.h>    // IWYU pragma: export
#include <jinja2cpp/fwd.h>                   // IWYU pragma: export
#include <jinja2cpp/generic_list.h>          // IWYU pragma: export
#include <jinja2cpp/generic_list_iterator.h> // IWYU pragma: export
#include <jinja2cpp/make_generic_list.h>     // IWYU pragma: export
#include <jinja2cpp/reflected_value.h>       // IWYU pragma: export
#include <jinja2cpp/string_helpers.h>        // IWYU pragma: export
#include <jinja2cpp/template.h>              // IWYU pragma: export
#include <jinja2cpp/template_env.h>          // IWYU pragma: export
#include <jinja2cpp/user_callable.h>         // IWYU pragma: export
#include <jinja2cpp/utils/ordered_map.h>     // IWYU pragma: export
#include <jinja2cpp/value.h>                 // IWYU pragma: export
#include <jinja2cpp/version.h>               // IWYU pragma: export

#endif // JINJA2CPP_JINJA2CPP_H
