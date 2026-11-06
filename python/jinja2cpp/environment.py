"""Environment and Template (jinja2.environment): the Python layer over the native classes.

The engine's classes (``_jinja2cpp.Environment`` and ``Template``) render; what needs no engine
lives here, so that a Python subclass of Environment works as in Jinja2 and the template classes
of an environment are its ``template_class``. Ported from Jinja2 3.1.6 (BSD-3-Clause, the Pallets
Projects) where the code is Jinja2's: the stream, select_template and list_templates.
"""

import typing as t
from functools import partial
from itertools import chain

from markupsafe import Markup

from . import _jinja2cpp
from .exceptions import TemplateNotFound
from .exceptions import TemplatesNotFound
from .exceptions import UndefinedError
from .runtime import Undefined

concat = "".join


class TemplateStream:
    """A stream of the rendered template: like a generator that can buffer several items.

    The output is not buffered by default. With a buffer size of 5, five items are combined into
    one string, which is useful to stream a big template to a client that flushes at each item.
    """

    def __init__(self, gen: t.Iterator[str]) -> None:
        self._gen = gen
        self.disable_buffering()

    def dump(self, fp: t.Union[str, t.IO[bytes]], encoding: t.Optional[str] = None, errors: t.Optional[str] = "strict") -> None:
        """Dump the complete stream into a file or file-like object.

        Strings are written, unless an `encoding` is given. ``Template('Hello {{ name }}!').stream(name='foo').dump('hello.html')``
        """
        close = False
        if isinstance(fp, str):
            if encoding is None:
                encoding = "utf-8"
            real_fp: t.IO[bytes] = open(fp, "wb")
            close = True
        else:
            real_fp = fp
        try:
            if encoding is not None:
                iterable = (x.encode(encoding, errors) for x in self)  # type: ignore
            else:
                iterable = self  # type: ignore
            if hasattr(real_fp, "writelines"):
                real_fp.writelines(iterable)
            else:
                for item in iterable:
                    real_fp.write(item)
        finally:
            if close:
                real_fp.close()

    def disable_buffering(self) -> None:
        """Disable the output buffering."""
        self._next = partial(next, self._gen)
        self.buffered = False

    def _buffered_generator(self, size: int) -> t.Iterator[str]:
        buf: t.List[str] = []
        c_size = 0
        push = buf.append
        while True:
            try:
                while c_size < size:
                    c = next(self._gen)
                    push(c)
                    if c:
                        c_size += 1
            except StopIteration:
                if not c_size:
                    return
            yield concat(buf)
            del buf[:]
            c_size = 0

    def enable_buffering(self, size: int = 5) -> None:
        """Enable buffering.  Buffer `size` items before yielding them."""
        if size <= 1:
            raise ValueError("buffer size too small")
        self.buffered = True
        self._next = partial(next, self._buffered_generator(size))

    def __iter__(self) -> "TemplateStream":
        return self

    def __next__(self) -> str:
        return self._next()  # type: ignore


class TemplateModule:
    """What a template exports, as Jinja2's ``Template.module`` and ``make_module``.

    The exported variables and macros are attributes. ``str()`` is the output of the template's
    top level. A macro stays callable for as long as it is referenced, whatever happens to the
    module, the template and the environment.
    """

    def __init__(self, template: "Template", body: str, exports: t.Dict[str, t.Any]) -> None:
        self._body = body
        self.__dict__.update(exports)
        self.__name__ = template.name

    def __html__(self) -> Markup:
        return Markup(self._body)

    def __str__(self) -> str:
        return self._body

    def __repr__(self) -> str:
        if self.__name__ is None:
            name = f"memory:{id(self):x}"
        else:
            name = repr(self.__name__)
        return f"<{type(self).__name__} {name}>"


class ParsedTemplate:
    """The result of ``Environment.parse``: opaque, for the functions of jinja2cpp.meta."""

    __slots__ = ("_template",)

    def __init__(self, template: "Template") -> None:
        self._template = template

    def __repr__(self) -> str:
        return f"<ParsedTemplate {self._template.name!r}>"


class Template(_jinja2cpp.Template):
    """A compiled template: ``render``, ``generate``, ``stream`` and ``make_module``."""

    # The class Template(source, ...) makes its environment of (set below)
    environment_class: t.Type["Environment"]

    def __init__(self, source: t.Any, **kwargs: t.Any) -> None:
        super().__init__(source, self.environment_class(**kwargs))

    def generate(self, *args: t.Any, **kwargs: t.Any) -> t.Iterator[str]:
        """Render the template as a generator of strings: one item for now, the whole output."""
        output = self.render(*args, **kwargs)
        if output:
            yield output

    def stream(self, *args: t.Any, **kwargs: t.Any) -> TemplateStream:
        """Like ``generate``, as a TemplateStream."""
        return TemplateStream(self.generate(*args, **kwargs))

    def make_module(self, vars: t.Optional[t.Dict[str, t.Any]] = None, shared: bool = False, locals: t.Optional[t.Mapping[str, t.Any]] = None) -> TemplateModule:
        """Render the template with `vars` and return what it exports (docs/python.md for ``shared``)."""
        params: t.Dict[str, t.Any] = {}
        if not shared:
            params.update(self.globals)
        if vars:
            params.update(vars)
        if locals:
            params.update(locals)
        body, exports = self._make_module(params)
        return TemplateModule(self, body, exports)

    @property
    def module(self) -> TemplateModule:
        """The module of the template with no variables, made once."""
        module = self.__dict__.get("_module")
        if module is None:
            module = self.__dict__["_module"] = self.make_module()
        return module

    def __repr__(self) -> str:
        if self.name is None:
            name = f"memory:{id(self):x}"
        else:
            name = repr(self.name)
        return f"<{type(self).__name__} {name}>"


# Environment's parameters in Jinja2's order, for the positional form
_ENVIRONMENT_PARAMETERS = (
    "block_start_string",
    "block_end_string",
    "variable_start_string",
    "variable_end_string",
    "comment_start_string",
    "comment_end_string",
    "line_statement_prefix",
    "line_comment_prefix",
    "trim_blocks",
    "lstrip_blocks",
    "newline_sequence",
    "keep_trailing_newline",
    "extensions",
    "optimized",
    "undefined",
    "finalize",
    "autoescape",
    "loader",
    "cache_size",
    "auto_reload",
    "bytecode_cache",
    "enable_async",
)


class Environment(_jinja2cpp.Environment):
    """The configuration and the loaded templates; subclass it as Jinja2's."""

    def __init__(self, *args: t.Any, **kwargs: t.Any) -> None:
        if len(args) > len(_ENVIRONMENT_PARAMETERS):
            raise TypeError(f"Environment() takes at most {len(_ENVIRONMENT_PARAMETERS)} positional arguments but {len(args)} were given")
        for name, value in zip(_ENVIRONMENT_PARAMETERS, args):
            if name in kwargs:
                raise TypeError(f"Environment() got multiple values for argument '{name}'")
            kwargs[name] = value
        super().__init__(**kwargs)

    #: the class of the templates ``from_string`` and ``get_template`` make
    template_class: t.Type[Template] = Template

    def from_string(self, source: t.Any, globals: t.Optional[t.Mapping[str, t.Any]] = None, template_class: t.Optional[t.Type[Template]] = None) -> Template:
        """Load a template from a source string without using ``loader``."""
        return self._from_string(source, globals, template_class or self.template_class)  # type: ignore[no-any-return]

    def join_path(self, template: str, parent: str) -> str:
        """Join a template with the parent. By default all the lookups are relative to the loader root, so this is a noop; override it to change that."""
        return template

    def _load_template(self, name: t.Any, globals: t.Optional[t.Mapping[str, t.Any]]) -> Template:
        return self._load(name, globals, self.template_class)  # type: ignore[no-any-return]

    def get_template(self, name: t.Any, parent: t.Optional[str] = None, globals: t.Optional[t.Mapping[str, t.Any]] = None) -> Template:
        """Load a template by name with ``loader`` and return it."""
        if isinstance(name, _jinja2cpp.Template):
            return name
        if parent is not None:
            name = self.join_path(name, parent)
        return self._load_template(name, globals)

    def select_template(
        self, names: t.Iterable[t.Union[str, Template]], parent: t.Optional[str] = None, globals: t.Optional[t.Mapping[str, t.Any]] = None
    ) -> Template:
        """The first of the templates that exists; TemplatesNotFound if none does."""
        if isinstance(names, Undefined):
            names._fail_with_undefined_error()
        if not names:
            raise TemplatesNotFound(message="Tried to select from an empty list of templates.")
        for name in names:
            if isinstance(name, _jinja2cpp.Template):
                return name  # type: ignore[return-value]
            if parent is not None:
                name = self.join_path(name, parent)
            try:
                return self._load_template(name, globals)
            except (TemplateNotFound, UndefinedError):
                pass
        raise TemplatesNotFound(names)  # type: ignore[arg-type]

    def get_or_select_template(
        self, template_name_or_list: t.Union[str, Template, t.List[t.Union[str, Template]]], parent: t.Optional[str] = None, globals: t.Optional[t.Mapping[str, t.Any]] = None
    ) -> Template:
        """``get_template`` for a name or a template, ``select_template`` for a list."""
        if isinstance(template_name_or_list, (str, Undefined)):
            return self.get_template(template_name_or_list, parent, globals)
        elif isinstance(template_name_or_list, _jinja2cpp.Template):
            return template_name_or_list  # type: ignore[return-value]
        return self.select_template(template_name_or_list, parent, globals)

    def list_templates(self, extensions: t.Optional[t.Collection[str]] = None, filter_func: t.Optional[t.Callable[[str], bool]] = None) -> t.List[str]:
        """The names of the templates the loader can give, sorted, filtered by the extensions or the function."""
        assert self.loader is not None, "No loader configured."
        names = self.loader.list_templates()
        if extensions is not None:
            if filter_func is not None:
                raise TypeError("either extensions or filter_func can be passed, but not both")

            def filter_func(x: str) -> bool:
                return "." in x and x.rsplit(".", 1)[1] in extensions  # type: ignore[operator]

        if filter_func is not None:
            names = [name for name in names if filter_func(name)]
        return names  # type: ignore[no-any-return]

    def parse(self, source: str, name: t.Optional[str] = None, filename: t.Optional[str] = None) -> ParsedTemplate:
        """Parse a source for ``jinja2cpp.meta``. Unlike Jinja2's, it also checks what the compile checks (an unknown filter or test raises)."""
        return ParsedTemplate(self._compile(source, name, filename, None, self.template_class))


Template.environment_class = Environment
