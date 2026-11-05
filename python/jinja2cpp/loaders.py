"""DictLoader and FileSystemLoader (jinja2.loaders).

The engine reads these two directly: a DictLoader's mapping, and a FileSystemLoader's search
path through Jinja2C++'s ConfinedFileSystem, which does not follow a symbolic link out of a
directory of the search path. Any other loader, Jinja2's own included, is asked through its
get_source(environment, name).
"""

import os
import typing as t

from .exceptions import TemplateNotFound


def split_template_path(template: str) -> t.List[str]:
    """Split a path into segments and perform a sanity check."""
    pieces = []
    for piece in template.split("/"):
        if os.path.sep in piece or (os.path.altsep and os.path.altsep in piece) or piece == os.path.pardir:
            raise TemplateNotFound(template)
        elif piece and piece != ".":
            pieces.append(piece)
    return pieces


class BaseLoader:
    """Base class of loaders: a subclass implements get_source."""

    has_source_access = True

    def get_source(self, environment: t.Any, template: str) -> t.Tuple[str, t.Optional[str], t.Optional[t.Callable[[], bool]]]:
        raise TemplateNotFound(template)

    def list_templates(self) -> t.List[str]:
        raise TypeError("this loader cannot iterate over all templates")

    def load(self, environment: t.Any, name: str, globals: t.Optional[t.MutableMapping[str, t.Any]] = None) -> t.Any:
        return environment.get_template(name, globals=globals)


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
            if os.path.isfile(filename):
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
