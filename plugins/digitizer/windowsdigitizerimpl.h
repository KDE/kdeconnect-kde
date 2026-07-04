#pragma once

#include "abstractdigitizerimpl.h"

#include <QElapsedTimer>
#include <QObject>

class WindowsDigitizerImpl : public AbstractDigitizerImpl
{
    Q_OBJECT

public:
    explicit WindowsDigitizerImpl(QObject *parent, const Device *device);
    ~WindowsDigitizerImpl() override;

    void startSession(int width, int height, int resolutionX, int resolutionY) override;
    void endSession() override;
    void onToolEvent(ToolEvent event) override;

private:
    bool resolveApi();
    bool injectPointerUp();

    // Opaque handle for the synthetic pointer device (HSYNTHETICPOINTERDEVICE)
    void *m_pointerDevice;
    // Function pointers loaded dynamically from user32.dll
    void *m_createDeviceFunc;
    void *m_injectInputFunc;
    void *m_destroyDeviceFunc;
    bool m_available;
    bool m_inContact;
    QElapsedTimer m_strokeTimer;
    int m_sessionWidth;
    int m_sessionHeight;
    int m_screenWidth;
    int m_screenHeight;
};
