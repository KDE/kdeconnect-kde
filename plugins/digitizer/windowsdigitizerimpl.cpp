#include "windowsdigitizerimpl.h"

#include <core/daemon.h>

#include "plugin_digitizer_debug.h"
#include "toolevent.h"

#include <KLocalizedString>

#include <QFile>
#include <QGuiApplication>
#include <QScreen>
#include <QStandardPaths>
#include <QTextStream>

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0A00
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

// Toggle file-based diagnostic logging.  Set to true to get a per-event
// trace in %TEMP%\kdeconnect_digitizer.log while the plugin is active.
static constexpr bool kDigitizerFileLog = false;

// ---- Function pointer types for synthetic pointer injection ----

typedef HANDLE(WINAPI *CreateSyntheticPointerDevice_t)(POINTER_INPUT_TYPE, ULONG, POINTER_FEEDBACK_MODE);
typedef BOOL(WINAPI *InjectSyntheticPointerInput_t)(HANDLE, const POINTER_TYPE_INFO *, UINT32);
typedef BOOL(WINAPI *DestroySyntheticPointerDevice_t)(HANDLE);

// ---- Implementation ----

bool WindowsDigitizerImpl::resolveApi()
{
    HMODULE user32 = GetModuleHandleW(L"user32.dll");
    if (!user32) {
        user32 = LoadLibraryW(L"user32.dll");
        if (!user32) {
            qWarning(KDECONNECT_PLUGIN_DIGITIZER) << "Failed to load user32.dll";
            return false;
        }
    }

    FARPROC createProc = GetProcAddress(user32, "CreateSyntheticPointerDevice");
    FARPROC injectProc = GetProcAddress(user32, "InjectSyntheticPointerInput");
    FARPROC destroyProc = GetProcAddress(user32, "DestroySyntheticPointerDevice");

    if (!createProc || !injectProc || !destroyProc) {
        qWarning(KDECONNECT_PLUGIN_DIGITIZER) << "Pointer injection API not available (requires Windows 10 1809+)";
        return false;
    }

    static_assert(sizeof(void *) == sizeof(FARPROC), "Pointer size mismatch");
    memcpy(&m_createDeviceFunc, &createProc, sizeof(FARPROC));
    memcpy(&m_injectInputFunc, &injectProc, sizeof(FARPROC));
    memcpy(&m_destroyDeviceFunc, &destroyProc, sizeof(FARPROC));
    return true;
}

WindowsDigitizerImpl::WindowsDigitizerImpl(QObject *parent, const Device *device)
    : AbstractDigitizerImpl(parent, device)
    , m_pointerDevice(nullptr)
    , m_createDeviceFunc(nullptr)
    , m_injectInputFunc(nullptr)
    , m_destroyDeviceFunc(nullptr)
    , m_available(false)
    , m_inContact(false)
    , m_sessionWidth(0)
    , m_sessionHeight(0)
    , m_screenWidth(0)
    , m_screenHeight(0)
{
    m_available = resolveApi();

    if (!m_available) {
        qWarning(KDECONNECT_PLUGIN_DIGITIZER) << "WindowsDigitizerImpl: API not available, plugin will be non-functional.";
        return;
    }

    QScreen *screen = QGuiApplication::primaryScreen();
    if (screen) {
        QRect geo = screen->geometry();
        m_screenWidth = geo.width();
        m_screenHeight = geo.height();
        qCDebug(KDECONNECT_PLUGIN_DIGITIZER) << "Screen size:" << m_screenWidth << "x" << m_screenHeight;
    }
}

WindowsDigitizerImpl::~WindowsDigitizerImpl()
{
    endSession();
}

void WindowsDigitizerImpl::startSession(int width, int height, int resolutionX, int resolutionY)
{
    Q_UNUSED(resolutionX);
    Q_UNUSED(resolutionY);

    if (!m_available) {
        qCWarning(KDECONNECT_PLUGIN_DIGITIZER) << "Cannot start session: pointer injection API not available.";
        return;
    }

    if (m_pointerDevice) {
        endSession();
    }

    m_sessionWidth = width;
    m_sessionHeight = height;
    m_inContact = false;
    m_strokeTimer.start();

    auto createFunc = reinterpret_cast<CreateSyntheticPointerDevice_t>(m_createDeviceFunc);
    m_pointerDevice = createFunc(PT_PEN, 1, POINTER_FEEDBACK_DEFAULT);

    if (m_pointerDevice == nullptr) {
        qCCritical(KDECONNECT_PLUGIN_DIGITIZER) << "Failed to create synthetic pointer device. Last error:" << GetLastError();

        Daemon::instance()->sendSimpleNotification(QStringLiteral("digitizerError"),
                                                   i18n("KDE Connect Error"),
                                                   i18n("Unable to create a virtual drawing tablet. If this error persists, file a bug report."),
                                                   QStringLiteral("error"));
        return;
    }

    qCDebug(KDECONNECT_PLUGIN_DIGITIZER) << "Started drawing tablet session.";

    if constexpr (kDigitizerFileLog) {
        QString logPath = QStandardPaths::writableLocation(QStandardPaths::TempLocation) + QStringLiteral("/kdeconnect_digitizer.log");
        QFile::remove(logPath);
        qCDebug(KDECONNECT_PLUGIN_DIGITIZER) << "Digitizer debug log:" << logPath;
    }
}

void WindowsDigitizerImpl::endSession()
{
    if (m_pointerDevice) {
        if (m_inContact) {
            injectPointerUp();
        }

        auto destroyFunc = reinterpret_cast<DestroySyntheticPointerDevice_t>(m_destroyDeviceFunc);
        destroyFunc(m_pointerDevice);
        m_pointerDevice = nullptr;
        m_inContact = false;
    }

    qCDebug(KDECONNECT_PLUGIN_DIGITIZER) << "Ended drawing tablet session.";
}

bool WindowsDigitizerImpl::injectPointerUp()
{
    POINTER_TYPE_INFO info = {};
    info.type = PT_PEN;
    info.penInfo.pointerInfo.pointerType = PT_PEN;
    info.penInfo.pointerInfo.pointerId = 1;
    info.penInfo.penMask = PEN_MASK_PRESSURE;
    info.penInfo.pointerInfo.pointerFlags = POINTER_FLAG_UP | POINTER_FLAG_INRANGE;
    info.penInfo.pointerInfo.sourceDevice = nullptr;
    info.penInfo.pointerInfo.hwndTarget = nullptr;

    auto injectFunc = reinterpret_cast<InjectSyntheticPointerInput_t>(m_injectInputFunc);
    if (!injectFunc(m_pointerDevice, &info, 1)) {
        qCCritical(KDECONNECT_PLUGIN_DIGITIZER) << "Failed to inject pointer UP event. Error:" << GetLastError();
        Daemon::instance()->sendSimpleNotification(
            QStringLiteral("digitizerError"),
            i18n("KDE Connect Error"),
            i18n("Unable to process one or more input events for the virtual device. If this error persists, file a bug report."),
            QStringLiteral("error"));
        return false;
    }
    return true;
}

void WindowsDigitizerImpl::onToolEvent(ToolEvent event)
{
    if (!m_available || !m_pointerDevice || m_screenWidth <= 0 || m_screenHeight <= 0) {
        return;
    }

    // Handle active=false: remote device stopped tracking
    if (event.active.has_value() && !event.active.value()) {
        if (m_inContact) {
            injectPointerUp();
            m_inContact = false;
        }
        return;
    }

    // ---- Gap detection ----
    // When the user lifts the pen, some phones (e.g. Redmi Note 10) don't
    // send an explicit active=false — they simply stop sending events.
    // Detect the resulting >200ms gap and automatically end the stroke.
    if (m_inContact && m_strokeTimer.hasExpired(200)) {
        injectPointerUp();
        m_inContact = false;
    }
    m_strokeTimer.restart();

    // Map phone coordinates to screen coordinates
    LONG mappedX = 0;
    LONG mappedY = 0;
    if (event.x.has_value() && m_sessionWidth > 0) {
        mappedX = static_cast<LONG>(static_cast<qint64>(event.x.value()) * m_screenWidth / m_sessionWidth);
    }
    if (event.y.has_value() && m_sessionHeight > 0) {
        mappedY = static_cast<LONG>(static_cast<qint64>(event.y.value()) * m_screenHeight / m_sessionHeight);
    }

    // Determine contact state.
    //
    // Some Android phones (e.g. Redmi Note 10) NEVER send touching=true
    // or pressure>0 — touching is always false when present, pressure is
    // always 0.  They merely send tool+coordinates while the user draws.
    // Therefore we cannot rely on touching or pressure alone to detect
    // contact.
    //
    // Strategy:
    //   - explicit touching=true or pressure>0  → contact
    //   - explicit touching=false  → defer to m_inContact; if we are
    //     already drawing, the phone may be sending a periodic "heartbeat"
    //     with touching=false while the user is still touching; if we are
    //     not drawing, stay out of contact.
    //   - coordinates present, no touching / pressure  → contact
    //     (the phone is sending coordinate deltas while touching)
    //   - otherwise → keep previous state
    bool shouldBeInContact;
    if (event.touching.has_value() && event.touching.value()) {
        shouldBeInContact = true;
    } else if (event.pressure.has_value() && event.pressure.value() > 0.0) {
        shouldBeInContact = true;
    } else if (event.touching.has_value() && !event.touching.value()) {
        shouldBeInContact = m_inContact;
    } else if (event.x.has_value() && event.y.has_value()) {
        shouldBeInContact = true;
    } else {
        shouldBeInContact = m_inContact;
    }

    bool isDown = shouldBeInContact && !m_inContact;
    bool isUp = !shouldBeInContact && m_inContact;

    auto injectFunc = reinterpret_cast<InjectSyntheticPointerInput_t>(m_injectInputFunc);

    if constexpr (kDigitizerFileLog) {
        if (isDown) {
            Daemon::instance()->sendSimpleNotification(
                QStringLiteral("digitizerDebug"),
                i18n("Digitizer Debug"),
                i18n("DOWN detected (contact start). touching=%1 pressure=%2")
                    .arg(event.touching.has_value() ? (event.touching.value() ? QStringLiteral("true") : QStringLiteral("false")) : QStringLiteral("null"))
                    .arg(event.pressure.has_value() ? QString::number(event.pressure.value()) : QStringLiteral("null")),
                QStringLiteral("kdeconnect"));
        } else if (isUp) {
            Daemon::instance()->sendSimpleNotification(QStringLiteral("digitizerDebug"),
                                                       i18n("Digitizer Debug"),
                                                       i18n("UP detected (contact end)."),
                                                       QStringLiteral("kdeconnect"));
        }
    }

    // When initiating contact, first inject a HOVER event so the ink
    // stack initializes a proper pressured stroke. The reference
    // implementation (draw-stream / usb-pen-injection) requires this
    // two-step sequence to avoid a "flat tap".
    if (isDown) {
        POINTER_TYPE_INFO hover = {};
        hover.type = PT_PEN;
        hover.penInfo.pointerInfo.pointerType = PT_PEN;
        hover.penInfo.pointerInfo.pointerId = 1;
        hover.penInfo.pointerInfo.sourceDevice = nullptr;
        hover.penInfo.pointerInfo.hwndTarget = nullptr;
        hover.penInfo.pointerInfo.ptPixelLocation.x = mappedX;
        hover.penInfo.pointerInfo.ptPixelLocation.y = mappedY;
        hover.penInfo.pointerInfo.dwTime = 0;
        hover.penInfo.pointerInfo.historyCount = 0;
        hover.penInfo.pointerInfo.InputData = 0;
        hover.penInfo.pointerInfo.dwKeyStates = 0;
        hover.penInfo.pointerInfo.PerformanceCount = 0;
        hover.penInfo.pointerInfo.ButtonChangeType = POINTER_CHANGE_NONE;
        hover.penInfo.penMask = PEN_MASK_PRESSURE;
        hover.penInfo.pointerInfo.pointerFlags = POINTER_FLAG_UPDATE | POINTER_FLAG_INRANGE;

        if (!injectFunc(m_pointerDevice, &hover, 1)) {
            qCCritical(KDECONNECT_PLUGIN_DIGITIZER) << "Failed to inject HOVER event. Error:" << GetLastError();
        }
    }

    m_inContact = shouldBeInContact;

    // ---- Diagnostic file logging (toggle with kDigitizerFileLog above) ----
    if constexpr (kDigitizerFileLog) {
        QString logPath = QStandardPaths::writableLocation(QStandardPaths::TempLocation) + QStringLiteral("/kdeconnect_digitizer.log");
        QFile logFile(logPath);
        if (logFile.open(QIODevice::Append | QIODevice::Text)) {
            QTextStream out(&logFile);
            out << "active=" << (event.active.has_value() ? (event.active.value() ? QStringLiteral("true") : QStringLiteral("false")) : QStringLiteral("null"))
                << " touching="
                << (event.touching.has_value() ? (event.touching.value() ? QStringLiteral("true") : QStringLiteral("false")) : QStringLiteral("null"))
                << " tool=" << (event.tool.has_value() ? event.tool.value() : QStringLiteral("null"))
                << " x=" << (event.x.has_value() ? QString::number(event.x.value()) : QStringLiteral("null"))
                << " y=" << (event.y.has_value() ? QString::number(event.y.value()) : QStringLiteral("null"))
                << " pressure=" << (event.pressure.has_value() ? QString::number(event.pressure.value()) : QStringLiteral("null"))
                << " contact=" << (shouldBeInContact ? QStringLiteral("true") : QStringLiteral("false"))
                << " m_inContact=" << (m_inContact ? QStringLiteral("true") : QStringLiteral("false")) << QStringLiteral("\n");
        }
    }
    // ----

    // Build the pointer type info
    POINTER_TYPE_INFO info = {};
    info.type = PT_PEN;

    POINTER_PEN_INFO &pen = info.penInfo;

    pen.pointerInfo.pointerType = PT_PEN;
    pen.pointerInfo.pointerId = 1;
    pen.pointerInfo.sourceDevice = nullptr;
    pen.pointerInfo.hwndTarget = nullptr;
    pen.pointerInfo.ptPixelLocation.x = mappedX;
    pen.pointerInfo.ptPixelLocation.y = mappedY;
    pen.pointerInfo.dwTime = 0;
    pen.pointerInfo.historyCount = 0;
    pen.pointerInfo.InputData = 0;
    pen.pointerInfo.dwKeyStates = 0;
    pen.pointerInfo.PerformanceCount = 0;
    pen.pointerInfo.ButtonChangeType = POINTER_CHANGE_NONE;

    // Always set penMask so Windows knows the pen data fields are valid
    pen.penMask = PEN_MASK_PRESSURE;

    // Set pointer flags (reference pattern from usb-pen-injection / draw-stream)
    if (isDown) {
        pen.pointerInfo.pointerFlags = POINTER_FLAG_DOWN | POINTER_FLAG_INRANGE | POINTER_FLAG_INCONTACT;
    } else if (isUp) {
        pen.pointerInfo.pointerFlags = POINTER_FLAG_UP | POINTER_FLAG_INRANGE;
    } else if (m_inContact) {
        pen.pointerInfo.pointerFlags = POINTER_FLAG_UPDATE | POINTER_FLAG_INRANGE | POINTER_FLAG_INCONTACT;
    } else {
        pen.pointerInfo.pointerFlags = POINTER_FLAG_UPDATE | POINTER_FLAG_INRANGE;
    }

    // Set pressure (draw-stream reference always provides it)
    if (event.pressure.has_value()) {
        pen.pressure = static_cast<UINT32>(event.pressure.value() * 1024.0);
        if (pen.pressure > 1024) {
            pen.pressure = 1024;
        }
    }

    // Set tool type (Pen vs Eraser)
    if (event.tool.has_value()) {
        if (event.tool.value() == TOOL_RUBBER) {
            pen.penFlags |= PEN_FLAG_INVERTED | PEN_FLAG_ERASER;
        }
    }

    if (!injectFunc(m_pointerDevice, &info, 1)) {
        qCCritical(KDECONNECT_PLUGIN_DIGITIZER) << "Failed to inject pointer event. Error:" << GetLastError();

        Daemon::instance()->sendSimpleNotification(
            QStringLiteral("digitizerError"),
            i18n("KDE Connect Error"),
            i18n("Unable to process one or more input events for the virtual device. If this error persists, file a bug report."),
            QStringLiteral("error"));
    }
}

#include "moc_windowsdigitizerimpl.cpp"
