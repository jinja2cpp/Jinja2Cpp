"""Jinja2C++ as a Python module: Jinja2's API rendered by a C++ engine (docs/python.md).

``import jinja2cpp as jinja2`` works for the subset docs/python.md lists.
"""

from markupsafe import Markup as Markup
from markupsafe import escape as escape

from . import ext as ext
from . import meta as meta
from .environment import Environment as Environment
from .environment import Template as Template
from .environment import TemplateModule as TemplateModule
from .environment import TemplateStream as TemplateStream
from .exceptions import FilterArgumentError as FilterArgumentError
from .exceptions import SecurityError as SecurityError
from .exceptions import TemplateAssertionError as TemplateAssertionError
from .exceptions import TemplateError as TemplateError
from .exceptions import TemplateNotFound as TemplateNotFound
from .exceptions import TemplateRuntimeError as TemplateRuntimeError
from .exceptions import TemplatesNotFound as TemplatesNotFound
from .exceptions import TemplateSyntaxError as TemplateSyntaxError
from .exceptions import UndefinedError as UndefinedError
from .exceptions import UnsupportedError as UnsupportedError
from .loaders import BaseLoader as BaseLoader
from .loaders import ChoiceLoader as ChoiceLoader
from .loaders import DictLoader as DictLoader
from .loaders import FileSystemLoader as FileSystemLoader
from .loaders import FunctionLoader as FunctionLoader
from .loaders import ModuleLoader as ModuleLoader
from .loaders import PackageLoader as PackageLoader
from .loaders import PrefixLoader as PrefixLoader
from .runtime import ChainableUndefined as ChainableUndefined
from .runtime import DebugUndefined as DebugUndefined
from .runtime import StrictUndefined as StrictUndefined
from .runtime import Undefined as Undefined
from .utils import is_undefined as is_undefined
from .utils import select_autoescape as select_autoescape

__version__ = "2.0.0"
