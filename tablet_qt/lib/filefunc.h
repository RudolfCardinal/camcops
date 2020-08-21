/*
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
    along with CamCOPS. If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once
class QString;

namespace filefunc {

// Does the file exist?
bool fileExists(const QString& filename);

// Return the contents of a textfile.
QString textfileContents(const QString& filename);

// Deletes a file.
bool deleteFile(const QString& filename);

// Renames a file.
bool renameFile(const QString& from, const QString& to);

// Checks that a directory exists; if not, try make it; returns success.
bool ensureDirectoryExists(const QString& dir);

// Calls ensureDirectoryExists(); stop the app upon failure.
void ensureDirectoryExistsOrDie(const QString& dir);

}  // namespace filefunc
