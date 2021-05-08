#!/usr/bin/env python

"""
playing/sqla_concrete_inheritance_2.py

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

**Test SQLAlchemy inheritance.**

"""
# https://stackoverflow.com/questions/26724897/adjacency-list-abstract-base-class-inheritance-used-in-relationship

from sqlalchemy import Column, String, Integer, create_engine, ForeignKey, Float
from sqlalchemy.orm import configure_mappers, Session, relationship
from sqlalchemy.ext.declarative import declarative_base
from sqlalchemy.orm.collections import attribute_mapped_collection
from sqlalchemy.ext.declarative import declared_attr, AbstractConcreteBase
from camcops_server.cc_modules.cc_sqla_coltypes import BoolColumn, CamcopsColumn


CREATE = False
CREATE_BOTH = False
ADD = False
PREPARE = True

WITH_CAMCOPS_COLUMNS = True


Base = declarative_base()


class TreeNode(AbstractConcreteBase, Base):
    id = Column(Integer, primary_key=True)
    name = Column(String)
    if WITH_CAMCOPS_COLUMNS:
        counter = CamcopsColumn("counter", Integer)
        flipswitch = BoolColumn("flipswitch")

    @declared_attr
    def __mapper_args__(cls):
        return {'polymorphic_identity': cls.__name__, 'concrete': True}


class IntTreeNode(TreeNode):
    __tablename__ = "int_tree_node"
    value = Column(Integer)


class FloatTreeNode(TreeNode):
    __tablename__ = "float_tree_node"
    value = Column(Float)
    miau = Column(String(50), default='zuff')


e = create_engine("sqlite://", echo=True)
Base.metadata.create_all(e)

session = Session(e)

if PREPARE:
    configure_mappers()
    # Aha! If you don't do this, then queries involving an AbstractConcreteBase
    # fail: e.g.
    #
    # sqlalchemy.exc.InvalidRequestError: SQL expression, column, or mapped
    # entity expected - got '<class '__main__.TreeNode'>'
    #
    # ... unless you've done something else to trigger the mapper
    # configuration, which includes creating instances.

if CREATE:
    itn = IntTreeNode(name='int_node')
    if CREATE_BOTH:
        ftn = FloatTreeNode(name="float_node")

    if ADD:
        session.add(itn)
        if CREATE_BOTH:
            session.add(ftn)
        session.commit()


q = session.query(TreeNode).all()
print("Query result: {!r}".format(q))

session.close()
