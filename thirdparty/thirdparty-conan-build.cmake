# find_package(expected-lite)
# find_package(variant-lite)
# find_package(optional-lite)
# find_package(value-ptr-lite)
# find_package(boost)

set (JINJA2_PRIVATE_LIBS_INT boost PARENT_SCOPE)
set (JINJA2_PUBLIC_LIBS_INT expected-lite variant-lite value-ptr-lite optional-lite PARENT_SCOPE)
