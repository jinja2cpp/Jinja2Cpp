"""jinja2.meta: what a template parsed by ``Environment.parse`` refers to.

Ported from Jinja2 3.1.6 (BSD-3-Clause, the Pallets Projects); the engine finds the names.
"""

import typing as t

from .environment import ParsedTemplate
from .environment import Template


def _is_jinja2_ast(ast: t.Any) -> bool:
    """A node tree of Jinja2 (a delegating environment's ``parse``)."""
    return type(ast).__module__ == "jinja2.nodes"


def _jinja2_tree(ast: t.Any) -> t.Any:
    """Jinja2's node tree for `ast` when it is one, or the template of Jinja2 (a fallback template) parsed again; else None."""
    if _is_jinja2_ast(ast):
        return ast
    template = ast._template if isinstance(ast, ParsedTemplate) else ast
    if isinstance(template, Template) and "_jt" in template.__dict__:
        return template._jinja2_ast()  # type: ignore[attr-defined]
    return None


def _template_of(ast: t.Any) -> Template:
    if isinstance(ast, ParsedTemplate):
        return ast._template
    if isinstance(ast, Template):
        return ast
    raise TypeError(f"expected the result of Environment.parse, not {type(ast).__name__}")


def find_undeclared_variables(ast: ParsedTemplate) -> t.Set[str]:
    """Returns a set of all variables in the AST that will be looked up from the context at runtime.

    Because at compile time it's not known which variables will be used depending on the path the
    execution takes at runtime, all variables are returned::

        >>> from jinja2cpp import Environment, meta
        >>> env = Environment()
        >>> ast = env.parse('{% set foo = 42 %}{{ bar + foo }}')
        >>> meta.find_undeclared_variables(ast) == {'bar'}
        True

    Templates the template includes, imports or extends are not looked into.
    """
    tree = _jinja2_tree(ast)
    if tree is not None:
        import jinja2.meta

        return jinja2.meta.find_undeclared_variables(tree)
    template = _template_of(ast)
    # The engine leaves out its own globals; the environment's are Python's dict
    declared = template.globals
    return {name for name in template._undeclared() if name not in declared}


def find_referenced_templates(ast: ParsedTemplate) -> t.Iterator[t.Optional[str]]:
    """Finds all the referenced templates from the AST.

    This will return an iterator over all the hardcoded template extensions, inclusions and
    imports. If dynamic inheritance or inclusion is used, `None` will be yielded.

        >>> from jinja2cpp import Environment, meta
        >>> env = Environment()
        >>> ast = env.parse('{% extends "layout.html" %}{% include helper %}')
        >>> list(meta.find_referenced_templates(ast))
        ['layout.html', None]

    This function is useful for dependency tracking, for example to build a database of
    dependencies for the templates.
    """
    tree = _jinja2_tree(ast)
    if tree is not None:
        import jinja2.meta

        yield from jinja2.meta.find_referenced_templates(tree)
        return
    yield from _template_of(ast)._referenced()
