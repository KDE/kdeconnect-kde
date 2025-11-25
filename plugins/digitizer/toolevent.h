/**
 * SPDX-FileCopyrightText: 2025 Martin Sh <hemisputnik@proton.me>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <QString>
#include <optional>

#define TOOL_PEN QStringLiteral("Pen")
#define TOOL_RUBBER QStringLiteral("Rubber")

struct ToolEvent {
    std::optional<bool> active;
    std::optional<bool> touching;
    std::optional<QString> tool;
    std::optional<int> x;
    std::optional<int> y;
    std::optional<double> pressure;
};
