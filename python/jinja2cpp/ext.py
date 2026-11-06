"""The extensions jinja2cpp implements natively, by Jinja2's names.

Environment(extensions=[...]) takes these, Jinja2's own classes or their dotted names
("jinja2.ext.do", "jinja2.ext.loopcontrols").
"""


class Extension:
    """Base class of the extensions; custom extensions are not supported yet."""


class ExprStmtExtension(Extension):
    """The {% do %} statement."""


class LoopControlExtension(Extension):
    """{% break %} and {% continue %} in loops."""


do = ExprStmtExtension
loopcontrols = LoopControlExtension
