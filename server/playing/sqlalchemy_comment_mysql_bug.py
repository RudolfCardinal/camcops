#!/usr/bin/env python

"""
playing/sqlalchemy_comment_mysql_bug.py

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

-- In MySQL:
DROP DATABASE dummy;
CREATE DATABASE dummy;
GRANT ALL PRIVILEGES ON dummy.* TO 'scott'@'localhost' IDENTIFIED BY 'tiger';
"""

from sqlalchemy.engine import create_engine
from sqlalchemy.ext.declarative import declarative_base
from sqlalchemy.sql.schema import Column
from sqlalchemy.sql.sqltypes import Boolean, Integer, UnicodeText

Base = declarative_base()


class Thing(Base):
    __tablename__ = "thing"
    id = Column(Integer, primary_key=True, autoincrement=True)
    weight_loss = Column(Boolean, comment="Weight loss of 5% or more")
    test_unicode_text = Column(UnicodeText)


# url = "mysql+mysqldb://scott:tiger@127.0.0.1:3306/dummy?charset=utf8"
url = "mysql+mysqldb://scott:tiger@127.0.0.1:3306/dummy"
engine = create_engine(url, echo=True)
Base.metadata.create_all(engine)
