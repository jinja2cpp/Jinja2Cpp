"""The loaders (jinja2.loaders), ported from Jinja2 3.1.6 (BSD-3-Clause, the Pallets Projects).

The engine reads two of them directly: a DictLoader's mapping, and a FileSystemLoader's search
path through Jinja2C++'s ConfinedFileSystem, which does not follow a symbolic link out of a
directory of the search path. Any other loader, Jinja2's own included, is asked through its
get_source(environment, name), and the engine honours the uptodate function it returns when the
environment's auto_reload is on. ChoiceLoader, PrefixLoader, FunctionLoader and PackageLoader
only combine get_source calls. ModuleLoader needs Jinja2's compiled templates, which jinja2cpp
has none of: it raises UnsupportedError.
"""

import importlib.util
import os
import posixpath
import sys
import typing as t
import zipimport
from importlib import import_module

from . import _jinja2cpp
from .exceptions import TemplateNotFound
from .exceptions import UnsupportedError


def split_template_path(template: str) -> t.List[str]:
    """Split a path into segments and perform a sanity check."""
    pieces = []
    for piece in template.split("/"):
        if os.path.sep in piece or (os.path.altsep and os.path.altsep in piece) or piece == os.path.pardir:
            raise TemplateNotFound(template)
        elif piece and piece != ".":
            pieces.append(piece)
    return pieces


def _confined(searchpath: str, filename: str) -> bool:
    """Whether `filename` is inside `searchpath` once links are followed, as the engine's own
    file system loader reads (a link that leaves the directory is not a template). get_source
    runs when a FileSystemLoader sits inside a ChoiceLoader or PrefixLoader, or in a fallback
    to Jinja2, and must not read what the engine refuses."""
    root = os.path.realpath(searchpath)
    real = os.path.realpath(filename)
    return real == root or real.startswith(root.rstrip(os.path.sep) + os.path.sep)


class BaseLoader:
    """Base class of loaders: a subclass implements get_source."""

    has_source_access = True

    def get_source(self, environment: t.Any, template: str) -> t.Tuple[str, t.Optional[str], t.Optional[t.Callable[[], bool]]]:
        if not self.has_source_access:
            raise RuntimeError(f"{type(self).__name__} cannot provide access to the source")
        raise TemplateNotFound(template)

    def list_templates(self) -> t.List[str]:
        raise TypeError("this loader cannot iterate over all templates")

    def load(self, environment: t.Any, name: str, globals: t.Optional[t.MutableMapping[str, t.Any]] = None) -> t.Any:
        """Loads the template `name` from get_source, bypassing the environment's cache."""
        source, filename, uptodate = self.get_source(environment, name)
        if not isinstance(environment, _jinja2cpp.Environment):
            # Jinja2's own environment (these loaders work in it too): its compile and its template class
            code = environment.compile(source, name, filename)
            return environment.template_class.from_code(environment, code, globals, uptodate)
        return environment._compile(source, name, filename, globals, environment.template_class)


class DictLoader(BaseLoader):
    """Loads templates from a dict of names to sources."""

    def __init__(self, mapping: t.Mapping[str, str]) -> None:
        self.mapping = mapping

    def get_source(self, environment: t.Any, template: str) -> t.Tuple[str, None, t.Callable[[], bool]]:
        if template in self.mapping:
            source = self.mapping[template]
            return source, None, lambda: source == self.mapping.get(template)
        raise TemplateNotFound(template)

    def list_templates(self) -> t.List[str]:
        return sorted(self.mapping)


class FileSystemLoader(BaseLoader):
    """Loads templates from directories of the file system."""

    def __init__(
        self,
        searchpath: t.Union[str, "os.PathLike[str]", t.Sequence[t.Union[str, "os.PathLike[str]"]]],
        encoding: str = "utf-8",
        followlinks: bool = False,
    ) -> None:
        if not isinstance(searchpath, (list, tuple)) or isinstance(searchpath, str):
            searchpath = [searchpath]  # type: ignore
        self.searchpath = [os.fspath(p) for p in searchpath]  # type: ignore
        self.encoding = encoding
        self.followlinks = followlinks

    def get_source(self, environment: t.Any, template: str) -> t.Tuple[str, str, t.Callable[[], bool]]:
        pieces = split_template_path(template)
        for searchpath in self.searchpath:
            filename = os.path.join(searchpath, *pieces)
            if os.path.isfile(filename) and _confined(searchpath, filename):
                break
        else:
            raise TemplateNotFound(template)
        with open(filename, encoding=self.encoding) as f:
            contents = f.read()
        mtime = os.path.getmtime(filename)

        def uptodate() -> bool:
            try:
                return os.path.getmtime(filename) == mtime
            except OSError:
                return False

        return contents, os.path.normpath(filename), uptodate

    def list_templates(self) -> t.List[str]:
        found = set()
        for searchpath in self.searchpath:
            walk_dir = os.walk(searchpath, followlinks=self.followlinks)
            for dirpath, _, filenames in walk_dir:
                for filename in filenames:
                    template = os.path.join(dirpath, filename)[len(searchpath) :].strip(os.path.sep).replace(os.path.sep, "/")
                    if template[:2] == "./":
                        template = template[2:]
                    found.add(template)
        return sorted(found)


if sys.version_info >= (3, 13):

    def _get_zipimporter_files(z: t.Any) -> t.Dict[str, object]:
        try:
            get_files = z._get_files
        except AttributeError as e:
            raise TypeError("This zip import does not have the required metadata to list templates.") from e
        return get_files()  # type: ignore[no-any-return]

else:

    def _get_zipimporter_files(z: t.Any) -> t.Dict[str, object]:
        try:
            files = z._files
        except AttributeError as e:
            raise TypeError("This zip import does not have the required metadata to list templates.") from e
        return files  # type: ignore[no-any-return]


class PackageLoader(BaseLoader):
    """Loads templates from a directory in a Python package, installed as a directory or in a zip file."""

    def __init__(self, package_name: str, package_path: "str" = "templates", encoding: str = "utf-8") -> None:
        package_path = os.path.normpath(package_path).rstrip(os.path.sep)
        # normpath preserves ".", which isn't valid in zip paths.
        if package_path == os.path.curdir:
            package_path = ""
        elif package_path[:2] == os.path.curdir + os.path.sep:
            package_path = package_path[2:]

        self.package_path = package_path
        self.package_name = package_name
        self.encoding = encoding

        # Make sure the package exists. This also makes namespace packages work, otherwise
        # get_loader returns None.
        import_module(package_name)
        spec = importlib.util.find_spec(package_name)
        assert spec is not None, "An import spec was not found for the package."
        loader = spec.loader
        assert loader is not None, "A loader was not found for the package."
        self._loader = loader
        self._archive = None
        if isinstance(loader, zipimport.zipimporter):
            self._archive = loader.archive
            pkgdir = next(iter(spec.submodule_search_locations))  # type: ignore
            template_root = os.path.join(pkgdir, package_path).rstrip(os.path.sep)
        else:
            roots: t.List[str] = []
            # One element for regular packages, multiple for namespace packages, or None for
            # a single module file.
            if spec.submodule_search_locations:
                roots.extend(spec.submodule_search_locations)
            # A single module file, use the parent directory instead.
            elif spec.origin is not None:
                roots.append(os.path.dirname(spec.origin))
            if not roots:
                raise ValueError(f"The {package_name!r} package was not installed in a way that PackageLoader understands.")
            for root in roots:
                root = os.path.join(root, package_path)
                if os.path.isdir(root):
                    template_root = root
                    break
            else:
                raise ValueError(f"PackageLoader could not find a {package_path!r} directory in the {package_name!r} package.")
        self._template_root = template_root

    def get_source(self, environment: t.Any, template: str) -> t.Tuple[str, str, t.Optional[t.Callable[[], bool]]]:
        # Use posixpath even on Windows to avoid "drive:" or UNC segments breaking out of the
        # search directory. Use normpath to convert Windows altsep to sep.
        p = os.path.normpath(posixpath.join(self._template_root, *split_template_path(template)))
        up_to_date: t.Optional[t.Callable[[], bool]]
        if self._archive is None:
            # Package is a directory.
            if not os.path.isfile(p):
                raise TemplateNotFound(template)
            with open(p, "rb") as f:
                source = f.read()
            mtime = os.path.getmtime(p)

            def up_to_date() -> bool:
                return os.path.isfile(p) and os.path.getmtime(p) == mtime

        else:
            # Package is a zip file.
            try:
                source = self._loader.get_data(p)  # type: ignore
            except OSError as e:
                raise TemplateNotFound(template) from e
            # Reported as always current: the zip does not change under the process.
            up_to_date = None
        return source.decode(self.encoding), p, up_to_date

    def list_templates(self) -> t.List[str]:
        results: t.List[str] = []
        if self._archive is None:
            # Package is a directory.
            offset = len(self._template_root)
            for dirpath, _, filenames in os.walk(self._template_root):
                dirpath = dirpath[offset:].lstrip(os.path.sep)
                results.extend(os.path.join(dirpath, name).replace(os.path.sep, "/") for name in filenames)
        else:
            files = _get_zipimporter_files(self._loader)
            # Package is a zip file.
            prefix = self._template_root[len(self._archive) :].lstrip(os.path.sep) + os.path.sep
            offset = len(prefix)
            for name in files:
                # Find names under the templates directory that aren't directories.
                if name.startswith(prefix) and name[-1] != os.path.sep:
                    results.append(name[offset:].replace(os.path.sep, "/"))
        results.sort()
        return results


class FunctionLoader(BaseLoader):
    """Loads templates from a function of the template name that returns the source, a tuple
    ``(source, filename, uptodatefunc)``, or None if the template does not exist."""

    def __init__(
        self,
        load_func: t.Callable[[str], t.Optional[t.Union[str, t.Tuple[str, t.Optional[str], t.Optional[t.Callable[[], bool]]]]]],
    ) -> None:
        self.load_func = load_func

    def get_source(self, environment: t.Any, template: str) -> t.Tuple[str, t.Optional[str], t.Optional[t.Callable[[], bool]]]:
        rv = self.load_func(template)
        if rv is None:
            raise TemplateNotFound(template)
        if isinstance(rv, str):
            return rv, None, None
        return rv


class PrefixLoader(BaseLoader):
    """A dict of loaders, each bound to a prefix of the template name, delimited by a slash or `delimiter`."""

    def __init__(self, mapping: t.Mapping[str, BaseLoader], delimiter: str = "/") -> None:
        self.mapping = mapping
        self.delimiter = delimiter

    def get_loader(self, template: str) -> t.Tuple[BaseLoader, str]:
        try:
            prefix, name = template.split(self.delimiter, 1)
            loader = self.mapping[prefix]
        except (ValueError, KeyError) as e:
            raise TemplateNotFound(template) from e
        return loader, name

    def get_source(self, environment: t.Any, template: str) -> t.Tuple[str, t.Optional[str], t.Optional[t.Callable[[], bool]]]:
        loader, name = self.get_loader(template)
        try:
            return loader.get_source(environment, name)
        except TemplateNotFound as e:
            # re-raise the exception with the correct filename here (the one that includes the prefix)
            raise TemplateNotFound(template) from e

    def list_templates(self) -> t.List[str]:
        result = []
        for prefix, loader in self.mapping.items():
            for template in loader.list_templates():
                result.append(prefix + self.delimiter + template)
        return result


class ChoiceLoader(BaseLoader):
    """Tries each of its loaders in turn: a template one does not find is looked for in the next."""

    def __init__(self, loaders: t.Sequence[BaseLoader]) -> None:
        self.loaders = loaders

    def get_source(self, environment: t.Any, template: str) -> t.Tuple[str, t.Optional[str], t.Optional[t.Callable[[], bool]]]:
        for loader in self.loaders:
            try:
                return loader.get_source(environment, template)
            except TemplateNotFound:
                pass
        raise TemplateNotFound(template)

    def list_templates(self) -> t.List[str]:
        found = set()
        for loader in self.loaders:
            found.update(loader.list_templates())
        return sorted(found)


class ModuleLoader(BaseLoader):
    """Loads precompiled templates: not supported, jinja2cpp compiles no Python modules."""

    has_source_access = False

    def __init__(self, path: t.Any) -> None:
        raise UnsupportedError("ModuleLoader loads Jinja2's compiled templates, which jinja2cpp does not have; use Jinja2 for it")
