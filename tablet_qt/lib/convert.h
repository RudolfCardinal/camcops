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
#include <QAbstractSocket>
#include <QDebug>
#include <QMap>
#include <QSsl>
#include <QString>
#include <QUrlQuery>
#include <QVariant>
#include "common/dpi.h"
#include "crypto/secureqbytearray.h"
#include "crypto/secureqstring.h"
#include "lib/version.h"
#include "maths/mathfunc.h"

class QByteArray;
class QImage;


namespace convert {

// ============================================================================
// Constants used in several places internally
// ============================================================================

extern const QChar BACKSLASH;
extern const QChar COMMA;
extern const QChar CR;  // carriage return
extern const QChar DQUOTE;  // double quote
extern const QChar NL;  // newline
extern const QChar QMARK;  // question mark
extern const QChar SPACE;
extern const QChar SQUOTE;  // single quote
extern const QChar TAB;
extern const QChar ZERO;

extern const ushort UNICODE_BACKSLASH;
extern const ushort UNICODE_COMMA;
extern const ushort UNICODE_CR;
extern const ushort UNICODE_DQUOTE;
extern const ushort UNICODE_NL;
extern const ushort UNICODE_SPACE;
extern const ushort UNICODE_TAB;


// ============================================================================
// SQL literals (and things very like them)
// ============================================================================

extern const QString NULL_STR;  // "NULL"

// Escape LF (\n) to the literal "\n"; similarly with CR (\r); escape
// backslashes to double-backslashes.
QString escapeNewlines(QString raw);

// Reverse escapeNewlines()
QString unescapeNewlines(const QString& escaped);

// Convert e.g. "Bob's house" to "'Bob''s house'", giving an SQL string literal.
QString sqlQuoteString(QString raw);

// Reverse sqlQuoteString().
QString sqlDequoteString(const QString& quoted);

// Encode bytes as base64, in the special format "64'...'"
QString blobToQuotedBase64(const QByteArray& blob);

// Reverses blobToQuotedBase64().
QByteArray quotedBase64ToBlob(const QString& quoted);

// Takes hex, e.g. "7", "FF", and if it has one character, prepend a zero,
// giving e.g. "07", "FF".
QString padHexTwo(const QString& input);

// Returns hex-encoded data in the format "X'01FF76A8'".
QString blobToQuotedHex(const QByteArray& blob);

// Reverses blobToQuotedHex().
QByteArray quotedHexToBlob(const QString& hex);

// Takes a large variety of QVariant objects and turns them into an SQL literal
// or something very similar (e.g. our special base-64 notation), suitable for
// fairly efficient network transmission.
QString toSqlLiteral(const QVariant& value);

// Reverses toSqlLiteral().
QVariant fromSqlLiteral(const QString& literal);

// Takes a CSV string, applies fromSqlLiteral() to each part, and returns the
// resulting values.
QVector<QVariant> csvSqlLiteralsToValues(const QString& csv);

// Converts a list of QVariants into CSV-encoded SQL-style literals, via
// toSqlLiteral().
QString valuesToCsvSqlLiterals(const QVector<QVariant>& values);

// ============================================================================
// C++ literals
// ============================================================================

// Turns a string into the text you would type into C++ to represent that
// string; e.g. converts LF (\n) to "\n".
QString stringToUnquotedCppLiteral(const QString& raw);

// As for stringToUnquotedCppLiteral(), but also encloses the string in
// double quotes.
QString stringToCppLiteral(const QString& raw);

// Reverses stringToUnquotedCppLiteral().
QString unquotedCppLiteralToString(const QString& escaped);

// Reverses stringToCppLiteral().
QString cppLiteralToString(const QString& escaped);

// ============================================================================
// Images
// ============================================================================

// Writes a QImage to bytes in the specified image format.
QByteArray imageToByteArray(const QImage& image,
                            const char* format = "png");

// Writes a QImage to a QVariant (of bytes) in the specified image format.
QVariant imageToVariant(const QImage& image, const char* format = "png");

// Converts a byte array to a QImage. You can specify the format or allow Qt
// to autodetect it.
QImage byteArrayToImage(const QByteArray& array,
                        bool* successful,
                        const char* format = nullptr);

// Converts a length in pixels from one DPI setting to another (maintaining the
// same real-world length).
int convertLengthByDpi(int old_length, qreal to_dpi, qreal from_dpi);

// Converts a length in pixels from our default internal DPI setting
// (uiconst::DEFAULT_DPI) to what we think is the DPI setting of the system
// we're running on (uiconst::g_logical_dpi_x or uiconst::g_logical_dpi_y).
int convertLengthByLogicalDpiX(int old_length);
int convertLengthByLogicalDpiY(int old_length);

// Converts a QSize by DPI; as for convertLengthByDpi(int, qreal, qreal).
QSize convertSizeByDpi(const QSize& old_size,
                       const Dpi& to_dpi, const Dpi& from_dpi);

// Converts a QSize by default logical DPI.
QSize convertSizeByLogicalDpi(const QSize& old_size);

// Converts a distance in cm to a number of pixels, given a DPI setting.
int convertCmToPx(qreal cm, qreal dpi);

// ============================================================================
// Cryptography
// ============================================================================

// Converts text containing a plain base-64 encoding into bytes.
QByteArray base64ToBytes(const QString& data_b64);

// Same as base64ToBytes() at present.
SecureQByteArray base64ToSecureBytes(const QString& data_b64);

// ============================================================================
// Display formatting
// ============================================================================

// Formats a number with a certain number of decimal places.
QString toDp(double x, int dp);

// Displays a QVariant in a pretty format, with an explicit type specified.
QString prettyValue(const QVariant& variant, int dp, QVariant::Type type);

// Displays a QVariant in a pretty format, asking it for its type.
QString prettyValue(const QVariant& variant, int dp = -1);

// Formats a size in bytes in a pretty way, e.g. "3 KiB" or "3 kb" etc.
QString prettySize(double num, bool space = true, bool binary = false,
                   bool longform = false, const QString& suffix = "B");

// Returns a string form of an arbitrary pointer.
QString prettyPointer(const void* pointer);

// ============================================================================
// Networking
// ============================================================================

// Transforms a dictionary into a QUrlQuery, intended for the "?k1=v1&k2=v2"
// format used in URLs.
QUrlQuery getPostDataAsUrlQuery(const QMap<QString, QString>& dict);

// Converts a server reply looking like key1:value1\nkey2:value2 ...
// into a dictionary.
QMap<QString, QString> getReplyDict(const QByteArray& data);

// Converts UTF-8-encoded bytes into a string.
QString getReplyString(const QByteArray& data);

extern const QString SSLPROTODESC_SSLV3;
extern const QString SSLPROTODESC_SSLV2;
extern const QString SSLPROTODESC_TLSV1_0;
extern const QString SSLPROTODESC_TLSV1_1;
extern const QString SSLPROTODESC_TLSV1_2;
extern const QString SSLPROTODESC_ANYPROTOCOL;
extern const QString SSLPROTODESC_TLSV1_SSLV3;
extern const QString SSLPROTODESC_SECUREPROTOCOLS;
extern const QString SSLPROTODESC_TLSV1_0_OR_LATER;
extern const QString SSLPROTODESC_TLSV1_1_OR_LATER;
extern const QString SSLPROTODESC_TLSV1_2_OR_LATER;
extern const QString SSLPROTODESC_UNKNOWN_PROTOCOL;

// Returns a description of an SSL protocol.
QString describeSslProtocol(QSsl::SslProtocol protocol);

// The reverse of describeSslProtocol().
QSsl::SslProtocol sslProtocolFromDescription(const QString& desc);

// ============================================================================
// QChar oddities
// ============================================================================

// Converts a QString-type QVariant into a QChar-type QVariant (something that
// Qt is reluctant to do).
QVariant toQCharVariant(const QVariant& v);

// ============================================================================
// Specific vectors as strings
// ============================================================================

// Converts a numeric (e.g. int) vector into a CSV string representation,
// via QString::number.
template<typename T>
QString numericVectorToCsvString(const QVector<T>& vec)
{
    QStringList strings;
    for (const T& value : vec) {
        strings.append(QString::number(value));
    }
    return strings.join(COMMA);
}


// Converts a CSV string into an int vector.
// (Duff values will be converted to 0.)
QVector<int> csvStringToIntVector(const QString& str);

// Converts a QStringList to CSV, encoding each string via stringToCppLiteral().
QString qStringListToCsvString(const QStringList& vec);

// Reverses csvStringToQStringList().
QStringList csvStringToQStringList(const QString& str);

// ============================================================================
// QVariant modifications
// ============================================================================

extern const char* TYPENAME_QVECTOR_INT;
extern const char* TYPENAME_VERSION;

// Register our custom types with QVariant, via qRegisterMetaType().
void registerTypesForQVariant();

// Register custom data types that need to be passed via Qt signals/slots, but
// which don't need to be stored in a QVariant.
void registerOtherTypesForSignalsSlots();

// "Is this QVariant one of the user-defined QVariant types?"
bool isQVariantOfUserType(const QVariant& v, const QString& type_name);

// Converts a QVariant that's of the user-registered type QVector<int> into
// that QVector<int>.
QVector<int> qVariantToIntVector(const QVariant& v);

// ============================================================================
// JSON
// ============================================================================

// Returns a JSON-encoded version of a string list (as a JSON array).
QString stringListToJson(const QStringList& list, bool compact = true);

// ============================================================================
// Physical units (other than time: in datetime namespace)
// ============================================================================

extern const double CM_PER_INCH;
extern const int CM_PER_M;
extern const int INCHES_PER_FOOT;

extern const int POUNDS_PER_STONE;
extern const int OUNCES_PER_POUND;
extern const int GRAMS_PER_KG;
extern const double GRAMS_PER_POUND;
extern const double POUNDS_PER_KG;

// Distance: imperial to metric
double metresFromFeetInches(double feet, double inches);
double centimetresFromInches(double inches);

// Distance: metric to imperial
void feetInchesFromMetres(double metres, int& feet, double& inches);
double inchesFromCentimetres(double centimeters);

// Mass: imperial to metric
double kilogramsFromStonesPoundsOunces(double stones, double pounds,
                                       double ounces = 0);

// Mass: metric to imperial
void stonesPoundsFromKilograms(
        double kilograms, int& stones, double& pounds);
void stonesPoundsOuncesFromKilograms(
        double kilograms, int& stones, int& pounds, double& ounces);

// Time unit conversion
int msFromMin(qreal minutes);  // max 32-bit signed int is +2,147,483,647 ms = 35,791.39 minutes = 24.8 days
int msFromSec(qreal seconds);  // ditto


// ============================================================================
// Tests
// ============================================================================

// Assert that two things are equal, or crash.

template<typename T>
void assert_eq(const T& a, const T& b)
{
    if (a == b) {
        qDebug() << "Conversion success:" << a << "==" << b;
    } else {
        qCritical() << "Conversion failure:" << a << "!=" << b;
        Q_ASSERT(false);
        qFatal("Stopping");
    }
}

// Specialization of assert_eq().

template<>
void assert_eq(const double& a, const double& b);

// Perform a self-test of our conversion functions.

void testConversions();


}  // namespace convert


// ============================================================================
// Using QVector in QVariant: see also convert::registerQVectorTypesForQVariant()
// ============================================================================

Q_DECLARE_METATYPE(QVector<int>)

// Other signal/slot registrations for Qt core types:

// Q_DECLARE_METATYPE(QAbstractSocket::SocketError)
//
// ... Docs at http://doc.qt.io/qt-5/qabstractsocket.html#signals say that
// "QAbstractSocket::SocketError is not a registered metatype, so for queued
// connections, you will have to register it with Q_DECLARE_METATYPE() and
// qRegisterMetaType()" -- however, using
// Q_DECLARE_METATYPE(QAbstractSocket::SocketError) causes
// "error: redefinition of ‘struct QMetaTypeId<QAbstractSocket::SocketError>’
//
// And indeed, there is "Q_DECLARE_METATYPE(QAbstractSocket::SocketError)"
// in qabstractsocket.h. See also
// https://forum.qt.io/topic/61394/qabstractsocket-error-signal-not-emitted/8
