/**
 * SPDX-FileCopyrightText: 2023 Albert Vaca <albertvaka@gmail.com>
 * SPDX-FileCopyrightText: 2023 Edward Kigwana <ekigwana@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "sslhelper.h"
#include "core_debug.h"

extern "C" {
#include <openssl/bn.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/rsa.h>
#include <openssl/x509.h>
}

namespace SslHelper
{
QString getSslError()
{
    char buf[256];
    ERR_error_string_n(ERR_get_error(), buf, sizeof(buf));
    return QString::fromLatin1(buf);
}

QSslKey generateEcPrivateKey()
{
    // Initialize context.
    auto pctxRaw = EVP_PKEY_CTX_new_id(EVP_PKEY_EC, nullptr);
    auto pctx = std::unique_ptr<EVP_PKEY_CTX, decltype(&::EVP_PKEY_CTX_free)>(pctxRaw, ::EVP_PKEY_CTX_free);
    if (!pctx) {
        qCWarning(KDECONNECT_CORE) << "Generate EC Private Key failed to allocate context " << getSslError();
        return QSslKey();
    }

    if (EVP_PKEY_keygen_init(pctx.get()) <= 0) {
        qCWarning(KDECONNECT_CORE) << "Generate EC Private Key failed to initialize context " << getSslError();
        return QSslKey();
    }

    // Set the curve.
    if (EVP_PKEY_CTX_set_ec_paramgen_curve_nid(pctx.get(), NID_X9_62_prime256v1) <= 0) {
        qCWarning(KDECONNECT_CORE) << "Generate EC Private Key failed to set curve " << getSslError();
        return QSslKey();
    }

    // Generate private key.
    auto pkey = std::unique_ptr<EVP_PKEY, decltype(&::EVP_PKEY_free)>(EVP_PKEY_new(), ::EVP_PKEY_free);
    if (!pkey) {
        qCWarning(KDECONNECT_CORE) << "Generate EC Private Key failed to allocate private key " << getSslError();
        return QSslKey();
    }

    auto pkey_raw = pkey.get();
    if (EVP_PKEY_keygen(pctx.get(), &pkey_raw) <= 0) {
        qCWarning(KDECONNECT_CORE) << "Generate EC Private Key failed to generate private key " << getSslError();
        return QSslKey();
    }

    // Convert private key format to PEM as required by QSslKey.
    auto bio = std::unique_ptr<BIO, decltype(&::BIO_free_all)>(BIO_new(BIO_s_mem()), ::BIO_free_all);
    if (!bio) {
        qCWarning(KDECONNECT_CORE) << "Generate EC Private Key failed to allocate I/O abstraction " << getSslError();
        return QSslKey();
    }

    if (!PEM_write_bio_PrivateKey(bio.get(), pkey_raw, nullptr, nullptr, 0, nullptr, nullptr)) {
        qCWarning(KDECONNECT_CORE) << "Generate EC Private Key failed write PEM format private key to BIO " << getSslError();
        return QSslKey();
    }

    BUF_MEM *mem = nullptr;
    if (!BIO_get_mem_ptr(bio.get(), &mem)) {
        qCWarning(KDECONNECT_CORE) << "Generate EC Private Key failed get PEM format address " << getSslError();
        return QSslKey();
    }

    return QSslKey(QByteArray(mem->data, mem->length), QSsl::KeyAlgorithm::Ec);
}

QSslKey generateRsaPrivateKey()
{
    // Initialize context.
    auto pctxRaw = EVP_PKEY_CTX_new_id(EVP_PKEY_RSA, nullptr);
    auto pctx = std::unique_ptr<EVP_PKEY_CTX, decltype(&::EVP_PKEY_CTX_free)>(pctxRaw, ::EVP_PKEY_CTX_free);
    if (!pctx) {
        qCWarning(KDECONNECT_CORE) << "Generate RSA Private Key failed to allocate context " << getSslError();
        return QSslKey();
    }

    if (EVP_PKEY_keygen_init(pctx.get()) <= 0) {
        qCWarning(KDECONNECT_CORE) << "Generate RSA Private Key failed to initialize context " << getSslError();
        return QSslKey();
    }

    // Set key bits.
    if (EVP_PKEY_CTX_set_rsa_keygen_bits(pctx.get(), 2048) <= 0) {
        qCWarning(KDECONNECT_CORE) << "Generate RSA Private Key failed to set key bits " << getSslError();
        return QSslKey();
    }

    // Generate private key.
    auto pkey = std::unique_ptr<EVP_PKEY, decltype(&::EVP_PKEY_free)>(EVP_PKEY_new(), ::EVP_PKEY_free);
    if (!pkey) {
        qCWarning(KDECONNECT_CORE) << "Generate RSA Private Key failed to allocate private key " << getSslError();
        return QSslKey();
    }

    auto pkey_raw = pkey.get();
    if (EVP_PKEY_keygen(pctx.get(), &pkey_raw) <= 0) {
        qCWarning(KDECONNECT_CORE) << "Generate RSA Private Key failed to generate private key " << getSslError();
        return QSslKey();
    }

    // Convert private key format to PEM as required by QSslKey.
    auto bio = std::unique_ptr<BIO, decltype(&::BIO_free_all)>(BIO_new(BIO_s_mem()), ::BIO_free_all);
    if (!bio) {
        qCWarning(KDECONNECT_CORE) << "Generate RSA Private Key failed to allocate I/O abstraction " << getSslError();
        return QSslKey();
    }

    if (!PEM_write_bio_PrivateKey(bio.get(), pkey_raw, nullptr, nullptr, 0, nullptr, nullptr)) {
        qCWarning(KDECONNECT_CORE) << "Generate RSA Private Key failed write PEM format private key to BIO " << getSslError();
        return QSslKey();
    }

    BUF_MEM *mem = nullptr;
    if (!BIO_get_mem_ptr(bio.get(), &mem)) {
        qCWarning(KDECONNECT_CORE) << "Generate RSA Private Key failed get PEM format address " << getSslError();
        return QSslKey();
    }

    return QSslKey(QByteArray(mem->data, mem->length), QSsl::KeyAlgorithm::Rsa);
}

QSslCertificate generateSelfSignedCertificate(const QSslKey &qtPrivateKey, const QString &commonName)
{
    // Create certificate.
    auto x509 = std::unique_ptr<X509, decltype(&::X509_free)>(X509_new(), ::X509_free);
    if (!x509) {
        qCWarning(KDECONNECT_CORE) << "Generate Self Signed Certificate failed to allocate certificate " << getSslError();
        return QSslCertificate();
    }

    constexpr int x509version = 3 - 1; // version is 0-indexed, so we need the -1
    if (!X509_set_version(x509.get(), x509version)) {
        qCWarning(KDECONNECT_CORE) << "Generate Self Signed Certificate failed to set version " << getSslError();
        return QSslCertificate();
    }

    // Generate a random serial number for the certificate.
    auto sn = std::unique_ptr<BIGNUM, decltype(&::BN_free)>(BN_new(), ::BN_free);
    if (!sn) {
        qCWarning(KDECONNECT_CORE) << "Generate Self Signed Certificate failed to allocate big number structure " << getSslError();
        return QSslCertificate();
    }

    if (!BN_rand(sn.get(), 160, -1, 0)) { // as per rfc3280, serial numbers must be 20 bytes (160 bits) or less
        qCWarning(KDECONNECT_CORE) << "Generate Self Signed Certificate failed to generate random number " << getSslError();
        return QSslCertificate();
    }

    if (!BN_to_ASN1_INTEGER(sn.get(), X509_get_serialNumber(x509.get()))) {
        qCWarning(KDECONNECT_CORE) << "Generate Self Signed Certificate failed to convert number structure to integer" << getSslError();
        return QSslCertificate();
    }

    // Set the certificate subject and issuer (self-signed).
    auto name = X509_get_subject_name(x509.get());
    QByteArray commonNameBytes = commonName.toLatin1();
    const unsigned char *commonNameCStr = reinterpret_cast<const unsigned char *>(commonNameBytes.data());
    if (!X509_NAME_add_entry_by_txt(name, "CN", MBSTRING_ASC, commonNameCStr, -1, -1, 0)) { // Common Name
        qCWarning(KDECONNECT_CORE) << "Generate Self Signed Certificate failed to set common name to " << commonName << " " << getSslError();
        return QSslCertificate();
    }

    const unsigned char *organizationCStr = reinterpret_cast<const unsigned char *>("KDE");
    if (!X509_NAME_add_entry_by_txt(name, "O", MBSTRING_ASC, organizationCStr, -1, -1, 0)) { // Organization
        qCWarning(KDECONNECT_CORE) << "Generate Self Signed Certificate failed to set organization " << getSslError();
        return QSslCertificate();
    }

    const unsigned char *organizationalUnitCStr = reinterpret_cast<const unsigned char *>("KDE Connect");
    if (!X509_NAME_add_entry_by_txt(name, "OU", MBSTRING_ASC, organizationalUnitCStr, -1, -1, 0)) { // Organizational Unit
        qCWarning(KDECONNECT_CORE) << "Generate Self Signed Certificate failed to set organizational unit " << getSslError();
        return QSslCertificate();
    }

    if (!X509_set_subject_name(x509.get(), name)) {
        qCWarning(KDECONNECT_CORE) << "Generate Self Signed Certificate failed to set subject name" << getSslError();
        return QSslCertificate();
    }

    if (!X509_set_issuer_name(x509.get(), name)) {
        qCWarning(KDECONNECT_CORE) << "Generate Self Signed Certificate failed to set issuer name" << getSslError();
        return QSslCertificate();
    }

    // Set the certificate validity period.
    int a_year_in_seconds = 365 * 24 * 60 * 60;
    X509_gmtime_adj(X509_getm_notBefore(x509.get()), -a_year_in_seconds);
    X509_gmtime_adj(X509_getm_notAfter(x509.get()), 10 * a_year_in_seconds);

    // Convert the QSslKey to the OpenSSL private key format.
    QByteArray keyPemData = qtPrivateKey.toPem();
    auto bio = std::unique_ptr<BIO, decltype(&::BIO_free_all)>(BIO_new_mem_buf(keyPemData.data(), -1), ::BIO_free_all);
    if (!bio) {
        qCWarning(KDECONNECT_CORE) << "Generate Self Signed Certificate failed to allocate I/O abstraction " << getSslError();
        return QSslCertificate();
    }

    auto pkeyRaw = PEM_read_bio_PrivateKey(bio.get(), nullptr, nullptr, nullptr);
    auto pkey = std::unique_ptr<EVP_PKEY, decltype(&::EVP_PKEY_free)>(pkeyRaw, ::EVP_PKEY_free);
    if (!pkey) {
        qCWarning(KDECONNECT_CORE) << "Generate Self Signed Certificate failed to read private key " << getSslError();
        return QSslCertificate();
    }

    if (!X509_set_pubkey(x509.get(), pkey.get())) {
        qCWarning(KDECONNECT_CORE) << "Generate Self Signed Certificate failed to set private key " << getSslError();
        return QSslCertificate();
    }

    // Sign the certificate with private key.
    if (!X509_sign(x509.get(), pkey.get(), EVP_sha512())) {
        qCWarning(KDECONNECT_CORE) << "Generate Self Signed Certificate failed to sign certificate " << getSslError();
        return QSslCertificate();
    }

    // Convert to PEM which is the format needed for QSslCertificate.
    bio = std::unique_ptr<BIO, decltype(&::BIO_free_all)>(BIO_new(BIO_s_mem()), ::BIO_free_all);
    if (!bio) {
        qCWarning(KDECONNECT_CORE) << "Generate Self Signed Certificate failed to allocate I/O abstraction " << getSslError();
        return QSslCertificate();
    }

    if (!PEM_write_bio_X509(bio.get(), x509.get())) {
        qCWarning(KDECONNECT_CORE) << "Generate Self Signed Certificate failed write PEM format certificate to BIO " << getSslError();
        return QSslCertificate();
    }

    BUF_MEM *pem = nullptr;
    if (!BIO_get_mem_ptr(bio.get(), &pem)) {
        qCWarning(KDECONNECT_CORE) << "Generate Self Signed Certificate failed get PEM format address " << getSslError();
        return QSslCertificate();
    }

    return QSslCertificate(QByteArray(pem->data, pem->length));
}

} // namespace SslHelper
