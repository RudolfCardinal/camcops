#!/usr/bin/env python

"""
playing/sqla_concrete_inheritance_1.py

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
from sqlalchemy.orm import Session, relationship
from sqlalchemy.ext.declarative import declarative_base
from sqlalchemy.orm.collections import attribute_mapped_collection
from sqlalchemy.ext.declarative import declared_attr, AbstractConcreteBase


Base = declarative_base()


class Mammut(Base):
    __tablename__ = "mammut"

    id = Column(Integer, primary_key=True)
    nodes = relationship(
        'TreeNode',
        lazy='dynamic',
        back_populates='mammut',
    )


class TreeNode(AbstractConcreteBase, Base):
    id = Column(Integer, primary_key=True)
    name = Column(String)

    @declared_attr
    def __tablename__(cls):
        if cls.__name__ == 'TreeNode':
            return None
        else:
            return cls.__name__.lower()

    @declared_attr
    def __mapper_args__(cls):
        return {'polymorphic_identity': cls.__name__, 'concrete': True}

    @declared_attr
    def parent_id(cls):
        return Column(Integer, ForeignKey(cls.id))

    @declared_attr
    def mammut_id(cls):
        return Column(Integer, ForeignKey('mammut.id'))

    @declared_attr
    def mammut(cls):
        return relationship("Mammut", back_populates="nodes")

    @declared_attr
    def children(cls):
        return relationship(
            cls,
            back_populates="parent",
            collection_class=attribute_mapped_collection('name'),
        )

    @declared_attr
    def parent(cls):
        return relationship(
            cls, remote_side="%s.id" % cls.__name__,
            back_populates='children')


class IntTreeNode(TreeNode):
    value = Column(Integer)


class FloatTreeNode(TreeNode):
    value = Column(Float)
    miau = Column(String(50), default='zuff')

e = create_engine("sqlite://", echo=True)
Base.metadata.create_all(e)

session = Session(e)

root = IntTreeNode(name='root')
IntTreeNode(name='n1', parent=root)
n2 = IntTreeNode(name='n2', parent=root)
IntTreeNode(name='n2n1', parent=n2)

m1 = Mammut()
m1.nodes.append(n2)
m1.nodes.append(root)

session.add(root)
session.commit()


session.close()

root = session.query(TreeNode).filter_by(name='root').one()
print(root.children)
