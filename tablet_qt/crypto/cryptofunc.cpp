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

// ============================================================================
// Notes
// ============================================================================
// RNG
// - OpenSSL will seed itself automatically.
//   https://wiki.openssl.org/index.php/Random_Numbers#Seeds
//
// Simple reversible encryption
// - Under Titanium, we used SJCL: http://bitwiseshiftleft.github.io/sjcl/
// - Here, we'll use OpenSSL.
//   - https://wiki.openssl.org/index.php/EVP_Symmetric_Encryption_and_Decryption
//
// You need to store the IV (initialization vector) for AES
// - http://crypto.stackexchange.com/questions/7935/does-the-iv-need-to-be-known-by-aes-cbc-mode
// - http://crypto.stackexchange.com/questions/3965/what-is-the-main-difference-between-a-key-an-iv-and-a-nonce
// - https://en.wikipedia.org/wiki/Block_cipher_mode_of_operation#Initialization_vector_.28IV.29


// #define DANGER_DEBUG_CRYPTO  // never use this in a production environment
// #define DEBUG_LOAD_CRYPTO_DLL

#include "cryptofunc.h"
#include <cmath>  // for ceil
#include <memory>  // for std::unique_ptr
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/opensslv.h>  // version numbers
#include <QDebug>
#include <QByteArray>
#include "lib/convert.h"
#include "lib/uifunc.h"

// ----------------------------------------------------------------------------
// Determine OpenSSL version
// ----------------------------------------------------------------------------

// OPENSSL_VERSION_NUMBER format: MNNFFPPS: major minor fix patch status
#define OPENSSL_VERSION_MAJOR  ((OPENSSL_VERSION_NUMBER & 0xf0000000L) >> 28)
#define OPENSSL_VERSION_MINOR  ((OPENSSL_VERSION_NUMBER & 0x0ff00000L) >> 20)
#define OPENSSL_VERSION_FIX    ((OPENSSL_VERSION_NUMBER & 0x000ff000L) >> 12)
#define OPENSSL_VERSION_PATCH  ((OPENSSL_VERSION_NUMBER & 0x00000ff0L) >>  4)
#define OPENSSL_VERSION_STATUS ((OPENSSL_VERSION_NUMBER & 0x0000000fL) >>  0)

#if OPENSSL_VERSION_MAJOR == 0
    #error OpenSSL 0.x.x is unsupported
#elif OPENSSL_VERSION_MAJOR == 1
    // OpenSSL 1.x.x
    #if OPENSSL_VERSION_MINOR == 0
        // OpenSSL 1.0.x; e.g. tested with OpenSSL 1.0.2h
        #define OPENSSL_V_1_0
    #elif OPENSSL_VERSION_MINOR == 1
        // OpenSSL 1.1.x
        #define OPENSSL_V_1_1
        /*
            In OpenSSL 1.1,
            - EVP_MD_CTX_create (1.0.x) is renamed EVP_MD_CTX_new (1.1.x)
            - EVP_MD_CTX_destroy (1.0.x) is renamed EVP_MD_CTX_free (1.1.x)
            See "HISTORY" in
            https://www.openssl.org/docs/man1.1.0/crypto/EVP_DigestInit.html
        */
    #else
        // OpenSSL 1.x that's higher than 1.1; we will assume it behaves as 1.1
    #endif
#else
    // OpenSSL 2.x.x or higher
    #error Only know how to support OpenSSL 1.x.x
#endif

#ifdef OPENSSL_VIA_QLIBRARY

    #include <QLibrary>
    #define OPENSSL_QLIBRARY_NAME "crypto"
    QLibrary lib_crypto(OPENSSL_QLIBRARY_NAME);

    // ------------------------------------------------------------------------
    // Typedefs for using a DLL approach
    // ------------------------------------------------------------------------

    // From the headers, replace
    //      returntype functionname(parameters);
    // with a typedef for a pointer to that function:
    //      typedef returntype (*prototype_functionname)(parameters);

    typedef const EVP_CIPHER* (*prototype_EVP_aes_256_cbc)(void);

    typedef EVP_CIPHER_CTX* (*prototype_EVP_CIPHER_CTX_new)(void);
    typedef void (*prototype_EVP_CIPHER_CTX_free)(EVP_CIPHER_CTX* c);

    typedef int (*prototype_EVP_DecryptInit_ex)(
            EVP_CIPHER_CTX* ctx, const EVP_CIPHER* cipher, ENGINE* impl,
            const unsigned char* key, const unsigned char* iv);
    typedef int (*prototype_EVP_DecryptUpdate)(
            EVP_CIPHER_CTX* ctx, unsigned char* out,
            int* outl, const unsigned char* in, int inl);
    typedef int (*prototype_EVP_DecryptFinal_ex)(
            EVP_CIPHER_CTX* ctx, unsigned char* outm, int* outl);
    typedef __owur int (*prototype_EVP_DigestInit_ex)(
            EVP_MD_CTX* ctx, const EVP_MD* type, ENGINE* impl);
    typedef __owur int (*prototype_EVP_DigestUpdate)(
            EVP_MD_CTX* ctx, const void* d, size_t cnt);
    typedef __owur int (*prototype_EVP_DigestFinal_ex)(
            EVP_MD_CTX* ctx, unsigned char* md, unsigned int* s);

    typedef int (*prototype_EVP_EncryptInit_ex)(
            EVP_CIPHER_CTX* ctx, const EVP_CIPHER* type,
            ENGINE* impl, const unsigned char* key, const unsigned char* iv);
    typedef int (*prototype_EVP_EncryptUpdate)(
            EVP_CIPHER_CTX* ctx, unsigned char* out,
            int* outl, const unsigned char* in, int inl);
    typedef int (*prototype_EVP_EncryptFinal_ex)(
            EVP_CIPHER_CTX* ctx, unsigned char* out, int* outl);

    #ifdef OPENSSL_V_1_0
        typedef EVP_MD_CTX* (*prototype_EVP_MD_CTX_create)(void);
        typedef void (*prototype_EVP_MD_CTX_destroy)(EVP_MD_CTX* ctx);
    #else
        typedef EVP_MD_CTX* (*prototype_EVP_MD_CTX_new)(void);
        typedef void (*prototype_EVP_MD_CTX_free)(EVP_MD_CTX* ctx);
    #endif

    typedef const EVP_MD* (*prototype_EVP_sha512)(void);

    // ------------------------------------------------------------------------
    // #defines and helper functions for using a DLL approach
    // ------------------------------------------------------------------------

    // Define DEFINE_CRYPTO_FN such that it loads the function X from the DLL/.so
    // file, and calls it crypto_X.
    #define DEFINE_CRYPTO_FN(funcname) \
        prototype_##funcname crypto_##funcname = (prototype_##funcname) lib_crypto.resolve(#funcname);

    #define ENSURE_CRYPTO_FN_LOADED(funcname) \
        ensureCryptoFunctionLoaded((void*) crypto_##funcname, #funcname);

    void ensureCryptoFunctionLoaded(void* funcptr, const char* funcname)
    {
        if (!funcptr) {
            const QString msg = QString(
                        "Could not load %1 from crypto library %2")
                    .arg(funcname, lib_crypto.fileName());
            qCritical() << msg;
            exit(EXIT_FAILURE);
        }
    #ifdef DEBUG_LOAD_CRYPTO_DLL
        else {
            const QString msg = QString(
                        "Loaded %1 from crypto library %2")
                    .arg(funcname, lib_crypto.fileName());
            qDebug().noquote() << msg;
        }
    #endif
    }

    // ------------------------------------------------------------------------
    // For every crypto function X, either load it from the DLL and call it
    // crypto_X, or map crypto_X to the static version.
    // ------------------------------------------------------------------------

    DEFINE_CRYPTO_FN(EVP_aes_256_cbc)

    DEFINE_CRYPTO_FN(EVP_CIPHER_CTX_new)
    DEFINE_CRYPTO_FN(EVP_CIPHER_CTX_free)

    DEFINE_CRYPTO_FN(EVP_DecryptInit_ex)
    DEFINE_CRYPTO_FN(EVP_DecryptUpdate)
    DEFINE_CRYPTO_FN(EVP_DecryptFinal_ex)
    DEFINE_CRYPTO_FN(EVP_DigestInit_ex)
    DEFINE_CRYPTO_FN(EVP_DigestUpdate)
    DEFINE_CRYPTO_FN(EVP_DigestFinal_ex)

    DEFINE_CRYPTO_FN(EVP_EncryptInit_ex)
    DEFINE_CRYPTO_FN(EVP_EncryptUpdate)
    DEFINE_CRYPTO_FN(EVP_EncryptFinal_ex)

    #ifdef OPENSSL_V_1_0
        DEFINE_CRYPTO_FN(EVP_MD_CTX_create)
        DEFINE_CRYPTO_FN(EVP_MD_CTX_destroy)
    #else
        DEFINE_CRYPTO_FN(EVP_MD_CTX_new)
        DEFINE_CRYPTO_FN(EVP_MD_CTX_free)
    #endif

    DEFINE_CRYPTO_FN(EVP_sha512)


    void cryptofunc::ensureAllCryptoFunctionsLoaded()
    {
        ENSURE_CRYPTO_FN_LOADED(EVP_aes_256_cbc)

        ENSURE_CRYPTO_FN_LOADED(EVP_CIPHER_CTX_new)
        ENSURE_CRYPTO_FN_LOADED(EVP_CIPHER_CTX_free)

        ENSURE_CRYPTO_FN_LOADED(EVP_DecryptInit_ex)
        ENSURE_CRYPTO_FN_LOADED(EVP_DecryptUpdate)
        ENSURE_CRYPTO_FN_LOADED(EVP_DecryptFinal_ex)
        ENSURE_CRYPTO_FN_LOADED(EVP_DigestInit_ex)
        ENSURE_CRYPTO_FN_LOADED(EVP_DigestUpdate)
        ENSURE_CRYPTO_FN_LOADED(EVP_DigestFinal_ex)

        ENSURE_CRYPTO_FN_LOADED(EVP_EncryptInit_ex)
        ENSURE_CRYPTO_FN_LOADED(EVP_EncryptUpdate)
        ENSURE_CRYPTO_FN_LOADED(EVP_EncryptFinal_ex)

    #ifdef OPENSSL_V_1_0
        ENSURE_CRYPTO_FN_LOADED(EVP_MD_CTX_create)
        ENSURE_CRYPTO_FN_LOADED(EVP_MD_CTX_destroy)
    #else
        ENSURE_CRYPTO_FN_LOADED(EVP_MD_CTX_new)
        ENSURE_CRYPTO_FN_LOADED(EVP_MD_CTX_free)
    #endif

        ENSURE_CRYPTO_FN_LOADED(EVP_sha512)
    }

    // ------------------------------------------------------------------------
    // typedefs for our code
    // ------------------------------------------------------------------------
    using EVP_CIPHER_CTX_ptr = std::unique_ptr<EVP_CIPHER_CTX, decltype(crypto_EVP_CIPHER_CTX_free)>;
    using EVP_MD_CTX_ptr = std::unique_ptr<EVP_MD_CTX, decltype(crypto_EVP_MD_CTX_free)>;

#else

    // ------------------------------------------------------------------------
    // Non-DLL code. Map crypto_* to the OpenSSL functions directly.
    // ------------------------------------------------------------------------

    #define crypto_EVP_aes_256_cbc ::EVP_aes_256_cbc

    #define crypto_EVP_CIPHER_CTX_new ::EVP_CIPHER_CTX_new
    #define crypto_EVP_CIPHER_CTX_free ::EVP_CIPHER_CTX_free

    #define crypto_EVP_DecryptInit_ex ::EVP_DecryptInit_ex
    #define crypto_EVP_DecryptUpdate ::EVP_DecryptUpdate
    #define crypto_EVP_DecryptFinal_ex ::EVP_DecryptFinal_ex
    #define crypto_EVP_DigestInit_ex ::EVP_DigestInit_ex
    #define crypto_EVP_DigestUpdate ::EVP_DigestUpdate
    #define crypto_EVP_DigestFinal_ex ::EVP_DigestFinal_ex

    #define crypto_EVP_EncryptInit_ex ::EVP_EncryptInit_ex
    #define crypto_EVP_EncryptUpdate ::EVP_EncryptUpdate
    #define crypto_EVP_EncryptFinal_ex ::EVP_EncryptFinal_ex

    #ifdef OPENSSL_V_1_0
        #define crypto_EVP_MD_CTX_new ::EVP_MD_CTX_create  // new (1.1.x) alias -> old (1.0.x) name
        #define crypto_EVP_MD_CTX_free ::EVP_MD_CTX_destroy  // new (1.1.x) alias -> old (1.0.x) name
        // #define EVP_MD_CTX_new EVP_MD_CTX_create  // new (1.1.x) alias -> old (1.0.x) name
        // #define EVP_MD_CTX_free EVP_MD_CTX_destroy  // new (1.1.x) alias -> old (1.0.x) name
    #else
        #define crypto_EVP_MD_CTX_new ::EVP_MD_CTX_new
        #define crypto_EVP_MD_CTX_free ::EVP_MD_CTX_free
    #endif
    //#ifdef OPENSSL_V_1_0
    //#endif

    #define crypto_EVP_sha512 ::EVP_sha512

    // ------------------------------------------------------------------------
    // typedefs for our code
    // ------------------------------------------------------------------------
    using EVP_CIPHER_CTX_ptr = std::unique_ptr<EVP_CIPHER_CTX, decltype(&::EVP_CIPHER_CTX_free)>;
    using EVP_MD_CTX_ptr = std::unique_ptr<EVP_MD_CTX, decltype(&::EVP_MD_CTX_free)>;

#endif


// ============================================================================
// Constants
// ============================================================================

// const int BCRYPT_LOG_ROUNDS = 6;
// because tablets are pretty slow; see
// http://security.stackexchange.com/questions/3959/

const int AES_256BIT_KEY_SIZE = 256 / 8;
const int AES_BLOCK_SIZE_BYTES = 16;  // AES is 128 bits = 16 bytes
const int SALT_LENGTH_BYTES = 64;
// ... https://www.owasp.org/index.php/Password_Storage_Cheat_Sheet
const int SALT_LENGTH_TEXT = cryptofunc::base64Length(SALT_LENGTH_BYTES);


// ============================================================================
// Simple calculations
// ============================================================================

int cryptofunc::base64Length(const int nbytes)
{
    // http://stackoverflow.com/questions/13378815/base64-length-calculation
    double d = nbytes;
    auto len = static_cast<int>(ceil(4 * (d / 3)));
    // Round up to a multiple of 4:
    while (len % 4 != 0) {
        ++len;
    }
    return len;
}

// ============================================================================
// OpenSSL low-level calls
// ============================================================================

void cryptofunc::aesEncrypt(const QByteArray& key_bytes,
                            const QByteArray& iv_bytes,
                            const QByteArray& plaintext_bytes,
                            QByteArray& ciphertext_bytes)
{
#ifdef DANGER_DEBUG_CRYPTO
    qDebug() << Q_FUNC_INFO;
    qDebug() << "key_bytes" << key_bytes;
    qDebug() << "iv_bytes" << iv_bytes;
    qDebug() << "plaintext_bytes" << plaintext_bytes;
#endif
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto keydata = reinterpret_cast<const unsigned char*>(key_bytes.constData());
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto ivdata = reinterpret_cast<const unsigned char*>(iv_bytes.constData());
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto ptextdata = reinterpret_cast<const unsigned char*>(plaintext_bytes.constData());

    EVP_CIPHER_CTX_ptr ctx(crypto_EVP_CIPHER_CTX_new(),
                           crypto_EVP_CIPHER_CTX_free);
    int retcode = crypto_EVP_EncryptInit_ex(
                ctx.get(),  // EVP_CIPHER_CTX* ctx
                crypto_EVP_aes_256_cbc(), // const EVP_CIPHER* type
                nullptr,  // ENGINE* impl: implementation
                keydata, // unsigned char* key
                ivdata);  // unsigned char* iv
    if (retcode != 1) {
        throw std::runtime_error("EVP_EncryptInit_ex failed");
    }

    // Recovered text expands up to BLOCK_SIZE
    ciphertext_bytes.resize(plaintext_bytes.size() + AES_BLOCK_SIZE_BYTES);
    // Set the pointer (ctextdata) AFTER the resize; it's likely that the
    // resize invalidates any previous pointers to the data.
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto ctextdata = reinterpret_cast<unsigned char*>(ciphertext_bytes.data());
    int out_len1 = ciphertext_bytes.size();

    retcode = crypto_EVP_EncryptUpdate(ctx.get(), ctextdata, &out_len1,
                                       ptextdata, plaintext_bytes.size());
    if (retcode != 1) {
        throw std::runtime_error("EVP_EncryptUpdate failed");
    }

    int out_len2 = ciphertext_bytes.size() - out_len1;
    retcode = crypto_EVP_EncryptFinal_ex(
                ctx.get(), ctextdata + out_len1, &out_len2);
    if (retcode != 1) {
        throw std::runtime_error("EVP_EncryptFinal_ex failed");
    }

    // Set cipher text size now that we know it
    ciphertext_bytes.resize(out_len1 + out_len2);
#ifdef DANGER_DEBUG_CRYPTO
    qDebug() << "-> ciphertext_bytes" << ciphertext_bytes;
#endif
}


void cryptofunc::aesDecrypt(const QByteArray& key_bytes,
                            const QByteArray& iv_bytes,
                            const QByteArray& ciphertext_bytes,
                            QByteArray& recoveredtext_bytes)
{
#ifdef DANGER_DEBUG_CRYPTO
    qDebug() << Q_FUNC_INFO;
    qDebug() << "key_bytes" << key_bytes;
    qDebug() << "iv_bytes" << iv_bytes;
    qDebug() << "ciphertext_bytes" << ciphertext_bytes;
#endif
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto keydata = reinterpret_cast<const unsigned char*>(key_bytes.constData());
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto ivdata = reinterpret_cast<const unsigned char*>(iv_bytes.constData());
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto ctextdata = reinterpret_cast<const unsigned char*>(ciphertext_bytes.constData());

    EVP_CIPHER_CTX_ptr ctx(crypto_EVP_CIPHER_CTX_new(),
                           crypto_EVP_CIPHER_CTX_free);
    int retcode = crypto_EVP_DecryptInit_ex(ctx.get(), EVP_aes_256_cbc(), nullptr,
                                            keydata, ivdata);
    if (retcode != 1) {
        throw std::runtime_error("EVP_DecryptInit_ex failed");
    }

    // Recovered text contracts up to AES_BLOCK_SIZE_BYTES
    recoveredtext_bytes.resize(ciphertext_bytes.size());
    // Set the pointer (ptextdata) AFTER the resize; it's likely that the
    // resize invalidates any previous pointers to the data.
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto ptextdata = reinterpret_cast<unsigned char*>(recoveredtext_bytes.data());
    int out_len1 = recoveredtext_bytes.size();

    retcode = crypto_EVP_DecryptUpdate(ctx.get(), ptextdata, &out_len1,
                                       ctextdata, ciphertext_bytes.size());
    if (retcode != 1) {
        // throw std::runtime_error("EVP_DecryptUpdate failed");
        qWarning() << "DECRYPTION FAILED (EVP_DecryptUpdate failed)";
        recoveredtext_bytes = QByteArray();
        return;
    }

    int out_len2 = recoveredtext_bytes.size() - out_len1;
    retcode = crypto_EVP_DecryptFinal_ex(
                ctx.get(), ptextdata + out_len1, &out_len2);
    if (retcode != 1) {
        // throw std::runtime_error("EVP_DecryptFinal_ex failed");
        qWarning() << "DECRYPTION FAILED (EVP_DecryptFinal_ex failed)";
        recoveredtext_bytes = QByteArray();
        return;
    }

    // Set recovered text size now that we know it
    recoveredtext_bytes.resize(out_len1 + out_len2);
#ifdef DANGER_DEBUG_CRYPTO
    qDebug() << "-> recoveredtext_bytes" << recoveredtext_bytes;
#endif
}


SecureQByteArray cryptofunc::hashBytes(const QByteArray& plaintext_bytes)
{
    EVP_MD_CTX_ptr context(crypto_EVP_MD_CTX_new(),
                           crypto_EVP_MD_CTX_free);
    int retcode = crypto_EVP_DigestInit_ex(context.get(), EVP_sha512(), nullptr);
    if (retcode != 1) {
        throw std::runtime_error("EVP_DigestInit_ex failed");
    }
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto msgdata = reinterpret_cast<const unsigned char*>(plaintext_bytes.constData());
    int msglen = plaintext_bytes.size();
    retcode = crypto_EVP_DigestUpdate(context.get(), msgdata,
                                      static_cast<size_t>(msglen));
    if (retcode != 1) {
        qWarning() << "HASHING FAILED (EVP_DigestUpdate failed)";
        return QByteArray();
    }
    QByteArray result(EVP_MAX_MD_SIZE, 0);
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto resultdata = reinterpret_cast<unsigned char*>(result.data());
    unsigned int digestlen;
    retcode = crypto_EVP_DigestFinal_ex(context.get(), resultdata, &digestlen);
    if (retcode != 1) {
        qWarning() << "HASHING FAILED (EVP_DigestUpdate failed)";
        return QByteArray();
    }
    result.resize(static_cast<int>(digestlen));
    return result;
}


SecureQByteArray cryptofunc::makeAesIV()
{
    SecureQByteArray iv(AES_BLOCK_SIZE_BYTES, 0);
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto ivdata = reinterpret_cast<unsigned char*>(iv.data());
    RAND_bytes(ivdata, AES_BLOCK_SIZE_BYTES);
    return iv;
}


QString cryptofunc::generateIVBase64()
{
    SecureQByteArray iv = makeAesIV();
    return QString(iv.toBase64());
}


bool cryptofunc::isValidAesKey(const QByteArray& key_bytes)
{
    // https://en.wikipedia.org/wiki/Advanced_Encryption_Standard
    int n_bytes = key_bytes.size();
    int n_bits = n_bytes * 8;
#ifdef DANGER_DEBUG_CRYPTO
    qDebug() << Q_FUNC_INFO << "key_bytes" << key_bytes
             << "n_bytes" << n_bytes << "n_bits" << n_bits;
#endif
    if (n_bits == 128 || n_bits == 192 || n_bits == 256) {
        return true;
    }
    qWarning() << "... Invalid AES key size (must be 128, 192, or 256 bits); "
                  "was" << n_bytes << "bytes =" << n_bits << "bits";
    return false;
}


bool cryptofunc::isValidAesKey(const QString& key_b64)
{
    SecureQByteArray key_bytes = convert::base64ToSecureBytes(key_b64);
#ifdef DANGER_DEBUG_CRYPTO
    qDebug() << Q_FUNC_INFO
             << "key_b64" << key_b64
             << "key_bytes" << key_bytes;
#endif
    return isValidAesKey(key_bytes);
}


bool cryptofunc::isValidAesIV(const QByteArray& iv_bytes)
{
    int n_bytes = iv_bytes.size();
    int n_bits = n_bytes * 8;
#ifdef DANGER_DEBUG_CRYPTO
    qDebug() << Q_FUNC_INFO << "iv_bytes" << iv_bytes
             << "n_bytes" << n_bytes << "n_bits" << n_bits;
#endif
    if (n_bits == 128) {
        return true;
    }
    qWarning() << "... Invalid AES IV size (must be 128 bits); "
                  "was" << n_bytes << "bytes =" << n_bits << "bits";
    return false;
}


bool cryptofunc::isValidAesIV(const QString& iv_b64)
{
    QByteArray iv_bytes = convert::base64ToBytes(iv_b64);
#ifdef DANGER_DEBUG_CRYPTO
    qDebug() << Q_FUNC_INFO
             << "iv_b64" << iv_b64
             << "iv_bytes" << iv_bytes;
#endif
    return isValidAesIV(iv_bytes);
}


// ============================================================================
// Front end
// ============================================================================

SecureQByteArray cryptofunc::randomBytes(const int n)
{
    SecureQByteArray array(n, 0);
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto ptr = reinterpret_cast<unsigned char*>(array.data());
    int retcode = RAND_bytes(ptr, n);
    if (retcode == -1) {  // failure; see rand_lib.c
        uifunc::stopApp("Call to OpenSSL RAND_bytes failed");
    }
    return array;
}


SecureQString cryptofunc::generateObscuringKeyBase64()
{
    // This doesn't need a cryptographically secure RNG, really.
    // Still, we have openSSL...
    return randomBytes(AES_256BIT_KEY_SIZE).toBase64();
}


QString cryptofunc::encryptToBase64(const QString& plaintext,
                                    const QString& key_b64,
                                    const QString& iv_b64)
{
#ifdef DANGER_DEBUG_CRYPTO
    qDebug() << Q_FUNC_INFO
             << "plaintext" << plaintext
             << "key_b64" << key_b64
             << "iv_b64" << iv_b64;
#endif
    SecureQByteArray key_bytes = convert::base64ToSecureBytes(key_b64);
    if (!isValidAesKey(key_bytes)) {
        qCritical() << Q_FUNC_INFO << "Bad AES key";
        return "";
    }
    SecureQByteArray iv_bytes = convert::base64ToSecureBytes(iv_b64);
    SecureQByteArray plaintext_bytes = plaintext.toLocal8Bit();  // no other conversion
    SecureQByteArray ciphertext_bytes;
    aesEncrypt(key_bytes, iv_bytes, plaintext_bytes, ciphertext_bytes);
    QString ciphertext_b64(ciphertext_bytes.toBase64());
#ifdef DANGER_DEBUG_CRYPTO
    qDebug() << "... encrypted to" << ciphertext_b64;
#endif
    return ciphertext_b64;
}


SecureQString cryptofunc::decryptFromBase64(const QString& ciphertext_b64,
                                            const QString& key_b64,
                                            const QString& iv_b64)
{
#ifdef DANGER_DEBUG_CRYPTO
    qDebug() << Q_FUNC_INFO
             << "ciphertext_b64" << ciphertext_b64
             << "key_b64" << key_b64;
#endif
    SecureQByteArray key_bytes = convert::base64ToSecureBytes(key_b64);
    if (!isValidAesKey(key_bytes)) {
        qCritical() << Q_FUNC_INFO << "Bad AES key";
        return "";
    }
    SecureQByteArray ciphertext_bytes = convert::base64ToSecureBytes(ciphertext_b64);
    SecureQByteArray iv_bytes = convert::base64ToSecureBytes(iv_b64);
    SecureQByteArray plaintext_bytes;
    aesDecrypt(key_bytes, iv_bytes, ciphertext_bytes, plaintext_bytes);
    SecureQString plaintext(plaintext_bytes);  // ASSUMES IT IS TEXT.
    // If there are zero bytes in plaintext_bytes, this will go wrong.
#ifdef DANGER_DEBUG_CRYPTO
    qDebug() << "... decrypted to" << plaintext;
#endif
    return plaintext;
}


QString cryptofunc::hash(const QString& plaintext, const QString& salt)
{
    if (salt.length() != SALT_LENGTH_TEXT) {
        qWarning() << "Salt length is" << salt.length()
                   << "but should be" << SALT_LENGTH_TEXT;
    }
    SecureQString to_hash_text(salt + plaintext);
    SecureQByteArray to_hash_bytes = to_hash_text.toLocal8Bit();
    QByteArray hashed_bytes = hashBytes(to_hash_bytes);
    QString hashed_text = hashed_bytes.toBase64();
    QString result = salt + hashed_text;
#ifdef DANGER_DEBUG_CRYPTO
    qDebug() << Q_FUNC_INFO
             << "salt" << salt
             << "+ plaintext" << plaintext
             << "-> " << result;
#endif
    return result;
}


QString cryptofunc::hash(const QString& plaintext)
{
    QString salt = makeSalt();
    return hash(plaintext, salt);
}


bool cryptofunc::matchesHash(const QString& plaintext, const QString& hashed)
{
    if (hashed.length() < SALT_LENGTH_TEXT) {
        return false;
    }
    QString salt = hashed.left(SALT_LENGTH_TEXT);  // recover salt
    return hashed == hash(plaintext, salt);
}


QString cryptofunc::makeSalt()
{
    return randomBytes(SALT_LENGTH_BYTES).toBase64();
}
