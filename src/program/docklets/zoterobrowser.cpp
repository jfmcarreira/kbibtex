/***************************************************************************
 *   Copyright (C) 2004-2015 by Thomas Fischer <fischer@unix-ag.uni-kl.de> *
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
 *   along with this program; if not, see <http://www.gnu.org/licenses/>.  *
 ***************************************************************************/

#include "zoterobrowser.h"

#include <QTreeView>
#include <QTabWidget>
#include <QListView>
#include <QMessageBox>
#include <QLayout>
#include <QFormLayout>
#include <QAbstractItemModel>
#include <QRadioButton>
#include <QPushButton>
#include <QDebug>
#include <QLineEdit>

#include <KLocalizedString>
#include <KComboBox>
#include <KConfigGroup>
#include <KSharedConfig>
#include <KWallet/KWallet>

#include "element.h"
#include "searchresults.h"
#include "zotero/collectionmodel.h"
#include "zotero/collection.h"
#include "zotero/items.h"
#include "zotero/groups.h"
#include "zotero/tags.h"
#include "zotero/tagmodel.h"
#include "zotero/api.h"
#include "zotero/oauthwizard.h"

using KWallet::Wallet;

class ZoteroBrowser::Private
{
private:
    ZoteroBrowser *p;

    KSharedConfigPtr config;
    static const QString configGroupName;
    static const QString configEntryNumericUserId, configEntryApiKey;

public:
    Zotero::Items *items;
    Zotero::Groups *groups;
    Zotero::Tags *tags;
    Zotero::TagModel *tagModel;
    Zotero::Collection *collection;
    Zotero::CollectionModel *collectionModel;
    Zotero::API *api;

    SearchResults *searchResults;

    QTabWidget *tabWidget;
    QTreeView *collectionBrowser;
    QListView *tagBrowser;
    QLineEdit *lineEditNumericUserId;
    QLineEdit *lineEditApiKey;
    QRadioButton *radioPersonalLibrary;
    QRadioButton *radioGroupLibrary;
    bool comboBoxGroupListInitialized;
    KComboBox *comboBoxGroupList;
    QPushButton *buttonLoadBibliography;

    QCursor nonBusyCursor;

    Wallet *wallet;
    static const QString walletFolderOAuth, walletEntryKBibTeXZotero, walletKeyZoteroId, walletKeyZoteroApiKey;

    Private(SearchResults *sr, ZoteroBrowser *parent)
            : p(parent), config(KSharedConfig::openConfig(QLatin1String("kbibtexrc"))), items(NULL), groups(NULL), tags(NULL), tagModel(NULL), collection(NULL), collectionModel(NULL), api(NULL), searchResults(sr), comboBoxGroupListInitialized(false), nonBusyCursor(p->cursor()) {
        setupGUI();

        wallet = Wallet::openWallet(Wallet::NetworkWallet(), parent->winId(), Wallet::Asynchronous);
        connect(wallet, &Wallet::walletOpened, parent, &ZoteroBrowser::readOAuthCredentials);
    }

    ~Private() {
        delete wallet;
    }

    void setupGUI()
    {
        QBoxLayout *layout = new QVBoxLayout(p);
        tabWidget = new QTabWidget(p);
        layout->addWidget(tabWidget);

        QWidget *container = new QWidget(tabWidget);
        tabWidget->addTab(container, QIcon::fromTheme("preferences-web-browser-identification"), i18n("Library"));
        QBoxLayout *containerLayout = new QVBoxLayout(container);

        /// Personal or Group Library
        QGridLayout *gridLayout = new QGridLayout();
        containerLayout->addLayout(gridLayout);
        gridLayout->setMargin(0);
        gridLayout->setColumnMinimumWidth(0, 16); // TODO determine size of a radio button
        radioPersonalLibrary = new QRadioButton(i18n("Personal library"), container);
        gridLayout->addWidget(radioPersonalLibrary, 0, 0, 1, 2);
        radioGroupLibrary = new QRadioButton(i18n("Group library"), container);
        gridLayout->addWidget(radioGroupLibrary, 1, 0, 1, 2);
        comboBoxGroupList = new KComboBox(false, container);
        gridLayout->addWidget(comboBoxGroupList, 2, 1, 1, 1);
        QSizePolicy sizePolicy = comboBoxGroupList->sizePolicy();
        sizePolicy.setHorizontalPolicy(QSizePolicy::MinimumExpanding);
        comboBoxGroupList->setSizePolicy(sizePolicy);
        radioPersonalLibrary->setChecked(true);
        comboBoxGroupList->setEnabled(false);
        comboBoxGroupList->addItem(i18n("No groups available"));
        connect(radioGroupLibrary, SIGNAL(toggled(bool)), p, SLOT(radioButtonsToggled()));
        connect(radioPersonalLibrary, SIGNAL(toggled(bool)), p, SLOT(radioButtonsToggled()));

        QBoxLayout *containerButtonLayout = new QHBoxLayout();
        containerLayout->addLayout(containerButtonLayout, 0);
        containerButtonLayout->setMargin(0);
        containerButtonLayout->addStretch(1);
        buttonLoadBibliography = new QPushButton(QIcon::fromTheme("download"), i18n("Load bibliography"), container);
        containerButtonLayout->addWidget(buttonLoadBibliography, 0);
        connect(buttonLoadBibliography, SIGNAL(clicked()), p, SLOT(applyCredentials()));

        containerLayout->addStretch(10);

        /// Credentials
        QFormLayout *containerForm = new QFormLayout();
        containerLayout->addLayout(containerForm, 1);
        containerForm->setMargin(0);
        lineEditNumericUserId = new QLineEdit(container);
        lineEditNumericUserId->setSizePolicy(sizePolicy);
        lineEditNumericUserId->setReadOnly(true);
        containerForm->addRow(i18n("Numeric user id:"), lineEditNumericUserId);
        connect(lineEditNumericUserId, &QLineEdit::textChanged, p, &ZoteroBrowser::invalidateGroupList);

        lineEditApiKey = new QLineEdit(container);
        lineEditApiKey->setSizePolicy(sizePolicy);
        lineEditApiKey->setReadOnly(true);
        containerForm->addRow(i18n("API key:"), lineEditApiKey);
        connect(lineEditApiKey, &QLineEdit::textChanged, p, &ZoteroBrowser::invalidateGroupList);

        containerButtonLayout = new QHBoxLayout();
        containerLayout->addLayout(containerButtonLayout, 0);
        containerButtonLayout->setMargin(0);
        QPushButton *buttonGetOAuthCredentials = new QPushButton(QIcon::fromTheme("preferences-web-browser-identification"), i18n("Get Credentials"), container);
        containerButtonLayout->addWidget(buttonGetOAuthCredentials, 0);
        connect(buttonGetOAuthCredentials, &QPushButton::clicked, p, &ZoteroBrowser::getOAuthCredentials);
        containerButtonLayout->addStretch(1);

        /// Collection browser
        collectionBrowser = new QTreeView(tabWidget);
        tabWidget->addTab(collectionBrowser, QIcon::fromTheme("folder-yellow"), i18n("Collections"));
        collectionBrowser->setHeaderHidden(true);
        collectionBrowser->setExpandsOnDoubleClick(false);
        connect(collectionBrowser, SIGNAL(doubleClicked(QModelIndex)), p, SLOT(collectionDoubleClicked(QModelIndex)));

        /// Tag browser
        tagBrowser = new QListView(tabWidget);
        tabWidget->addTab(tagBrowser, QIcon::fromTheme("mail-tagged"), i18n("Tags"));
        connect(tagBrowser, SIGNAL(doubleClicked(QModelIndex)), p, SLOT(tagDoubleClicked(QModelIndex)));
    }

    void queueWriteOAuthCredentials() {
        if (wallet->isOpen())
            p->writeOAuthCredentials(true);
        else {
            wallet = Wallet::openWallet(Wallet::NetworkWallet(), p->winId(), Wallet::Asynchronous);
            connect(wallet, &Wallet::walletOpened, p, &ZoteroBrowser::writeOAuthCredentials);
        }
    }
};

const QString ZoteroBrowser::Private::configGroupName = QLatin1String("ZoteroBrowser");
const QString ZoteroBrowser::Private::walletFolderOAuth = QLatin1String("OAuth");
const QString ZoteroBrowser::Private::walletEntryKBibTeXZotero = QLatin1String("KBibTeX/Zotero");
const QString ZoteroBrowser::Private::walletKeyZoteroId = QLatin1String("UserId");
const QString ZoteroBrowser::Private::walletKeyZoteroApiKey = QLatin1String("ApiKey");

ZoteroBrowser::ZoteroBrowser(SearchResults *searchResults, QWidget *parent)
        : QWidget(parent), d(new ZoteroBrowser::Private(searchResults, this))
{
    /// Forece GUI update
    updateButtons();
    radioButtonsToggled();
}

ZoteroBrowser::~ZoteroBrowser()
{
    delete d;
}

void ZoteroBrowser::modelReset()
{
    if (!d->collection->busy() && !d->tags->busy()) {
        setCursor(d->nonBusyCursor);
        setEnabled(true);
    } else {
        setCursor(Qt::WaitCursor);
        setEnabled(false);
    }

    if (!d->tags->busy() && !d->collection->busy() && !(d->collection->initialized() && d->tags->initialized()))
        QMessageBox::information(this, i18n("Failed to retrieve data from Zotero"), i18n("KBibTeX failed to retrieve the bibliography from Zotero. Please check that the provided user id and API key are valid."));
}

void ZoteroBrowser::collectionDoubleClicked(const QModelIndex &index)
{
    setCursor(Qt::WaitCursor);
    setEnabled(false); ///< will be re-enabled when item retrieve got finished (slot reenableWidget)

    const QString collectionId = index.data(Zotero::CollectionModel::CollectionIdRole).toString();
    d->searchResults->clear();
    d->items->retrieveItemsByCollection(collectionId);
}

void ZoteroBrowser::tagDoubleClicked(const QModelIndex &index)
{
    setCursor(Qt::WaitCursor);
    setEnabled(false); ///< will be re-enabled when item retrieve got finished (slot reenableWidget)

    const QString tag = index.data(Zotero::TagModel::TagRole).toString();
    d->searchResults->clear();
    d->items->retrieveItemsByTag(tag);
}

void ZoteroBrowser::showItem(QSharedPointer<Element> e)
{
    d->searchResults->insertElement(e);
}

void ZoteroBrowser::reenableWidget()
{
    setCursor(d->nonBusyCursor);
    setEnabled(true);
}

void ZoteroBrowser::updateButtons()
{
    const bool validNumericIdAndApiKey = !d->lineEditNumericUserId->text().isEmpty() && !d->lineEditApiKey->text().isEmpty();
    d->buttonLoadBibliography->setEnabled(validNumericIdAndApiKey);
    d->radioGroupLibrary->setEnabled(validNumericIdAndApiKey);
    d->radioPersonalLibrary->setEnabled(validNumericIdAndApiKey);
}

void ZoteroBrowser::applyCredentials()
{
    bool ok = false;
    const int userId = d->lineEditNumericUserId->text().toInt(&ok);
    if (ok) {
        setCursor(Qt::WaitCursor);
        setEnabled(false);

        ok = false;
        int groupId = d->comboBoxGroupList->itemData(d->comboBoxGroupList->currentIndex()).toInt(&ok);
        if (!ok) groupId = -1;

        d->api->deleteLater();
        d->collection->deleteLater();
        d->items->deleteLater();
        d->tags->deleteLater();
        d->collectionModel->deleteLater();
        d->tagModel->deleteLater();

        const bool makeGroupRequest = d->radioGroupLibrary->isChecked() && groupId > 0;
        d->api = new Zotero::API(makeGroupRequest ? Zotero::API::GroupRequest : Zotero::API::UserRequest, makeGroupRequest ? groupId : userId, d->lineEditApiKey->text(), this);
        d->items = new Zotero::Items(d->api, this);
        d->tags = new Zotero::Tags(d->api, this);
        d->tagModel = new Zotero::TagModel(d->tags, this);
        d->tagBrowser->setModel(d->tagModel);
        d->collection = new Zotero::Collection(d->api, this);
        d->collectionModel = new Zotero::CollectionModel(d->collection, this);
        d->collectionBrowser->setModel(d->collectionModel);

        connect(d->collectionModel, SIGNAL(modelReset()), this, SLOT(modelReset()));
        connect(d->tagModel, SIGNAL(modelReset()), this, SLOT(modelReset()));
        connect(d->items, SIGNAL(foundElement(QSharedPointer<Element>)), this, SLOT(showItem(QSharedPointer<Element>)));
        connect(d->items, SIGNAL(stoppedSearch(int)), this, SLOT(reenableWidget()));
        connect(d->tags, SIGNAL(finishedLoading()), this, SLOT(reenableWidget()));

        d->tabWidget->setCurrentIndex(1);
    } else
        QMessageBox::information(this, i18n("Invalid numeric identifier"), i18n("Value '%1' is not a valid numeric identifier of a user or a group.", d->lineEditNumericUserId->text()));
}

void ZoteroBrowser::radioButtonsToggled() {
    d->comboBoxGroupList->setEnabled(d->comboBoxGroupListInitialized && d->comboBoxGroupList->count() > 0 && d->radioGroupLibrary->isChecked());
    if (!d->comboBoxGroupListInitialized && d->radioGroupLibrary->isChecked())
        retrieveGroupList();
}

void ZoteroBrowser::retrieveGroupList() {
    bool ok = false;
    const int userId = d->lineEditNumericUserId->text().toInt(&ok);
    if (ok) {
        setCursor(Qt::WaitCursor);
        setEnabled(false);
        d->comboBoxGroupList->clear();
        d->comboBoxGroupListInitialized = false;

        d->api->deleteLater();
        d->groups->deleteLater();

        d->api = new Zotero::API(Zotero::API::UserRequest, userId, d->lineEditApiKey->text(), this);
        d->groups = new Zotero::Groups(d->api, this);

        connect(d->groups, SIGNAL(finishedLoading()), this, SLOT(gotGroupList()));
    }
}

void ZoteroBrowser::invalidateGroupList() {
    d->comboBoxGroupList->clear();
    d->comboBoxGroupListInitialized = false;
    d->comboBoxGroupList->addItem(i18n("No groups available or no permissions"));
    d->comboBoxGroupList->setEnabled(false);
    d->radioPersonalLibrary->setChecked(true);
}

void ZoteroBrowser::gotGroupList() {
    const QMap<int, QString> groupMap = d->groups->groups();
    for (QMap<int, QString>::ConstIterator it = groupMap.constBegin(); it != groupMap.constEnd(); ++it) {
        d->comboBoxGroupList->addItem(it.value(), QVariant::fromValue<int>(it.key()));
    }
    if (groupMap.isEmpty()) {
        invalidateGroupList();
    } else {
        d->comboBoxGroupListInitialized = true;
        d->radioGroupLibrary->setChecked(true);
        d->comboBoxGroupList->setEnabled(true);
    }

    reenableWidget();
}

void ZoteroBrowser::getOAuthCredentials()
{
    QPointer<Zotero::OAuthWizard> wizard = new Zotero::OAuthWizard(this);
    if (wizard->exec() && !wizard->apiKey().isEmpty() && wizard->userId() >= 0) {
        d->lineEditApiKey->setText(wizard->apiKey());
        d->lineEditNumericUserId->setText(QString::number(wizard->userId()));
        d->queueWriteOAuthCredentials();
        updateButtons();
        retrieveGroupList();
    }
    delete wizard;
}

void ZoteroBrowser::readOAuthCredentials(bool ok) {
    /// Do not call this slot a second time
    disconnect(d->wallet, &Wallet::walletOpened, this, &ZoteroBrowser::readOAuthCredentials);

    if (ok && (d->wallet->hasFolder(ZoteroBrowser::Private::walletFolderOAuth) || d->wallet->createFolder(ZoteroBrowser::Private::walletFolderOAuth)) && d->wallet->setFolder(ZoteroBrowser::Private::walletFolderOAuth)) {
        if (d->wallet->hasEntry(ZoteroBrowser::Private::walletEntryKBibTeXZotero)) {
            QMap<QString, QString> map;
            if (d->wallet->readMap(ZoteroBrowser::Private::walletEntryKBibTeXZotero, map) == 0) {
                if (map.contains(ZoteroBrowser::Private::walletKeyZoteroId) && map.contains(ZoteroBrowser::Private::walletKeyZoteroApiKey)) {
                    d->lineEditNumericUserId->setText(map.value(ZoteroBrowser::Private::walletKeyZoteroId, QString()));
                    d->lineEditApiKey->setText(map.value(ZoteroBrowser::Private::walletKeyZoteroApiKey, QString()));
                    updateButtons();
                    retrieveGroupList();
                }
            }
        }
    } else
        qWarning() << "Accessing KWallet to sync API key did not succeed";
}

void ZoteroBrowser::writeOAuthCredentials(bool ok) {
    disconnect(d->wallet, &Wallet::walletOpened, this, &ZoteroBrowser::writeOAuthCredentials);

    if (ok && (d->wallet->hasFolder(ZoteroBrowser::Private::walletFolderOAuth) || d->wallet->createFolder(ZoteroBrowser::Private::walletFolderOAuth)) && d->wallet->setFolder(ZoteroBrowser::Private::walletFolderOAuth)) {
        QMap<QString, QString> map;
        map.insert(ZoteroBrowser::Private::walletKeyZoteroId, d->lineEditNumericUserId->text());
        map.insert(ZoteroBrowser::Private::walletKeyZoteroApiKey, d->lineEditApiKey->text());
        if (d->wallet->writeMap(ZoteroBrowser::Private::walletEntryKBibTeXZotero, map) != 0)
            qWarning() << "Writing API key to KWallet failed";
    } else
        qWarning() << "Accessing KWallet to sync API key did not succeed";
}
