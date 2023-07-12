/**
 * SPDX-FileCopyrightText: 2023 Albert Vaca <albertvaka@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef SSLHELPER_H
#define SSLHELPER_H

#include <openssl/evp.h>
#include <openssl/x509.h>

#include <QByteArray>
#include <QString>

namespace SslHelper
{

// Delete with X509_free(certificate);
X509 *generateSelfSignedCertificate(EVP_PKEY *privateKey, const QString &commonName);

// Delete with EVP_PKEY_free(privateKey);
EVP_PKEY *generateRsaPrivateKey();

QByteArray certificateToPEM(X509 *certificate);

QByteArray privateKeyToPEM(EVP_PKEY *privateKey);
EVP_PKEY *pemToRsaPrivateKey(const QByteArray &privateKeyPem);

}

#endif