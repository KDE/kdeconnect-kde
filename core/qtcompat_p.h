/*
    SPDX-FileCopyrightText: 2017 Kevin Funk <kfunk@.kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KDEVELOP_QTCOMPAT_P_H
#define KDEVELOP_QTCOMPAT_P_H

#include <qglobal.h>

#include <QDir>

#if QT_VERSION < QT_VERSION_CHECK(5, 7, 0)
namespace QtPrivate
{
template<typename T>
struct QAddConst {
    typedef const T Type;
};
}

// this adds const to non-const objects (like std::as_const)
template<typename T>
Q_DECL_CONSTEXPR typename QtPrivate::QAddConst<T>::Type &qAsConst(T &t) Q_DECL_NOTHROW
{
    return t;
}
// prevent rvalue arguments:
template<typename T>
void qAsConst(const T &&) Q_DECL_EQ_DELETE;
#endif

#if QT_VERSION < QT_VERSION_CHECK(5, 7, 0)
template<typename... Args>
struct QNonConstOverload {
    template<typename R, typename T>
    static constexpr auto of(R (T::*func)(Args...)) noexcept -> decltype(func)
    {
        return func;
    }
};

template<typename... Args>
struct QConstOverload {
    template<typename R, typename T>
    static constexpr auto of(R (T::*func)(Args...) const) noexcept -> decltype(func)
    {
        return func;
    }
};

template<typename... Args>
struct QOverload : QConstOverload<Args...>, QNonConstOverload<Args...> {
    using QConstOverload<Args...>::of;
    using QNonConstOverload<Args...>::of;

    template<typename R>
    static constexpr auto of(R (*func)(Args...)) noexcept -> decltype(func)
    {
        return func;
    }
};
#endif

// compat for Q_FALLTHROUGH
#if QT_VERSION < QT_VERSION_CHECK(5, 8, 0)

#if defined(__has_cpp_attribute)
#if __has_cpp_attribute(fallthrough)
#define Q_FALLTHROUGH() [[fallthrough]]
#elif __has_cpp_attribute(clang::fallthrough)
#define Q_FALLTHROUGH() [[clang::fallthrough]]
#elif __has_cpp_attribute(gnu::fallthrough)
#define Q_FALLTHROUGH() [[gnu::fallthrough]]
#endif
#endif

#ifndef Q_FALLTHROUGH
#if defined(__GNUC__) && !defined(__INTEL_COMPILER) && (((__GNUC__ * 100) + __GNUC_MINOR__) >= 700)
#define Q_FALLTHROUGH() __attribute__((fallthrough))
#else
#define Q_FALLTHROUGH() (void)0
#endif
#endif

#endif

namespace QtCompat
{
// TODO: Just use QDir::listSeparator once we depend on Qt 5.6
Q_DECL_CONSTEXPR inline QChar listSeparator() Q_DECL_NOTHROW
{
#if QT_VERSION < QT_VERSION_CHECK(5, 6, 0)
#ifdef Q_OS_WIN
    return QLatin1Char(';');
#else
    return QLatin1Char(':');
#endif
#else
    return QDir::listSeparator();
#endif
}
}

#endif
