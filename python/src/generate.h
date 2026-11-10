#ifndef JINJA2CPP_PYTHON_GENERATE_H
#define JINJA2CPP_PYTHON_GENERATE_H

#include "common.h"

namespace jinja2cpp_py
{
// Template.generate (docs/tasks/0436): the render runs as the consumer asks for items
void BindGenerate(nb::module_& module);
// What the poll of a render on a generate() worker does besides PollSignals: hand the GIL to the
// consumer that waits for it (to check its signals), and end a render the consumer cancelled.
// Nothing on any other thread
void PollLazyRender();
} // namespace jinja2cpp_py

#endif // JINJA2CPP_PYTHON_GENERATE_H
