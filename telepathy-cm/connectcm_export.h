#ifndef CONNECTCM_EXPORT_H
#define CONNECTCM_EXPORT_H

#include <QtCore/QtGlobal>

#if defined(CONNECTCM_LIBRARY)
#define CONNECTCM_EXPORT Q_DECL_EXPORT
#else
#define CONNECTCM_EXPORT Q_DECL_IMPORT
#endif

#endif
