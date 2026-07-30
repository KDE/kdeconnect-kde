/**
 * SPDX-FileCopyrightText: 2026 Johann Specht <sajeg.dev@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#define PACKET_TYPE_RUNCOMMAND_OUTPUT QStringLiteral("kdeconnect.runcommand.output")

/**
 * Packet used to send the output of the commands that get executed through PACKET_TYPE_RUNCOMMAND
 *
 * The body should look like so:
 * "stderr": List<String>,          // List of type String that contains the output of the Error Channel
 * "stdout": List<String>,          // List of type String that contains the output of the Standard Channel
 * "commandFinished": Boolean       // Boolean that is true if this is the last output of the command
 *
 * Note: Often only stdout or stderr will contain Strings, not both. The other one will just have an empty list.
 */