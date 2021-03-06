/***************************************************************************
 *   SPDX-License-Identifier: GPL-2.0-or-later
 *                                                                         *
 *   SPDX-FileCopyrightText: 2004-2019 Thomas Fischer <fischer@unix-ag.uni-kl.de>
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, see <https://www.gnu.org/licenses/>. *
 ***************************************************************************/

#include "notificationhub.h"

#include <QSet>

#include "logging_config.h"

NotificationListener::~NotificationListener()
{
    NotificationHub::unregisterNotificationListener(this);
}

class NotificationHub::NotificationHubPrivate
{
public:
    QHash<int, QSet<NotificationListener *> > listenersPerEventId;
    QSet<NotificationListener *> listenersAnyEvent;

    static const QHash<int, QString> eventIdToName;

    NotificationHubPrivate(NotificationHub *)
    {
        /// nothing
    }
};

const QHash<int, QString> NotificationHub::NotificationHubPrivate::eventIdToName{
    {NotificationHub::EventAny, QStringLiteral("Any")},
    {NotificationHub::EventConfigurationChanged, QStringLiteral("ConfigurationChanged")},
    {NotificationHub::EventBibliographySystemChanged, QStringLiteral("BibliographySystemChanged")},
    {NotificationHub::EventUserDefined, QStringLiteral("UserDefined")}
};

NotificationHub::NotificationHub()
        : d(new NotificationHubPrivate(this))
{
    /// nothing
}

NotificationHub::~NotificationHub()
{
    delete d;
}

NotificationHub &NotificationHub::instance()
{
    static NotificationHub singleton;
    return singleton;
}

void NotificationHub::registerNotificationListener(NotificationListener *listener, int eventId)
{
    NotificationHub::NotificationHubPrivate *d = instance().d;
    if (eventId == EventAny)
        d->listenersAnyEvent.insert(listener);
    else {
        QSet<NotificationListener *> set = d->listenersPerEventId.value(eventId,  QSet<NotificationListener *>());
        set.insert(listener);
        d->listenersPerEventId.insert(eventId, set);
    }
}

void NotificationHub::unregisterNotificationListener(NotificationListener *listener, int eventId)
{
    NotificationHub::NotificationHubPrivate *d = instance().d;
    if (eventId == EventAny) {
        for (QHash<int, QSet<NotificationListener *> >::Iterator it = d->listenersPerEventId.begin(); it != d->listenersPerEventId.end(); ++it)
            it.value().remove(listener);
    } else {
        QSet<NotificationListener *> set = d->listenersPerEventId.value(eventId,  QSet<NotificationListener *>());
        set.remove(listener);
        d->listenersPerEventId.insert(eventId, set);
    }
    d->listenersAnyEvent.remove(listener);
}

void NotificationHub::publishEvent(int eventId)
{
    if (eventId >= 0) {
        NotificationHub::NotificationHubPrivate *d = instance().d;
        QSet<NotificationListener *> set(d->listenersPerEventId.value(eventId,  QSet<NotificationListener *>()));
        for (NotificationListener *listener : const_cast<const QSet<NotificationListener *> &>(d->listenersAnyEvent))
            set.insert(listener);
        qCDebug(LOG_KBIBTEX_CONFIG) << "Notifying about event" << NotificationHubPrivate::eventIdToName.value(eventId, QString::number(eventId)) << "having" << set.count() << "receivers";
        for (NotificationListener *listener : const_cast<const QSet<NotificationListener *> &>(set))
            listener->notificationEvent(eventId);
    }
}

const int NotificationHub::EventAny = -1;
const int NotificationHub::EventConfigurationChanged = 0;
const int NotificationHub::EventBibliographySystemChanged = 1;
const int NotificationHub::EventUserDefined = 1024;
