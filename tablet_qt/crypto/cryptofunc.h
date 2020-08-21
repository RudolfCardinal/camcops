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

// #define OPENSSL_VIA_QLIBRARY

// #include <openssl/crypto.h>  // for OpenSSL_cleanse
#include <QString>
#include "secureqbytearray.h"
#include "secureqstring.h"


namespace cryptofunc {

// ============================================================================
// DLL helpers
// ============================================================================

#ifdef OPENSSL_VIA_QLIBRARY
// Ensure all relevant OpenSSL functions are loaded.
void ensureAllCryptoFunctionsLoaded();
#endif

// ============================================================================
// Simple calculations
// ============================================================================

// Length of the base64 representation of this many bytes.
int base64Length(int nbytes);

// ============================================================================
// Low-level calls
// ============================================================================

// AES encryption.
void aesEncrypt(const QByteArray& key_bytes, const QByteArray& iv_bytes,
                const QByteArray& plaintext_bytes,
                QByteArray& ciphertext_bytes);

// AES decryption.
void aesDecrypt(const QByteArray& key_bytes, const QByteArray& iv_bytes,
                const QByteArray& ciphertext_bytes,
                QByteArray& recoveredtext_bytes);

// Makes a new random AES initialization vector.
SecureQByteArray makeAesIV();

// Is this a valid AES key?
bool isValidAesKey(const QByteArray& key_bytes);

// Is this a valid AES key?
bool isValidAesKey(const QString& key_b64);

// Is this a valid AES initialization vector?
bool isValidAesIV(const QByteArray& iv_bytes);

// Is this a valid AES initialization vector?
bool isValidAesIV(const QString& iv_b64);

// Hash bytes via SHA512.
SecureQByteArray hashBytes(const QByteArray& plaintext_bytes);

// Salt plaintxt and hash it via SHA512.
QString hash(const QString& plaintext, const QString& salt);

// Create a random salt.
QString makeSalt();

// ============================================================================
// Front end
// ============================================================================

// Generate random bytes.
SecureQByteArray randomBytes(int n);

// Generate a base64 representation of some random bytes, for use as a
// password-obscuring key.
SecureQString generateObscuringKeyBase64();

// Generate an AES initialization vector in base64 format.
QString generateIVBase64();

// Encrypts plaintext via AES, returning the result in base64 format.
QString encryptToBase64(const QString& plaintext,
                        const QString& key_b64,
                        const QString& iv_b64);

// Decrypts base64-encoded AES-encrypted data.
SecureQString decryptFromBase64(const QString& ciphertext_b64,
                                const QString& key_b64,
                                const QString& iv_b64);

// Hashes a password.
QString hash(const QString& plaintext);

// Checks if a plaintext password matches a hashed version.
bool matchesHash(const QString& plaintext, const QString& hashed);

}  // namespace cryptofunc
