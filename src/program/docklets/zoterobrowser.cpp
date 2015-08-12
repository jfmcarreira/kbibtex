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

#include <KLocalizedString>
#include <KComboBox>
#include <KLineEdit>
#include <KConfigGroup>
#include <KSharedConfig>

#include "element.h"
#include "searchresults.h"
#include "zotero/collectionmodel.h"
#include "zotero/collection.h"
#include "zotero/items.h"
#include "zotero/groups.h"
#include "zotero/tags.h"
#include "zotero/tagmodel.h"
#include "zotero/api.h"
#ifdef HAVE_QTOAUTH // krazy:exclude=cpp
#include "zotero/oauthwizard.h"
#endif // HAVE_QTOAUTH

class ZoteroBrowser::Private
{
private:
    ZoteroBrowser *p;

    KSharedConfigPtr config;
    static const QString configGroupName;
    static const QString configEntryNumericUserIds, configEntryApiKeys;

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
    KComboBox *comboBoxNumericUserId;
    KComboBox *comboBoxApiKey;
    QRadioButton *radioPersonalLibrary;
    QRadioButton *radioGroupLibrary;
    bool comboBoxGroupListInitialized;
    KComboBox *comboBoxGroupList;
    QPushButton *buttonLoadBibliography;

    QCursor nonBusyCursor;

    Private(SearchResults *sr, ZoteroBrowser *parent)
        : p(parent), config(KSharedConfig::openConfig(QLatin1String("kbibtexrc"))), items(NULL), groups(NULL), tags(NULL), tagModel(NULL), collection(NULL), collectionModel(NULL), api(NULL), searchResults(sr), comboBoxGroupListInitialized(false), nonBusyCursor(p->cursor()) {
        setupGUI();
    }


    void setupGUI()
    {
        QBoxLayout *layout = new QVBoxLayout(p);
        tabWidget = new QTabWidget(p);
        layout->addWidget(tabWidget);

        /// Credentials
        QWidget *container = new QWidget(tabWidget);
        tabWidget->addTab(container, QIcon::fromTheme("preferences-web-browser-identification"), i18n("Credentials"));
        QBoxLayout *containerLayout = new QVBoxLayout(container);
        QFormLayout *containerForm = new QFormLayout();
        containerLayout->addLayout(containerForm, 1);
        containerForm->setMargin(0);

        comboBoxNumericUserId = new KComboBox(container);
        comboBoxNumericUserId->setEditable(true);
        QSizePolicy sizePolicy = comboBoxNumericUserId->sizePolicy();
        sizePolicy.setHorizontalPolicy(QSizePolicy::MinimumExpanding);
        comboBoxNumericUserId->setSizePolicy(sizePolicy);
        qobject_cast<KLineEdit *>(comboBoxNumericUserId->lineEdit())->setClearButtonShown(true);
        containerForm->addRow(i18n("Numeric user id:"), comboBoxNumericUserId);
        connect(comboBoxNumericUserId->lineEdit(), SIGNAL(textChanged(QString)), p, SLOT(invalidateGroupList()));
        connect(comboBoxNumericUserId->lineEdit(), SIGNAL(textChanged(QString)), p, SLOT(updateButtons()));

        comboBoxApiKey = new KComboBox(container);
        comboBoxApiKey->setEditable(true);
        comboBoxApiKey->setSizePolicy(sizePolicy);
        qobject_cast<KLineEdit *>(comboBoxApiKey->lineEdit())->setClearButtonShown(true);
        containerForm->addRow(i18n("API key:"), comboBoxApiKey);
        connect(comboBoxApiKey->lineEdit(), SIGNAL(textChanged(QString)), p, SLOT(invalidateGroupList()));
        connect(comboBoxApiKey->lineEdit(), SIGNAL(textChanged(QString)), p, SLOT(updateButtons()));

        QWidget *libraryContainer = new QWidget(container);
        QGridLayout *libraryContainerLayout = new QGridLayout(libraryContainer);
        libraryContainerLayout->setMargin(0);
        libraryContainerLayout->setColumnMinimumWidth(0, 16); // TODO determine size of a radio button
        containerForm->addRow(QString(), libraryContainer);
        radioPersonalLibrary = new QRadioButton(i18n("Personal library"), libraryContainer);
        libraryContainerLayout->addWidget(radioPersonalLibrary, 0, 0, 1, 2);
        radioGroupLibrary = new QRadioButton(i18n("Group library"), libraryContainer);
        libraryContainerLayout->addWidget(radioGroupLibrary, 1, 0, 1, 2);
        comboBoxGroupList = new KComboBox(false, libraryContainer);
        libraryContainerLayout->addWidget(comboBoxGroupList, 2, 1, 1, 1);
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

#ifdef HAVE_QTOAUTH // krazy:exclude=cpp
        containerButtonLayout = new QHBoxLayout();
        containerLayout->addLayout(containerButtonLayout, 0);
        containerButtonLayout->setMargin(0);
        QPushButton *buttonGetOAuthCredentials = new QPushButton(QIcon::fromTheme("preferences-web-browser-identification"), i18n("Get Credentials"), container);
        containerButtonLayout->addWidget(buttonGetOAuthCredentials, 0);
        connect(buttonGetOAuthCredentials, SIGNAL(clicked()), p, SLOT(getOAuthCredentials()));
        containerButtonLayout->addStretch(1);
#endif // HAVE_QTOAUTH

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

    void loadState() {
        KConfigGroup configGroup(config, configGroupName);

        const QStringList numericUserIds = configGroup.readEntry(configEntryNumericUserIds, QStringList());
        comboBoxNumericUserId->clear();
        comboBoxNumericUserId->addItems(numericUserIds);
        if (!numericUserIds.isEmpty()) comboBoxNumericUserId->setCurrentIndex(0);

        const QStringList apiKeys = configGroup.readEntry(configEntryApiKeys, QStringList());
        comboBoxApiKey->clear();
        comboBoxApiKey->addItems(apiKeys);
        if (!apiKeys.isEmpty()) comboBoxApiKey->setCurrentIndex(0);

        p->updateButtons();
    }

    void saveState() {
        KConfigGroup configGroup(config, configGroupName);

        QStringList buffer;
        for (int i = 0; i < comboBoxNumericUserId->count(); ++i)
            buffer.append(comboBoxNumericUserId->itemText(i));
        configGroup.writeEntry(configEntryNumericUserIds, buffer);

        buffer.clear();
        for (int i = 0; i < comboBoxApiKey->count(); ++i)
            buffer.append(comboBoxApiKey->itemText(i));
        configGroup.writeEntry(configEntryApiKeys, buffer);

        config->sync();
    }

    void addTextToLists() {
        QString buffer = comboBoxNumericUserId->currentText();
        if (comboBoxNumericUserId->itemText(0) != buffer) {
            for (int i = 0; i < comboBoxNumericUserId->count(); ++i)
                if (comboBoxNumericUserId->itemText(i) == buffer) {
                    comboBoxNumericUserId->removeItem(i);
                    break;
                }
            comboBoxNumericUserId->insertItem(0, buffer);
        }

        buffer = comboBoxApiKey->currentText();
        if (comboBoxApiKey->itemText(0) != buffer) {
            for (int i = 0; i < comboBoxApiKey->count(); ++i)
                if (comboBoxApiKey->itemText(i) == buffer) {
                    comboBoxApiKey->removeItem(i);
                    break;
                }
            comboBoxApiKey->insertItem(0, buffer);
        }
    }
};

const QString ZoteroBrowser::Private::configGroupName = QLatin1String("ZoteroBrowser");
const QString ZoteroBrowser::Private::configEntryNumericUserIds = QLatin1String("NumericUserIds");
const QString ZoteroBrowser::Private::configEntryApiKeys = QLatin1String("ApiKeys");

ZoteroBrowser::ZoteroBrowser(SearchResults *searchResults, QWidget *parent)
    : QWidget(parent), d(new ZoteroBrowser::Private(searchResults, this))
{
    d->loadState();
    /// Forece GUI update
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
    d->buttonLoadBibliography->setEnabled(!d->comboBoxNumericUserId->currentText().isEmpty() && !d->comboBoxApiKey->currentText().isEmpty());
}

void ZoteroBrowser::applyCredentials()
{
    bool ok = false;
    const int userId = d->comboBoxNumericUserId->currentText().toInt(&ok);
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

        d->addTextToLists();
        d->saveState();

        const bool makeGroupRequest = d->radioGroupLibrary->isChecked() && groupId > 0;
        d->api = new Zotero::API(makeGroupRequest ? Zotero::API::GroupRequest : Zotero::API::UserRequest, makeGroupRequest ? groupId : userId, d->comboBoxApiKey->currentText(), this);
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
        QMessageBox::information(this, i18n("Invalid numeric identifier"), i18n("Value '%1' is not a valid numeric identifier of a user or a group.", d->comboBoxNumericUserId->currentText()));
}

void ZoteroBrowser::radioButtonsToggled() {
    d->comboBoxGroupList->setEnabled(d->comboBoxGroupListInitialized && d->comboBoxGroupList->count() > 0 && d->radioGroupLibrary->isChecked());
    if (!d->comboBoxGroupListInitialized && d->radioGroupLibrary->isChecked())
        retrieveGroupList();
}

void ZoteroBrowser::retrieveGroupList() {
    bool ok = false;
    const int userId = d->comboBoxNumericUserId->currentText().toInt(&ok);
    if (ok) {
        setCursor(Qt::WaitCursor);
        setEnabled(false);
        d->comboBoxGroupList->clear();
        d->comboBoxGroupListInitialized = false;

        d->api->deleteLater();
        d->groups->deleteLater();

        d->api = new Zotero::API(Zotero::API::UserRequest, userId, d->comboBoxApiKey->currentText(), this);
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
        d->radioPersonalLibrary->setChecked(true);
        invalidateGroupList();
    } else {
        d->comboBoxGroupListInitialized = true;
        d->radioGroupLibrary->setChecked(true);
        d->comboBoxGroupList->setEnabled(true);
    }

    reenableWidget();
}

#ifdef HAVE_QTOAUTH // krazy:exclude=cpp
void ZoteroBrowser::getOAuthCredentials()
{
    QPointer<Zotero::OAuthWizard> wizard = new Zotero::OAuthWizard(this);
    if (wizard->exec() && !wizard->apiKey().isEmpty() && wizard->userId() >= 0) {
        d->comboBoxApiKey->setEditText(wizard->apiKey());
        d->comboBoxNumericUserId->setEditText(QString::number(wizard->userId()));
        d->addTextToLists();
    }
    delete wizard;
}
#endif // HAVE_QTOAUTH
