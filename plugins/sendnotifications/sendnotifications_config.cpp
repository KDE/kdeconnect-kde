/**
 * SPDX-FileCopyrightText: 2015 Holger Kaelberer <holger.k@elberer.de>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "sendnotifications_config.h"
#include "notifyingapplicationmodel.h"
#include "ui_sendnotifications_config.h"

#include <KCModule>
#include <KPluginFactory>

K_PLUGIN_FACTORY(SendNotificationsConfigFactory, registerPlugin<SendNotificationsConfig>();)

SendNotificationsConfig::SendNotificationsConfig(QObject *parent, const QVariantList &args)
    : KdeConnectPluginKcm(parent, args, QStringLiteral("kdeconnect_sendnotifications"))
    , m_ui(new Ui::SendNotificationsConfigUi())
    , appModel(new NotifyingApplicationModel)
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    qRegisterMetaTypeStreamOperators<NotifyingApplication>("NotifyingApplication");
#endif

    m_ui->setupUi(widget());
    m_ui->appList->setIconSize(QSize(32, 32));

    m_ui->appList->setModel(appModel);

    m_ui->appList->horizontalHeader()->setSectionResizeMode(0, QHeaderView::QHeaderView::Fixed);
    m_ui->appList->horizontalHeader()->setSectionResizeMode(1, QHeaderView::QHeaderView::Stretch);
    m_ui->appList->horizontalHeader()->setSectionResizeMode(2, QHeaderView::QHeaderView::Stretch);
    for (int i = 0; i < 3; i++)
        m_ui->appList->resizeColumnToContents(i);

    connect(m_ui->appList->horizontalHeader(), SIGNAL(sortIndicatorChanged(int, Qt::SortOrder)), m_ui->appList, SLOT(sortByColumn(int)));

    connect(m_ui->check_persistent, SIGNAL(toggled(bool)), this, SLOT(changed()));
    connect(m_ui->spin_urgency, SIGNAL(editingFinished()), this, SLOT(changed()));
    connect(m_ui->check_body, SIGNAL(toggled(bool)), this, SLOT(changed()));
    connect(m_ui->check_icons, SIGNAL(toggled(bool)), this, SLOT(changed()));

    connect(appModel, SIGNAL(applicationsChanged()), this, SLOT(changed()));

    connect(config(), &KdeConnectPluginConfig::configChanged, this, &SendNotificationsConfig::loadApplications);
}

SendNotificationsConfig::~SendNotificationsConfig()
{
    delete m_ui;
}

void SendNotificationsConfig::defaults()
{
    KCModule::defaults();
    m_ui->check_persistent->setChecked(false);
    m_ui->spin_urgency->setValue(0);
    m_ui->check_body->setChecked(true);
    m_ui->check_icons->setChecked(true);
    markAsChanged();
}

void SendNotificationsConfig::loadApplications()
{
    appModel->clearApplications();
    QVariantList list = config()->getList(QStringLiteral("applications"));
    for (const auto &a : list) {
        NotifyingApplication app = a.value<NotifyingApplication>();
        if (!appModel->containsApp(app.name)) {
            appModel->appendApp(app);
        }
    }
}

void SendNotificationsConfig::load()
{
    KCModule::load();
    bool persistent = config()->getBool(QStringLiteral("generalPersistent"), false);
    m_ui->check_persistent->setChecked(persistent);
    bool body = config()->getBool(QStringLiteral("generalIncludeBody"), true);
    m_ui->check_body->setChecked(body);
    bool icons = config()->getBool(QStringLiteral("generalSynchronizeIcons"), true);
    m_ui->check_icons->setChecked(icons);
    int urgency = config()->getInt(QStringLiteral("generalUrgency"), 0);
    m_ui->spin_urgency->setValue(urgency);

    loadApplications();
}

void SendNotificationsConfig::save()
{
    KCModule::save();
    config()->set(QStringLiteral("generalPersistent"), m_ui->check_persistent->isChecked());
    config()->set(QStringLiteral("generalIncludeBody"), m_ui->check_body->isChecked());
    config()->set(QStringLiteral("generalSynchronizeIcons"), m_ui->check_icons->isChecked());
    config()->set(QStringLiteral("generalUrgency"), m_ui->spin_urgency->value());

    QVariantList list;
    const auto apps = appModel->apps();
    list.reserve(apps.size());
    for (const auto &a : apps) {
        list.append(QVariant::fromValue<NotifyingApplication>(a));
    }
    config()->setList(QStringLiteral("applications"), list);
}

#include "sendnotifications_config.moc"
