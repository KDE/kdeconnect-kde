/**
 * SPDX-FileCopyrightText: 2013 Albert Vaca <albertvaka@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "core_debug.h"

#if defined(__GNU_LIBRARY__)
#include <execinfo.h>
#include <stdlib.h>
#include <unistd.h>
#endif

void logBacktrace()
{
#if defined(__GNU_LIBRARY__)
    void *array[32];
    size_t size = backtrace(array, 32);
    char **strings = backtrace_symbols(array, size);
    backtrace_symbols_fd(array, size, STDERR_FILENO);
    free(strings);
#endif
}
