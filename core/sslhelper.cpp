/**
 * SPDX-FileCopyrightText: 2023 Albert Vaca <albertvaka@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "sslhelper.h"

#include <openssl/bn.h>
#include <openssl/pem.h>
#include <openssl/rsa.h>
#include <openssl/x509v3.h>

namespace SslHelper
{

X509 *generateSelfSignedCertificate(EVP_PKEY *privateKey, const QString &commonName)
{
    X509 *x509 = X509_new();
    X509_set_version(x509, 2);

    // Generate a random serial number for the certificate
    BIGNUM *serialNumber = BN_new();
    BN_rand(serialNumber, 160, -1, 0);
    ASN1_INTEGER *serial = X509_get_serialNumber(x509);
    BN_to_ASN1_INTEGER(serialNumber, serial);
    BN_free(serialNumber);

    // Set the certificate subject and issuer (self-signed)
    X509_NAME *name = X509_NAME_new();
    X509_NAME_add_entry_by_txt(name, "CN", MBSTRING_ASC, reinterpret_cast<const unsigned char *>(commonName.toLatin1().data()), -1, -1, 0); // Common Name
    X509_NAME_add_entry_by_txt(name, "O", MBSTRING_ASC, reinterpret_cast<const unsigned char *>("KDE"), -1, -1, 0); // Organization
    X509_NAME_add_entry_by_txt(name, "OU", MBSTRING_ASC, reinterpret_cast<const unsigned char *>("KDE Connect"), -1, -1, 0); // Organizational Unit
    X509_set_subject_name(x509, name);
    X509_set_issuer_name(x509, name);
    X509_NAME_free(name);

    // Set the certificate validity period
    time_t now = time(nullptr);
    int a_year_in_seconds = 356 * 24 * 60 * 60;
    ASN1_TIME_set(X509_get_notBefore(x509), now - a_year_in_seconds);
    ASN1_TIME_set(X509_get_notAfter(x509), now + 10 * a_year_in_seconds);

    // Set the public key for the certificate
    X509_set_pubkey(x509, privateKey);

    // Sign the certificate with the private key
    X509_sign(x509, privateKey, EVP_sha256());

    return x509;
}

EVP_PKEY *generateRsaPrivateKey()
{
    EVP_PKEY *privateKey = EVP_PKEY_new();
    RSA *rsa = RSA_new();

    BIGNUM *exponent = BN_new();
    BN_set_word(exponent, RSA_F4);

    RSA_generate_key_ex(rsa, 2048, exponent, nullptr);
    EVP_PKEY_assign_RSA(privateKey, rsa);

    BN_free(exponent);

    return privateKey;
}

QByteArray certificateToPEM(X509 *certificate)
{
    BIO *bio = BIO_new(BIO_s_mem());
    PEM_write_bio_X509(bio, certificate);
    BUF_MEM *mem = nullptr;
    BIO_get_mem_ptr(bio, &mem);
    QByteArray pemData(mem->data, mem->length);
    BIO_free_all(bio);
    return pemData;
}

QByteArray privateKeyToPEM(EVP_PKEY *privateKey)
{
    BIO *bio = BIO_new(BIO_s_mem());
    PEM_write_bio_PrivateKey(bio, privateKey, nullptr, nullptr, 0, nullptr, nullptr);
    BUF_MEM *mem = nullptr;
    BIO_get_mem_ptr(bio, &mem);
    QByteArray pemData(mem->data, mem->length);
    BIO_free_all(bio);
    return pemData;
}

EVP_PKEY *pemToRsaPrivateKey(const QByteArray &privateKeyPem)
{
    const char *pemData = privateKeyPem.constData();
    BIO *bio = BIO_new_mem_buf(pemData, -1);
    EVP_PKEY *privateKey = PEM_read_bio_PrivateKey(bio, NULL, NULL, NULL);
    BIO_free(bio);
    return privateKey;
}

} // namespace SslHelper
