/*****************************************************************************
 *   Copyright (C) 2004-2012 by Thomas Fischer <fischer@unix-ag.uni-kl.de>   *
 *                                                                           *
 *                                                                           *
 *   This program is free software; you can redistribute it and/or modify    *
 *   it under the terms of the GNU General Public License as published by    *
 *   the Free Software Foundation; either version 2 of the License, or       *
 *   (at your option) any later version.                                     *
 *                                                                           *
 *   This program is distributed in the hope that it will be useful,         *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of          *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 *   GNU General Public License for more details.                            *
 *                                                                           *
 *   You should have received a copy of the GNU General Public License       *
 *   along with this program; if not, write to the                           *
 *   Free Software Foundation, Inc.,                                         *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.               *
 *****************************************************************************/

#include <QSet>

#include <KDebug>

#include "notificationhub.h"

NotificationListener::~NotificationListener()
{
    NotificationHub::unregisterNotificationListener(this);
}

class NotificationHub::NotificationHubPrivate
{
private:
    NotificationHub *p;

public:
    static NotificationHub *singleton;
    QHash<int, QSet<NotificationListener *> > listenersPerEventId;
    QSet<NotificationListener *> allListeners;

    NotificationHubPrivate(NotificationHub *parent)
        : p(parent) {
        // nothing
    }
};

NotificationHub::NotificationHub()
    : d(new NotificationHubPrivate(this))
{
    // TODO
}

NotificationHub *NotificationHub::getHub()
{
    if (NotificationHub::NotificationHubPrivate::singleton == NULL)
        NotificationHub::NotificationHubPrivate::singleton = new NotificationHub();
    return NotificationHub::NotificationHubPrivate::singleton;
}

void NotificationHub::registerNotificationListener(NotificationListener *listener, int eventId)
{
    NotificationHub::NotificationHubPrivate *d = getHub()->d;
    if (eventId != EventAny) {
        QSet< NotificationListener *> set = d->listenersPerEventId.value(eventId,  QSet<NotificationListener *>());
        set.insert(listener);
    }
    d->allListeners.insert(listener);
}

void NotificationHub::unregisterNotificationListener(NotificationListener *listener, int eventId)
{
    NotificationHub::NotificationHubPrivate *d = getHub()->d;
    if (eventId == EventAny) {
        for (QHash<int, QSet<NotificationListener *> >::Iterator it = d->listenersPerEventId.begin(); it != d->listenersPerEventId.end(); ++it)
            it.value().remove(listener);
    } else {
        QSet< NotificationListener *> set = d->listenersPerEventId.value(eventId,  QSet<NotificationListener *>());
        set.remove(listener);
    }
    d->allListeners.remove(listener);
}

void NotificationHub::publishEvent(int eventId)
{
    NotificationHub::NotificationHubPrivate *d = getHub()->d;
    if (eventId >= 0) {
        kDebug() << "Notifying about event" << eventId;

        QSet< NotificationListener *> set(d->listenersPerEventId.value(eventId,  QSet<NotificationListener *>()));
        foreach(NotificationListener * listener, d->allListeners) {
            set.insert(listener);
        }
        foreach(NotificationListener * listener, set) {
            listener->notificationEvent(eventId);
        }
        kDebug() << set.count() << d->allListeners.count() << d->listenersPerEventId.value(eventId,  QSet<NotificationListener *>()).count();
    }
}

const int NotificationHub::EventAny = -1;
const int NotificationHub::EventConfigurationChanged = 0;
const int NotificationHub::EventUserDefined = 1024;
NotificationHub *NotificationHub::NotificationHubPrivate::singleton = NULL;