#!/usr/bin/env python

"""
playing/sqla_metaclass_test.py

===============================================================================

    Copyright (C) 2012-2020 Rudolf Cardinal (rudolf@pobox.com).

    This file is part of CamCOPS.

    CamCOPS is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    CamCOPS is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with CamCOPS. If not, see <https://www.gnu.org/licenses/>.

===============================================================================

Metaclasses in general:
- https://www.python.org/dev/peps/pep-3115/
- http://eli.thegreenplace.net/2011/08/14/python-metaclasses-by-example
- https://stackoverflow.com/questions/27258557/metaclass-arguments-for-python-3-x
- http://python-3-patterns-idioms-test.readthedocs.io/en/latest/Metaprogramming.html#using-init-vs-new-in-metaclasses
MOST IMPORTANTLY FOR PYTHON 3:
    Use
        class X(metaclass=Y)
    not
        class X(object):
            __metaclass__ = Y  # WILL BE IGNORED
THE OVERALL SEQUENCE:
    - suppose class Thing has a metaclass MetaThing
        class Thing(object, metaclass=MetaThing):
            # AT THIS POINT IN EVALUATION, MetaThing.__prepare__() is called
            # (PEP 3115: "This attribute is named __prepare__ , which is
            # invoked as a function before the evaluation of the class body.")
            # THE POINT OF __prepare__ IS TO MODIFY THE BASE CLASS DICTIONARY.
            # See http://python-3-patterns-idioms-test.readthedocs.io/en/latest/Metaprogramming.html#the-prepare-metamethod
            someattr = 99
            def somefunc(self):
                return "hello"
        # NOW, WHEN THE CLASS DEFINITION HAS BEEN READ, MetaThing.__new__()
        # is called
Metaclass __new__ vs __init__:
- https://stackoverflow.com/questions/1840421/is-there-any-reason-to-choose-new-over-init-when-defining-a-metaclass
- https://docs.python.org/3/reference/datamodel.html#basic-customization
"""  # noqa

# =============================================================================
# Imports
# =============================================================================

import logging
from typing import Any, Dict, Tuple, Type

from cardinal_pythonlib.logs import (
    BraceStyleAdapter,
    main_only_quicksetup_rootlogger,
)
from cardinal_pythonlib.reprfunc import auto_repr
from cardinal_pythonlib.sqlalchemy.orm_inspect import get_orm_column_names
from sqlalchemy.ext.declarative import DeclarativeMeta
from sqlalchemy.sql.schema import Column
from sqlalchemy.sql.sqltypes import Integer

from camcops_server.cc_modules.cc_sqla_coltypes import (
    CamcopsColumn,
    PermittedValueChecker,
    permitted_value_failure_msgs,
    permitted_values_ok,
)
from camcops_server.cc_modules.cc_sqlalchemy import Base

log = BraceStyleAdapter(logging.getLogger(__name__))


# =============================================================================
# 1. Explore the metaclass system in detail
# =============================================================================

MODIFY_CLASS = True

if __name__ == '__main__':
    # Before classes are even declared:
    main_only_quicksetup_rootlogger()
    log.info("Module starting to load.")


def add_multiple_fields(cls: Type, first: int, last: int, prefix: str,
                        coltype: Type,
                        colkwargs: Dict[str, Any] = None) -> None:
    colkwargs = {} if colkwargs is None else colkwargs  # type: Dict[str, Any]
    for n in range(first, last + 1):
        nstr = str(n)
        colname = prefix + nstr
        setattr(cls, colname, Column(colname, coltype, **colkwargs))


class MetaSomeThing(DeclarativeMeta):
    # The metaclass must be a subclass of DeclarativeMeta, because
    # (1) The SQLAlchemy example:
    #     https://bitbucket.org/zzzeek/sqlalchemy/wiki/UsageRecipes/MultiKeyIndexesInMixins  # noqa
    # (2) It crashes otherwise:
    #       class SomeThing(Base, metaclass=MetaSomeThing):
    #       TypeError: metaclass conflict: the metaclass of a derived class
    #       must be a (non-strict) subclass of the metaclasses of all its bases

    log.debug("MetaSomeThing: first line of declaration")

    # noinspection PyMethodParameters
    def __prepare__(name: str,
                    bases: Tuple[Type, ...],
                    **kwds: Dict[str, Any]) -> Dict:
        """

        Args:
            name: name of the class being created, e.g. 'SomeThing'
            bases: tuple of the new class's base classes, e.g.
                (<class 'sqlalchemy.ext.declarative.api.Base'>,)
            **kwds: arbitrary keyword arguments specified by the user in the
                class definition

        Returns:
            a dictionary-like object to store the class member definitions;
            see https://www.python.org/dev/peps/pep-3115/
        """
        log.debug("MetaSomething.__prepare__: name={!r}, bases={!r}, "
                  "kwds={!r}".format(name, bases, kwds))
        return dict()

    def __new__(mcs: Type,
                name: str,
                bases: Tuple[Type, ...],
                classdict: Dict[str, Any]) -> Type:
        """
        Args:
            mcs: this metaclass, e.g.
                <class '__main__.MetaSomeThing'>
            name: the name of the new class, e.g.
                'SomeThing'
            bases: tuple of the new class's base classes, e.g.
                (<class 'sqlalchemy.ext.declarative.api.Base'>,)
            classdict: dictionary mapping attribute names to attributes, e.g.
                {
                    '__qualname__': 'SomeThing',
                    '__tablename__': 'some_table',
                    '__init__':
                        <function SomeThing.__init__ at 0x7fd8065647b8>,
                    'a': Column('a', Integer(), table=None, primary_key=True,
                                nullable=False),
                    'somefunc':
                        <function SomeThing.somefunc at 0x7fd806564840>,
                    '__module__': '__main__',
                    'c': Column('c', Integer(), table=None)
                }

        Returns:
            the class being created, e.g. <class '__main__.SomeThing'>
        """
        log.debug("MetaSomeThing.__new__: mcs={0!r}, name={1!r}, bases={2!r}, "
                  "classdict={3!r}".format(mcs, name, bases, classdict))
        if MODIFY_CLASS:
            log.debug("MetaSomeThing.__new__: adding b_from_new_classdict, "
                      "via classdict")
            classdict['b_from_new_classdict'] = Column(
                "b_from_new_classdict", Integer)
        retval = super().__new__(mcs, name, bases, classdict)
        if MODIFY_CLASS:
            log.debug("MetaSomeThing.__new__: adding b_from_new_cls, via cls")
            retval.b_from_new_cls = Column("b_from_new_cls", Integer)
        # ... DOES in this case need the "cls" parameter
        log.debug("MetaSomeThing.__new__: returning {0!r}", retval)
        return retval

    def __init__(cls: Type,
                 name: str,
                 bases: Tuple[Type, ...],
                 classdict: Dict[str, Any]) -> None:
        """
        Args:
            cls: the class being created (not the metaclass), e.g.
                <class '__main__.SomeThing'>
            name: the name of the new class, e.g.
                'SomeThing'
            bases: tuple of the new class's base classes, e.g.
                (<class 'sqlalchemy.ext.declarative.api.Base'>,)
            classdict: dictionary mapping attribute names to attributes, e.g.
                {
                    '__qualname__': 'SomeThing',
                    '__tablename__': 'some_table',
                    '__init__':
                        <function SomeThing.__init__ at 0x7fd8065647b8>,
                    'a': Column('a', Integer(), table=None, primary_key=True,
                                nullable=False),
                    'somefunc':
                        <function SomeThing.somefunc at 0x7fd806564840>,
                    '__module__': '__main__',
                    'c': Column('c', Integer(), table=None)
                }
        """
        log.debug("MetaSomeThing.__init__: cls={0!r}, name={1!r}, "
                  "bases={2!r}, classdict={3!r}".format(cls, name, bases,
                                                        classdict))
        if getattr(cls, '_decl_class_registry', None) is None:
            log.debug("... says no")
            return
        if MODIFY_CLASS:
            log.debug("MetaSomeThing.__init__: attempting to add "
                      "b_from_init_classdict by modifying classdict "
                      "but THIS WILL NOT WORK")
            classdict['b_from_init_classdict'] = Column(
                "b_from_init_classdict", Integer)
            log.debug("MetaSomeThing.__init__: adding b_from_init_cls by "
                      "modifying cls directly; this will work")
            cls.b_from_init_cls = Column("b_from_init_cls", Integer)
            add_multiple_fields(cls, first=1, last=5, prefix="d",
                                coltype=Integer)
        super().__init__(name, bases, classdict)
        # ... DOES NOT need the "cls" parameter


class SomeThing(Base, metaclass=MetaSomeThing):

    log.debug("SomeThing: first line of declaration")

    __tablename__ = "some_table"
    a = Column("a", Integer, primary_key=True)
    c = Column("c", Integer)

    def __init__(self, *args, **kwargs):
        log.debug("SomeThing.__init__: args={0!r}, kwargs={1!r}", args, kwargs)

    def somefunc(self):
        self.a = 5
        # log.info("b_from_init_classdict = {}", self.b_from_init_classdict)
        log.info("b_from_init_cls = {}", self.b_from_init_cls)
        log.info("b_from_new_classdict = {}", self.b_from_new_classdict)
        log.info("b_from_new_cls = {}", self.b_from_new_cls)


def act_with(blah: SomeThing) -> None:
    log.debug(blah.d4)
    # log.debug(blah.does_not_exist)  # the PyCharm inspector doesn't complain!


# =============================================================================
# 2. Now, how simple can this look?
# =============================================================================

class SimpleTaskMetaclass(DeclarativeMeta):
    # noinspection PyInitNewSignature
    def __init__(cls, name, bases, classdict):
        add_multiple_fields(cls, first=1, last=5, prefix="q", coltype=Integer)
        super().__init__(name, bases, classdict)


class SimpleTask(Base, metaclass=SimpleTaskMetaclass):
    __tablename__ = "table_for_simple_task"
    some_pk = Column("some_pk", Integer, primary_key=True)


# =============================================================================
# 3. And for tasks, where we may want to mix in things like patient, clinician,
#    respondent?
# =============================================================================

class PretendTaskBase(object):
    has_extra_bits = False

    @staticmethod
    def quick_check_static_inheritance() -> str:
        return "PretendTaskBase.quick_check_static_inheritance"


class ExtraMixin(object):
    has_extra_bits = True
    x = Column("x", Integer)
    y = Column("y", Integer)
    z = Column("z", Integer)


class ComplexTask(ExtraMixin, PretendTaskBase, Base,
                  metaclass=SimpleTaskMetaclass):
    # The LEFT-MOST HAS PRIORITY WHEN ATTRIBUTES CLASH.
    # Similarly, the left-most comes first in the Method Resolution Order.
    __tablename__ = "table_for_complex_task"
    some_pk = Column("some_pk", Integer, primary_key=True)

    checkme = CamcopsColumn(
        "checkme_internal", Integer,
        # NB: different SQL name from attr name (this tests whether we have
        # the naming system right!).
        permitted_value_checker=PermittedValueChecker(
            not_null=True,
            # permitted_values=[3, 4],
            maximum=3
        )
    )

    def __repr__(self) -> str:
        return auto_repr(self)

    # This override works, but the type checker complains about signature
    # mismatch:

    # def quick_check_static_inheritance(self) -> str:
    #     return "ComplexTask.quick_check_static_inheritance: some_pk = " \
    #            "{}".format(self.some_pk)


# =============================================================================
# Main
# =============================================================================

if __name__ == '__main__':
    log.info("Classes declared; doing something else.")
    log.info("SomeThing == {!r}", SomeThing)
    log.info("dir(SomeThing) == {!r}", dir(SomeThing))
    log.info("get_orm_column_names(SomeThing) == {}",
             get_orm_column_names(SomeThing, sort=True))
    st = SomeThing()
    st.somefunc()
    # Now, which columns does the PyCharm type checker/autocomplete find?
    #                           Column?     Autocomplete?   Inspect code OK?
    #   a                       YES         YES             YES
    #   b_from_init_classdict   no          -               -
    #   b_from_init_cls         YES         YES             YES
    #   b_from_new_classdict    YES         NO              YES
    #   b_from_new_cls          YES         YES             YES
    #   c                       YES         YES             YES
    #   d1 to d5                YES         NO              YES
    #   nonexistent             -           -               [yes!]
    log.debug("{}", st.a)
    log.debug("{}", st.b_from_init_cls)
    log.debug("{}", st.b_from_new_classdict)
    log.debug("{}", st.b_from_new_cls)
    log.debug("{}", st.c)
    log.debug("{}", st.d3)
    act_with(st)

    x = SimpleTask()
    log.info("get_orm_column_names(SimpleTask) == {}",
             get_orm_column_names(SimpleTask, sort=True))

    ct = ComplexTask()
    log.info("get_orm_column_names(ComplexTask) == {}",
             get_orm_column_names(ComplexTask, sort=True))
    log.info("ct.has_extra_bits = {}", ct.has_extra_bits)
    log.info("ComplexTask.__mro__ = {}", ComplexTask.__mro__)

    log.debug(ct.quick_check_static_inheritance())

    log.info("{!r}", ct)
    log.info("permitted_values_ok(ct) = {}", permitted_values_ok(ct))
    log.info("permitted_value_failure_msgs(ct) = {}",
             permitted_value_failure_msgs(ct))

    ct.checkme = 5
    log.info("{!r}", ct)
    log.info("permitted_values_ok(ct) = {}", permitted_values_ok(ct))
    log.info("permitted_value_failure_msgs(ct) = {}",
             permitted_value_failure_msgs(ct))
