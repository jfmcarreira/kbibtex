/***************************************************************************
 *   Copyright (C) 2004-2017 by Thomas Fischer <fischer@unix-ag.uni-kl.de> *
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

#include "internalnetworkaccessmanager.h"

#include <ctime>

#include <QStringList>
#include <QRegExp>
#include <QNetworkAccessManager>
#include <QNetworkCookieJar>
#include <QNetworkCookie>
#include <QNetworkProxy>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QtGlobal>
#include <QApplication>
#include <QTimer>
#include <QUrl>

#include <KProtocolManager>

#include "logging_networking.h"

/**
 * @author Thomas Fischer <fischer@unix-ag.uni-kl.de>
 */
class InternalNetworkAccessManager::HTTPEquivCookieJar: public QNetworkCookieJar
{
    Q_OBJECT

public:
    void mergeHtmlHeadCookies(const QString &htmlCode, const QUrl &url) {
        static QRegExp cookieContent("^([^\"=; ]+)=([^\"=; ]+).*\\bpath=([^\"=; ]+)", Qt::CaseInsensitive);
        int p1 = -1;
        if ((p1 = htmlCode.toLower().indexOf(QStringLiteral("http-equiv=\"set-cookie\""), 0, Qt::CaseInsensitive)) >= 5
                && (p1 = htmlCode.lastIndexOf(QStringLiteral("<meta"), p1, Qt::CaseInsensitive)) >= 0
                && (p1 = htmlCode.indexOf(QStringLiteral("content=\""), p1, Qt::CaseInsensitive)) >= 0
                && cookieContent.indexIn(htmlCode.mid(p1 + 9, 256)) >= 0) {
            const QString key = cookieContent.cap(1);
            const QString value = cookieContent.cap(2);
            QUrl cookieUrl = url;
            QList<QNetworkCookie> cookies = cookiesForUrl(cookieUrl);
            cookies.append(QNetworkCookie(key.toLatin1(), value.toLatin1()));
            setCookiesFromUrl(cookies, cookieUrl);
        }
    }

    HTTPEquivCookieJar(QObject *parent = nullptr)
            : QNetworkCookieJar(parent) {
        // nothing
    }
};


QString InternalNetworkAccessManager::userAgentString;
InternalNetworkAccessManager *InternalNetworkAccessManager::instance = nullptr;

InternalNetworkAccessManager::InternalNetworkAccessManager(QObject *parent)
        : QNetworkAccessManager(parent)
{
    cookieJar = new HTTPEquivCookieJar(this);
}


void InternalNetworkAccessManager::mergeHtmlHeadCookies(const QString &htmlCode, const QUrl &url)
{
    Q_ASSERT_X(cookieJar != nullptr, "void InternalNetworkAccessManager::mergeHtmlHeadCookies(const QString &htmlCode, const QUrl &url)", "cookieJar is invalid");
    cookieJar->mergeHtmlHeadCookies(htmlCode, url);
    setCookieJar(cookieJar);
}

InternalNetworkAccessManager *InternalNetworkAccessManager::self()
{
    if (instance == nullptr) {
        instance = new InternalNetworkAccessManager(QApplication::instance());
    }

    return instance;
}

QNetworkReply *InternalNetworkAccessManager::get(QNetworkRequest &request, const QUrl &oldUrl)
{
    /// Query the KDE subsystem if a proxy has to be used
    /// for the host of a given URL
    QString proxyHostName = KProtocolManager::proxyForUrl(request.url());
    if (!proxyHostName.isEmpty() && proxyHostName != QStringLiteral("DIRECT")) {
        /// Extract both hostname and port number for proxy
        proxyHostName = proxyHostName.mid(proxyHostName.indexOf(QStringLiteral("://")) + 3);
        QStringList proxyComponents = proxyHostName.split(QStringLiteral(":"), QString::SkipEmptyParts);
        if (proxyComponents.length() == 1) {
            /// Proxy configuration is missing a port number,
            /// using 8080 as default
            proxyComponents << QStringLiteral("8080");
        }
        if (proxyComponents.length() == 2) {
            /// Set proxy to Qt's NetworkAccessManager
            setProxy(QNetworkProxy(QNetworkProxy::HttpProxy, proxyComponents[0], proxyComponents[1].toInt()));
        }
    } else {
        /// No proxy to be used, clear previous settings
        setProxy(QNetworkProxy());
    }

    if (!request.hasRawHeader(QByteArray("Accept")))
        request.setRawHeader(QByteArray("Accept"), QByteArray("text/*, */*;q=0.7"));
    request.setRawHeader(QByteArray("Accept-Charset"), QByteArray("utf-8, us-ascii, ISO-8859-1;q=0.7, ISO-8859-15;q=0.7, windows-1252;q=0.3"));
    request.setRawHeader(QByteArray("Accept-Language"), QByteArray("en-US, en;q=0.9"));
    request.setRawHeader(QByteArray("User-Agent"), userAgent().toLatin1());
    if (oldUrl.isValid())
        request.setRawHeader(QByteArray("Referer"), oldUrl.toDisplayString().toLatin1());
    QNetworkReply *reply = QNetworkAccessManager::get(request);
    return reply;
}

QNetworkReply *InternalNetworkAccessManager::get(QNetworkRequest &request, const QNetworkReply *oldReply)
{
    return get(request, oldReply == nullptr ? QUrl() : oldReply->url());
}

QString InternalNetworkAccessManager::userAgent()
{
    /// Various browser strings to "disguise" origin
    static const QStringList userAgentList = QStringList()
            << QStringLiteral("Mozilla/5.0 (Linux; U; Android 2.3.3; en-us; HTC_DesireS_S510e Build/GRI40) AppleWebKit/533.1 (KHTML, like Gecko) Version/4.0 Mobile Safari/533.1")
            << QStringLiteral("Mozilla/5.0 (Macintosh; U; Intel Mac OS X 10.6; en-US; rv:1.9.2.3) Gecko/20100402 Prism/1.0b4")
            << QStringLiteral("Mozilla/5.0 (Windows; U; Win 9x 4.90; SG; rv:1.9.2.4) Gecko/20101104 Netscape/9.1.0285")
            << QStringLiteral("Mozilla/5.0 (compatible; Konqueror/4.5; FreeBSD) KHTML/4.5.4 (like Gecko)")
            << QStringLiteral("Mozilla/5.0 (compatible; Yahoo! Slurp China; http://misc.yahoo.com.cn/help.html)")
            << QStringLiteral("yacybot (x86 Windows XP 5.1; java 1.6.0_12; Europe/de) http://yacy.net/bot.html")
            << QStringLiteral("Nokia6230i/2.0 (03.25) Profile/MIDP-2.0 Configuration/CLDC-1.1")
            << QStringLiteral("Links (2.3-pre1; NetBSD 5.0 i386; 96x36)")
            << QStringLiteral("Mozilla/5.0 (Windows; U; Windows NT 5.1; en-US) AppleWebKit/523.15 (KHTML, like Gecko, Safari/419.3) Arora/0.3 (Change: 287 c9dfb30)")
            << QStringLiteral("Mozilla/4.0 (compatible; Dillo 2.2)")
            << QStringLiteral("Mozilla/5.0 (Windows; U; Windows NT 6.1; en-US) AppleWebKit/532.0 (KHTML, like Gecko) Chrome/4.0.201.1 Safari/532.0")
            << QStringLiteral("Mozilla/5.0 (X11; Linux i686) AppleWebKit/535.1 (KHTML, like Gecko) Ubuntu/10.04 Chromium/14.0.813.0 Chrome/14.0.813.0 Safari/535.1")
            << QStringLiteral("Mozilla/5.0 (X11; U; Linux; de-DE) AppleWebKit/527+ (KHTML, like Gecko, Safari/419.3) Arora/0.8.0")
            << QStringLiteral("Mozilla/5.0 (X11; U; Linux i686; pt-PT; rv:1.9.2.3) Gecko/20100402 Iceweasel/3.6.3 (like Firefox/3.6.3) GTB7.0")
            << QStringLiteral("Mozilla/5.0 (X11; U; Linux i686; en-US; rv:1.0.1) Gecko/20020919")
            << QStringLiteral("Lynx/2.8.8dev.3 libwww-FM/2.14 SSL-MM/1.4.1")
            << QStringLiteral("Opera/9.80 (X11; Linux i686; U; ru) Presto/2.8.131 Version/11.11")
            << QStringLiteral("Mozilla/4.0 (compatible; MSIE 7.0; Windows NT 5.1; .NET CLR 1.1.4322; InfoPath.1; .NET CLR 2.0.50727) Sleipnir/2.8.4")
            << QStringLiteral("Mozilla/5.0 (X11; Linux i686; rv:2.2a1pre) Gecko/20110327 SeaMonkey/2.2a1pre")
            << QStringLiteral("Mozilla/5.0 (Macintosh; Intel Mac OS X 10_7_3) AppleWebKit/534.55.3 (KHTML, like Gecko) Version/5.1.3 Safari/534.53.10")
            << QStringLiteral("Mozilla/6.0 (X11; U; Linux x86_64; en-US; rv:2.9.0.3) Gecko/2009022510 FreeBSD/ Sunrise/4.0.1/like Safari")
            << QStringLiteral("Mozilla/5.0 (Linux; U; Tizen/1.0 like Android; en-us; AppleWebKit/534.46 (KHTML, like Gecko) Tizen Browser/1.0 Mobile")
            << QStringLiteral("Emacs-W3/4.0pre.46 URL/p4.0pre.46 (i686-pc-linux; X11)")
            << QStringLiteral("Mozilla/5.0 (X11; U; Linux i686; en-US; rv:1.8.1.13) Gecko/20080208 Galeon/2.0.4 (2008.1) Firefox/2.0.0.13")
            << QStringLiteral("Lynx/2.8 (compatible; iCab 2.9.8; Macintosh; U; 68K)")
            << QStringLiteral("Mozilla/5.0 (Macintosh; U; Intel Mac OS X; en; rv:1.8.1.14) Gecko/20080409 Camino/1.6 (like Firefox/2.0.0.14)")
            << QStringLiteral("msnbot/2.1")
            << QStringLiteral("Mozilla/5.0 (iPad; U; CPU OS 3_2 like Mac OS X; en-us) AppleWebKit/531.21.10 (KHTML, like Gecko) Version/4.0.4 Mobile/7B334b Safari/531.21.10")
            << QStringLiteral("Mozilla/5.0 (Windows; U; ; en-NZ) AppleWebKit/527+ (KHTML, like Gecko, Safari/419.3) Arora/0.8.0")
            << QStringLiteral("NCSA Mosaic/3.0 (Windows 95)")
            << QStringLiteral("Mozilla/5.0 (SymbianOS/9.1; U; [en]; Series60/3.0 NokiaE60/4.06.0) AppleWebKit/413 (KHTML, like Gecko) Safari/413")
            << QStringLiteral("Mozilla/5.0 (Windows; U; Windows NT 6.0; en-US) AppleWebKit/534.16 (KHTML, like Gecko) Chrome/10.0.648.133 Safari/534.16");

    if (userAgentString.isEmpty()) {
        qsrand(time(nullptr));
        userAgentString = userAgentList[qrand() % userAgentList.length()];
    }
    return userAgentString;
}

void InternalNetworkAccessManager::setNetworkReplyTimeout(QNetworkReply *reply, int timeOutSec)
{
    QTimer *timer = new QTimer(reply);
    connect(timer, &QTimer::timeout, this, &InternalNetworkAccessManager::networkReplyTimeout);
    m_mapTimerToReply.insert(timer, reply);
    timer->start(timeOutSec * 1000);
    connect(reply, &QNetworkReply::finished, this, &InternalNetworkAccessManager::networkReplyFinished);
}

void InternalNetworkAccessManager::networkReplyTimeout()
{
    QTimer *timer = static_cast<QTimer *>(sender());
    timer->stop();
    QNetworkReply *reply = m_mapTimerToReply[timer];
    if (reply != nullptr) {
        qCWarning(LOG_KBIBTEX_NETWORKING) << "Timeout on reply to " << reply->url().toDisplayString();
        reply->close();
        m_mapTimerToReply.remove(timer);
    }
}
void InternalNetworkAccessManager::networkReplyFinished()
{
    QNetworkReply *reply = static_cast<QNetworkReply *>(sender());
    QTimer *timer = m_mapTimerToReply.key(reply, nullptr);
    if (timer != nullptr) {
        disconnect(timer, &QTimer::timeout, this, &InternalNetworkAccessManager::networkReplyTimeout);
        timer->stop();
        m_mapTimerToReply.remove(timer);
    }
}

#include "internalnetworkaccessmanager.moc"
