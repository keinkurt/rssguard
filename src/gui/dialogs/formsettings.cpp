// This file is part of RSS Guard.
//
// Copyright (C) 2011-2016 by Martin Rotter <rotter.martinos@gmail.com>
//
// RSS Guard is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// RSS Guard is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with RSS Guard. If not, see <http://www.gnu.org/licenses/>.

#include "gui/dialogs/formsettings.h"

#include "definitions/definitions.h"
#include "core/feeddownloader.h"
#include "core/feedsmodel.h"
#include "core/messagesmodel.h"
#include "miscellaneous/application.h"
#include "miscellaneous/settings.h"
#include "miscellaneous/databasefactory.h"
#include "miscellaneous/localization.h"
#include "miscellaneous/systemfactory.h"
#include "miscellaneous/iconfactory.h"
#include "miscellaneous/skinfactory.h"
#include "miscellaneous/textfactory.h"
#include "network-web/webfactory.h"
#include "network-web/silentnetworkaccessmanager.h"
#include "gui/systemtrayicon.h"
#include "gui/feedmessageviewer.h"
#include "gui/feedsview.h"
#include "gui/feedstoolbar.h"
#include "gui/messagebox.h"
#include "gui/basetoolbar.h"
#include "gui/messagestoolbar.h"
#include "gui/messagesview.h"
#include "gui/statusbar.h"
#include "gui/dialogs/formmain.h"
#include "dynamic-shortcuts/dynamicshortcuts.h"

#include <QProcess>
#include <QNetworkProxy>
#include <QColorDialog>
#include <QFileDialog>
#include <QKeyEvent>
#include <QFontDialog>
#include <QDir>


FormSettings::FormSettings(QWidget *parent) : QDialog(parent), m_ui(new Ui::FormSettings), m_settings(nullptr) {
  m_ui->setupUi(this);

  m_settings = qApp->settings();

  // Set flags and attributes.
  setWindowFlags(Qt::MSWindowsFixedSizeDialogHint | Qt::Dialog | Qt::WindowSystemMenuHint | Qt::WindowTitleHint);
  setWindowIcon(qApp->icons()->fromTheme(QSL("emblem-system")));

  m_ui->m_editorMessagesToolbar->activeItemsWidget()->viewport()->installEventFilter(this);
  m_ui->m_editorFeedsToolbar->activeItemsWidget()->viewport()->installEventFilter(this);
  m_ui->m_editorMessagesToolbar->availableItemsWidget()->viewport()->installEventFilter(this);
  m_ui->m_editorFeedsToolbar->availableItemsWidget()->viewport()->installEventFilter(this);

  // Setup behavior.
  m_ui->m_listSettings->setCurrentRow(0);
  m_ui->m_treeLanguages->setColumnCount(3);
  m_ui->m_treeLanguages->setHeaderHidden(false);
  m_ui->m_treeLanguages->setHeaderLabels(QStringList()
                                         << /*: Language column of language list. */ tr("Language")
                                         << /*: Lang. code column of language list. */ tr("Code")
                                         << tr("Author"));

  m_ui->m_treeSkins->setColumnCount(4);
  m_ui->m_treeSkins->setHeaderHidden(false);
  m_ui->m_treeSkins->setHeaderLabels(QStringList()
                                     << /*: Skin list name column. */ tr("Name")
                                     << /*: Version column of skin list. */ tr("Version")
                                     << tr("Author")
                                     << tr("E-mail"));

  // Setup languages.
  m_ui->m_treeLanguages->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
  m_ui->m_treeLanguages->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
  m_ui->m_treeLanguages->header()->setSectionResizeMode(2, QHeaderView::ResizeToContents);

  // Setup skins.
  m_ui->m_treeSkins->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
  m_ui->m_treeSkins->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
  m_ui->m_treeSkins->header()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
  m_ui->m_treeSkins->header()->setSectionResizeMode(3, QHeaderView::ResizeToContents);

  // Establish needed connections.
  connect(m_ui->m_buttonBox, SIGNAL(accepted()), this, SLOT(saveSettings()));

  connect(m_ui->m_cmbProxyType, SIGNAL(currentIndexChanged(int)), this, SLOT(onProxyTypeChanged(int)));
  connect(m_ui->m_checkShowPassword, SIGNAL(stateChanged(int)), this, SLOT(displayProxyPassword(int)));
  connect(m_ui->m_cmbExternalBrowserPreset, SIGNAL(currentIndexChanged(int)), this, SLOT(changeDefaultBrowserArguments(int)));
  connect(m_ui->m_btnExternalBrowserExecutable, SIGNAL(clicked()), this, SLOT(selectBrowserExecutable()));
  connect(m_ui->m_cmbExternalEmailPreset, SIGNAL(currentIndexChanged(int)), this, SLOT(changeDefaultEmailArguments(int)));
  connect(m_ui->m_btnExternalEmailExecutable, SIGNAL(clicked()), this, SLOT(selectEmailExecutable()));

  connect(m_ui->m_btnDownloadsTargetDirectory, SIGNAL(clicked()), this, SLOT(selectDownloadsDirectory()));

  connect(m_ui->m_btnChangeMessagesFont, SIGNAL(clicked()), this, SLOT(changeMessagesFont()));


  // Load all settings.
  loadGeneral();
  loadDataStorage();
  loadShortcuts();
  loadInterface();
  loadProxy();
  loadBrowser();
  loadLanguage();
  loadFeedsMessages();
  loadDownloads();
}

FormSettings::~FormSettings() {
  qDebug("Destroying FormSettings distance.");
}

void FormSettings::changeDefaultBrowserArguments(int index) {
  if (index != 0) {
    m_ui->m_txtExternalBrowserArguments->setText(m_ui->m_cmbExternalBrowserPreset->itemData(index).toString());
  }
}

void FormSettings::onSkinSelected(QTreeWidgetItem *current, QTreeWidgetItem *previous) {
  Q_UNUSED(previous)

  if (current != nullptr) {
    const Skin skin = current->data(0, Qt::UserRole).value<Skin>();
    m_ui->m_lblSelectedContents->setText(skin.m_visibleName);
  }
}

void FormSettings::loadDownloads() {
  m_ui->m_checkOpenManagerWhenDownloadStarts->setChecked(m_settings->value(GROUP(Downloads),
                                                                           SETTING(Downloads::ShowDownloadsWhenNewDownloadStarts)).toBool());
  m_ui->m_txtDownloadsTargetDirectory->setText(QDir::toNativeSeparators(m_settings->value(GROUP(Downloads),
                                                                                          SETTING(Downloads::TargetDirectory)).toString()));
  m_ui->m_rbDownloadsAskEachFile->setChecked(m_settings->value(GROUP(Downloads),
                                                               SETTING(Downloads::AlwaysPromptForFilename)).toBool());
}

void FormSettings::saveDownloads() {
  m_settings->setValue(GROUP(Downloads), Downloads::ShowDownloadsWhenNewDownloadStarts, m_ui->m_checkOpenManagerWhenDownloadStarts->isChecked());
  m_settings->setValue(GROUP(Downloads), Downloads::TargetDirectory, m_ui->m_txtDownloadsTargetDirectory->text());
  m_settings->setValue(GROUP(Downloads), Downloads::AlwaysPromptForFilename, m_ui->m_rbDownloadsAskEachFile->isChecked());
  qApp->downloadManager()->setDownloadDirectory(m_ui->m_txtDownloadsTargetDirectory->text());
}

void FormSettings::selectDownloadsDirectory() {
  const QString target_directory = QFileDialog::getExistingDirectory(this,
                                                                     tr("Select downloads target directory"),
                                                                     m_ui->m_txtDownloadsTargetDirectory->text()
                                                                     );

  if (!target_directory.isEmpty()) {
    m_ui->m_txtDownloadsTargetDirectory->setText(QDir::toNativeSeparators(target_directory));
  }
}

void FormSettings::selectBrowserExecutable() {
  const QString executable_file = QFileDialog::getOpenFileName(this,
                                                               tr("Select web browser executable"),
                                                               qApp->homeFolderPath(),
                                                               //: File filter for external browser selection dialog.
                                                             #if defined(Q_OS_LINUX)
                                                               tr("Executables (*)")
                                                             #else
                                                               tr("Executables (*.*)")
                                                             #endif
                                                               );

  if (!executable_file.isEmpty()) {
    m_ui->m_txtExternalBrowserExecutable->setText(QDir::toNativeSeparators(executable_file));
  }
}

void FormSettings::changeDefaultEmailArguments(int index) {
  if (index != 0) {
    m_ui->m_txtExternalEmailArguments->setText(m_ui->m_cmbExternalEmailPreset->itemData(index).toString());
  }
}

void FormSettings::selectEmailExecutable() {
  QString executable_file = QFileDialog::getOpenFileName(this,
                                                         tr("Select e-mail executable"),
                                                         qApp->homeFolderPath(),
                                                         //: File filter for external e-mail selection dialog.
                                                       #if defined(Q_OS_LINUX)
                                                         tr("Executables (*)")
                                                       #else
                                                         tr("Executables (*.*)")
                                                       #endif
                                                         );

  if (!executable_file.isEmpty()) {
    m_ui->m_txtExternalEmailExecutable->setText(QDir::toNativeSeparators(executable_file));
  }
}

void FormSettings::loadFeedsMessages() {
  m_ui->m_checkKeppMessagesInTheMiddle->setChecked(m_settings->value(GROUP(Messages), SETTING(Messages::KeepCursorInCenter)).toBool());
  m_ui->m_checkRemoveReadMessagesOnExit->setChecked(m_settings->value(GROUP(Messages), SETTING(Messages::ClearReadOnExit)).toBool());
  m_ui->m_checkAutoUpdate->setChecked(m_settings->value(GROUP(Feeds), SETTING(Feeds::AutoUpdateEnabled)).toBool());
  m_ui->m_spinAutoUpdateInterval->setValue(m_settings->value(GROUP(Feeds), SETTING(Feeds::AutoUpdateInterval)).toInt());
  m_ui->m_spinFeedUpdateTimeout->setValue(m_settings->value(GROUP(Feeds), SETTING(Feeds::UpdateTimeout)).toInt());
  m_ui->m_checkUpdateAllFeedsOnStartup->setChecked(m_settings->value(GROUP(Feeds), SETTING(Feeds::FeedsUpdateOnStartup)).toBool());
  m_ui->m_cmbCountsFeedList->addItems(QStringList() << "(%unread)" << "[%unread]" << "%unread/%all" << "%unread-%all" << "[%unread|%all]");
  m_ui->m_cmbCountsFeedList->setEditText(m_settings->value(GROUP(Feeds), SETTING(Feeds::CountFormat)).toString());

  initializeMessageDateFormats();

  m_ui->m_checkMessagesDateTimeFormat->setChecked(m_settings->value(GROUP(Messages), SETTING(Messages::UseCustomDate)).toBool());
  const int index_format = m_ui->m_cmbMessagesDateTimeFormat->findData(m_settings->value(GROUP(Messages), SETTING(Messages::CustomDateFormat)).toString());

  if (index_format >= 0) {
    m_ui->m_cmbMessagesDateTimeFormat->setCurrentIndex(index_format);
  }

  m_ui->m_lblMessagesFont->setText(tr("Font preview"));
  QFont fon;
  fon.fromString(m_settings->value(GROUP(Messages),
                                   SETTING(Messages::PreviewerFontStandard)).toString());
  m_ui->m_lblMessagesFont->setFont(fon);

}

void FormSettings::initializeMessageDateFormats() {
  QStringList best_formats; best_formats << QSL("d/M/yyyy hh:mm:ss") << QSL("ddd, d. M. yy hh:mm:ss") <<
                                            QSL("yyyy-MM-dd HH:mm:ss.z") << QSL("yyyy-MM-ddThh:mm:ss") <<
                                            QSL("MMM d yyyy hh:mm:ss");;
  const QLocale current_locale = qApp->localization()->loadedLocale();
  const QDateTime current_dt = QDateTime::currentDateTime();

  foreach (const QString &format, best_formats) {
    m_ui->m_cmbMessagesDateTimeFormat->addItem(current_locale.toString(current_dt, format), format);
  }
}

void FormSettings::changeMessagesFont() {
  bool ok;
  QFont new_font = QFontDialog::getFont(&ok, m_ui->m_lblMessagesFont->font(),
                                        this, tr("Select new font for message viewer"),
                                        QFontDialog::DontUseNativeDialog);

  if (ok) {
    m_ui->m_lblMessagesFont->setFont(new_font);
  }
}

void FormSettings::saveFeedsMessages() {
  m_settings->setValue(GROUP(Messages), Messages::KeepCursorInCenter, m_ui->m_checkKeppMessagesInTheMiddle->isChecked());
  m_settings->setValue(GROUP(Messages), Messages::ClearReadOnExit, m_ui->m_checkRemoveReadMessagesOnExit->isChecked());
  m_settings->setValue(GROUP(Feeds), Feeds::AutoUpdateEnabled, m_ui->m_checkAutoUpdate->isChecked());
  m_settings->setValue(GROUP(Feeds), Feeds::AutoUpdateInterval, m_ui->m_spinAutoUpdateInterval->value());
  m_settings->setValue(GROUP(Feeds), Feeds::UpdateTimeout, m_ui->m_spinFeedUpdateTimeout->value());
  m_settings->setValue(GROUP(Feeds), Feeds::FeedsUpdateOnStartup, m_ui->m_checkUpdateAllFeedsOnStartup->isChecked());
  m_settings->setValue(GROUP(Feeds), Feeds::CountFormat, m_ui->m_cmbCountsFeedList->currentText());
  m_settings->setValue(GROUP(Messages), Messages::UseCustomDate, m_ui->m_checkMessagesDateTimeFormat->isChecked());
  m_settings->setValue(GROUP(Messages), Messages::CustomDateFormat,
                       m_ui->m_cmbMessagesDateTimeFormat->itemData(m_ui->m_cmbMessagesDateTimeFormat->currentIndex()).toString());

  // Save fonts.
  m_settings->setValue(GROUP(Messages), Messages::PreviewerFontStandard, m_ui->m_lblMessagesFont->font().toString());

  qApp->mainForm()->tabWidget()->feedMessageViewer()->loadMessageViewerFonts();
  qApp->mainForm()->tabWidget()->feedMessageViewer()->feedsView()->sourceModel()->updateAutoUpdateStatus();
  qApp->mainForm()->tabWidget()->feedMessageViewer()->feedsView()->sourceModel()->reloadWholeLayout();
  qApp->mainForm()->tabWidget()->feedMessageViewer()->messagesView()->sourceModel()->updateDateFormat();
  qApp->mainForm()->tabWidget()->feedMessageViewer()->messagesView()->sourceModel()->reloadWholeLayout();
}

void FormSettings::displayProxyPassword(int state) {
  if (state == Qt::Checked) {
    m_ui->m_txtProxyPassword->setEchoMode(QLineEdit::Normal);
  }
  else {
    m_ui->m_txtProxyPassword->setEchoMode(QLineEdit::PasswordEchoOnEdit);
  }
}

bool FormSettings::doSaveCheck() {
  bool everything_ok = true;
  QStringList resulting_information;

  // Setup indication of settings consistence.
  if (!m_ui->m_shortcuts->areShortcutsUnique()) {
    everything_ok = false;
    resulting_information.append(tr("some keyboard shortcuts are not unique"));
  }

  // User selected custom external browser but did not set its
  // properties.
  if (m_ui->m_grpCustomExternalBrowser->isChecked() &&
      (m_ui->m_txtExternalBrowserExecutable->text().simplified().isEmpty() ||
       !m_ui->m_txtExternalBrowserArguments->text().simplified().contains(QL1S("%1")))) {
    everything_ok = false;
    resulting_information.append(tr("custom external browser is not set correctly"));
  }

  if (!everything_ok) {
    resulting_information.replaceInStrings(QRegExp(QSL("^")), QString::fromUtf8(" • "));

    MessageBox::show(this,
                     QMessageBox::Critical,
                     tr("Cannot save settings"),
                     tr("Some critical settings are not set. You must fix these settings in order confirm new settings."),
                     QString(),
                     tr("List of errors:\n%1.").arg(resulting_information.join(QSL(",\n"))));
  }

  return everything_ok;
}

void FormSettings::promptForRestart() {
  if (!m_changedDataTexts.isEmpty()) {
    const QStringList changed_settings_description = m_changedDataTexts.replaceInStrings(QRegExp(QSL("^")), QString::fromUtf8(" • "));
    MessageBox::show(this,
                     QMessageBox::Question,
                     tr("Critical settings were changed"),
                     tr("Some critical settings were changed and will be applied after the application gets restarted. "
                        "\n\nYou have to restart manually."),
                     QString(),
                     tr("List of changes:\n%1.").arg(changed_settings_description .join(QSL(",\n"))),
                     QMessageBox::Ok, QMessageBox::Ok);
  }
}

void FormSettings::saveSettings() {
  // Make sure everything is saveable.
  if (!doSaveCheck()) {
    return;
  }

  // Save all settings.
  saveGeneral();
  saveDataStorage();
  saveShortcuts();
  saveInterface();
  saveProxy();
  saveBrowser();
  saveLanguage();
  saveFeedsMessages();
  saveDownloads();

  m_settings->checkSettings();
  promptForRestart();

  accept();
}

void FormSettings::onProxyTypeChanged(int index) {
  const QNetworkProxy::ProxyType selected_type = static_cast<QNetworkProxy::ProxyType>(m_ui->m_cmbProxyType->itemData(index).toInt());
  const bool is_proxy_selected = selected_type != QNetworkProxy::NoProxy && selected_type != QNetworkProxy::DefaultProxy;

  m_ui->m_txtProxyHost->setEnabled(is_proxy_selected);
  m_ui->m_txtProxyPassword->setEnabled(is_proxy_selected);
  m_ui->m_txtProxyUsername->setEnabled(is_proxy_selected);
  m_ui->m_spinProxyPort->setEnabled(is_proxy_selected);
  m_ui->m_checkShowPassword->setEnabled(is_proxy_selected);
  m_ui->m_lblProxyHost->setEnabled(is_proxy_selected);
  m_ui->m_lblProxyInfo->setEnabled(is_proxy_selected);
  m_ui->m_lblProxyPassword->setEnabled(is_proxy_selected);
  m_ui->m_lblProxyPort->setEnabled(is_proxy_selected);
  m_ui->m_lblProxyUsername->setEnabled(is_proxy_selected);
}

void FormSettings::loadBrowser() {
  // Load settings of web browser GUI.
  m_ui->m_cmbExternalBrowserPreset->addItem(tr("Opera 12 or older"), QSL("-nosession %1"));
  m_ui->m_txtExternalBrowserExecutable->setText(m_settings->value(GROUP(Browser), SETTING(Browser::CustomExternalBrowserExecutable)).toString());
  m_ui->m_txtExternalBrowserArguments->setText(m_settings->value(GROUP(Browser), SETTING(Browser::CustomExternalBrowserArguments)).toString());
  m_ui->m_grpCustomExternalBrowser->setChecked(m_settings->value(GROUP(Browser), SETTING(Browser::CustomExternalBrowserEnabled)).toBool());

  // Load settings of e-mail.
  m_ui->m_cmbExternalEmailPreset->addItem(tr("Mozilla Thunderbird"), QSL("-compose \"subject='%1',body='%2'\""));
  m_ui->m_txtExternalEmailExecutable->setText(m_settings->value(GROUP(Browser), SETTING(Browser::CustomExternalEmailExecutable)).toString());
  m_ui->m_txtExternalEmailArguments->setText(m_settings->value(GROUP(Browser), SETTING(Browser::CustomExternalEmailArguments)).toString());
  m_ui->m_grpCustomExternalEmail->setChecked(m_settings->value(GROUP(Browser), SETTING(Browser::CustomExternalEmailEnabled)).toBool());
}

void FormSettings::saveBrowser() {
  // Save settings of GUI of web browser.
  m_settings->setValue(GROUP(Browser), Browser::CustomExternalBrowserEnabled, m_ui->m_grpCustomExternalBrowser->isChecked());
  m_settings->setValue(GROUP(Browser), Browser::CustomExternalBrowserExecutable, m_ui->m_txtExternalBrowserExecutable->text());
  m_settings->setValue(GROUP(Browser), Browser::CustomExternalBrowserArguments, m_ui->m_txtExternalBrowserArguments->text());

  // Save settings of e-mail.
  m_settings->setValue(GROUP(Browser), Browser::CustomExternalEmailExecutable, m_ui->m_txtExternalEmailExecutable->text());
  m_settings->setValue(GROUP(Browser), Browser::CustomExternalEmailArguments, m_ui->m_txtExternalEmailArguments->text());
  m_settings->setValue(GROUP(Browser), Browser::CustomExternalEmailEnabled, m_ui->m_grpCustomExternalEmail->isChecked());
}

void FormSettings::loadProxy() {
  m_ui->m_cmbProxyType->addItem(tr("No proxy"), QNetworkProxy::NoProxy);
  m_ui->m_cmbProxyType->addItem(tr("System proxy"), QNetworkProxy::DefaultProxy);
  m_ui->m_cmbProxyType->addItem(tr("Socks5"), QNetworkProxy::Socks5Proxy);
  m_ui->m_cmbProxyType->addItem(tr("Http"), QNetworkProxy::HttpProxy);

  // Load the settings.
  QNetworkProxy::ProxyType selected_proxy_type = static_cast<QNetworkProxy::ProxyType>(m_settings->value(GROUP(Proxy), SETTING(Proxy::Type)).toInt());

  m_ui->m_cmbProxyType->setCurrentIndex(m_ui->m_cmbProxyType->findData(selected_proxy_type));
  m_ui->m_txtProxyHost->setText(m_settings->value(GROUP(Proxy), SETTING(Proxy::Host)).toString());
  m_ui->m_txtProxyUsername->setText(m_settings->value(GROUP(Proxy), SETTING(Proxy::Username)).toString());
  m_ui->m_txtProxyPassword->setText(TextFactory::decrypt(m_settings->value(GROUP(Proxy), SETTING(Proxy::Password)).toString()));
  m_ui->m_spinProxyPort->setValue(m_settings->value(GROUP(Proxy), SETTING(Proxy::Port)).toInt());
}

void FormSettings::saveProxy() {
  m_settings->setValue(GROUP(Proxy), Proxy::Type, m_ui->m_cmbProxyType->itemData(m_ui->m_cmbProxyType->currentIndex()));
  m_settings->setValue(GROUP(Proxy), Proxy::Host, m_ui->m_txtProxyHost->text());
  m_settings->setValue(GROUP(Proxy), Proxy::Username,  m_ui->m_txtProxyUsername->text());
  m_settings->setValue(GROUP(Proxy), Proxy::Password, TextFactory::encrypt(m_ui->m_txtProxyPassword->text()));
  m_settings->setValue(GROUP(Proxy), Proxy::Port, m_ui->m_spinProxyPort->value());

  // Reload settings for all network access managers.
  SilentNetworkAccessManager::instance()->loadSettings();
}

void FormSettings::loadLanguage() {
  foreach (const Language &language, qApp->localization()->installedLanguages()) {
    QTreeWidgetItem *item = new QTreeWidgetItem(m_ui->m_treeLanguages);
    item->setText(0, language.m_name);
    item->setText(1, language.m_code);
    item->setText(2, language.m_author);
    item->setIcon(0, qApp->icons()->miscIcon(QString(FLAG_ICON_SUBFOLDER) + QDir::separator() + language.m_code));
  }

  QList<QTreeWidgetItem*> matching_items = m_ui->m_treeLanguages->findItems(qApp->localization()->loadedLanguage(), Qt::MatchContains, 1);

  if (!matching_items.isEmpty()) {
    m_ui->m_treeLanguages->setCurrentItem(matching_items[0]);
  }
}

void FormSettings::saveLanguage() {
  if (m_ui->m_treeLanguages->currentItem() == nullptr) {
    qDebug("No localizations loaded in settings dialog, so no saving for them.");
    return;
  }

  const QString actual_lang = qApp->localization()->loadedLanguage();
  const QString new_lang = m_ui->m_treeLanguages->currentItem()->text(1);

  // Save prompt for restart if language has changed.
  if (new_lang != actual_lang) {
    m_changedDataTexts.append(tr("language changed"));
    m_settings->setValue(GROUP(General), General::Language, new_lang);
  }
}

void FormSettings::loadShortcuts() {
  m_ui->m_shortcuts->populate(qApp->mainForm()->allActions());
}

void FormSettings::saveShortcuts() {
  // Update the actual shortcuts of some actions.
  m_ui->m_shortcuts->updateShortcuts();

  // Save new shortcuts to the settings.
  DynamicShortcuts::save(qApp->mainForm()->allActions());
}

void FormSettings::loadDataStorage() {
  m_ui->m_lblMysqlTestResult->setStatus(WidgetWithStatus::Information,  tr("No connection test triggered so far."), tr("You did not executed any connection test yet."));

  // Load SQLite.
  m_ui->m_cmbDatabaseDriver->addItem(qApp->database()->humanDriverName(DatabaseFactory::SQLITE), APP_DB_SQLITE_DRIVER);

  // Load in-memory database status.
  m_ui->m_checkSqliteUseInMemoryDatabase->setChecked(m_settings->value(GROUP(Database), SETTING(Database::UseInMemory)).toBool());

  if (QSqlDatabase::isDriverAvailable(APP_DB_MYSQL_DRIVER)) {
    onMysqlHostnameChanged(QString());
    onMysqlUsernameChanged(QString());
    onMysqlPasswordChanged(QString());
    onMysqlDatabaseChanged(QString());

    // Load MySQL.
    m_ui->m_cmbDatabaseDriver->addItem(qApp->database()->humanDriverName(DatabaseFactory::MYSQL), APP_DB_MYSQL_DRIVER);

    // Setup placeholders.
    m_ui->m_txtMysqlHostname->lineEdit()->setPlaceholderText(tr("Hostname of your MySQL server"));
    m_ui->m_txtMysqlUsername->lineEdit()->setPlaceholderText(tr("Username to login with"));
    m_ui->m_txtMysqlPassword->lineEdit()->setPlaceholderText(tr("Password for your username"));
    m_ui->m_txtMysqlDatabase->lineEdit()->setPlaceholderText(tr("Working database which you have full access to."));

    m_ui->m_txtMysqlHostname->lineEdit()->setText(m_settings->value(GROUP(Database), SETTING(Database::MySQLHostname)).toString());
    m_ui->m_txtMysqlUsername->lineEdit()->setText(m_settings->value(GROUP(Database), SETTING(Database::MySQLUsername)).toString());
    m_ui->m_txtMysqlPassword->lineEdit()->setText(TextFactory::decrypt(m_settings->value(GROUP(Database), SETTING(Database::MySQLPassword)).toString()));
    m_ui->m_txtMysqlDatabase->lineEdit()->setText(m_settings->value(GROUP(Database), SETTING(Database::MySQLDatabase)).toString());
    m_ui->m_spinMysqlPort->setValue(m_settings->value(GROUP(Database), SETTING(Database::MySQLPort)).toInt());

    m_ui->m_checkMysqlShowPassword->setChecked(false);
  }

  int index_current_backend = m_ui->m_cmbDatabaseDriver->findData(m_settings->value(GROUP(Database), SETTING(Database::ActiveDriver)).toString());

  if (index_current_backend >= 0) {
    m_ui->m_cmbDatabaseDriver->setCurrentIndex(index_current_backend);
  }
}

void FormSettings::saveDataStorage() {
  // Setup in-memory database status.
  const bool original_inmemory = m_settings->value(GROUP(Database), SETTING(Database::UseInMemory)).toBool();
  const bool new_inmemory = m_ui->m_checkSqliteUseInMemoryDatabase->isChecked();

  if (original_inmemory != new_inmemory) {
    m_changedDataTexts.append(tr("in-memory database switched"));
  }

  // Save data storage settings.
  QString original_db_driver = m_settings->value(GROUP(Database), SETTING(Database::ActiveDriver)).toString();
  QString selected_db_driver = m_ui->m_cmbDatabaseDriver->itemData(m_ui->m_cmbDatabaseDriver->currentIndex()).toString();

  // Save SQLite.
  m_settings->setValue(GROUP(Database), Database::UseInMemory, new_inmemory);

  if (QSqlDatabase::isDriverAvailable(APP_DB_MYSQL_DRIVER)) {
    // Save MySQL.
    m_settings->setValue(GROUP(Database), Database::MySQLHostname, m_ui->m_txtMysqlHostname->lineEdit()->text());
    m_settings->setValue(GROUP(Database), Database::MySQLUsername, m_ui->m_txtMysqlUsername->lineEdit()->text());
    m_settings->setValue(GROUP(Database), Database::MySQLPassword, TextFactory::encrypt(m_ui->m_txtMysqlPassword->lineEdit()->text()));
    m_settings->setValue(GROUP(Database), Database::MySQLDatabase, m_ui->m_txtMysqlDatabase->lineEdit()->text());
    m_settings->setValue(GROUP(Database), Database::MySQLPort, m_ui->m_spinMysqlPort->value());
  }

  m_settings->setValue(GROUP(Database), Database::ActiveDriver, selected_db_driver);

  if (original_db_driver != selected_db_driver || m_initialSettings.m_dataStorageDataChanged) {
    m_changedDataTexts.append(tr("data storage backend changed"));
  }
}

void FormSettings::mysqlTestConnection() {
  const DatabaseFactory::MySQLError error_code = qApp->database()->mysqlTestConnection(m_ui->m_txtMysqlHostname->lineEdit()->text(),
                                                                                       m_ui->m_spinMysqlPort->value(),
                                                                                       m_ui->m_txtMysqlDatabase->lineEdit()->text(),
                                                                                       m_ui->m_txtMysqlUsername->lineEdit()->text(),
                                                                                       m_ui->m_txtMysqlPassword->lineEdit()->text());
  const QString interpretation = qApp->database()->mysqlInterpretErrorCode(error_code);


  switch (error_code) {
    case DatabaseFactory::MySQLOk:
    case DatabaseFactory::MySQLUnknownDatabase:
      m_ui->m_lblMysqlTestResult->setStatus(WidgetWithStatus::Ok, interpretation, interpretation);
      break;

    default:
      m_ui->m_lblMysqlTestResult->setStatus(WidgetWithStatus::Error, interpretation, interpretation);
      break;
  }
}

void FormSettings::onMysqlHostnameChanged(const QString &new_hostname) {
  if (new_hostname.isEmpty()) {
    m_ui->m_txtMysqlHostname->setStatus(LineEditWithStatus::Warning, tr("Hostname is empty."));
  }
  else {
    m_ui->m_txtMysqlHostname->setStatus(LineEditWithStatus::Ok, tr("Hostname looks ok."));
  }
}

void FormSettings::onMysqlUsernameChanged(const QString &new_username) {
  if (new_username.isEmpty()) {
    m_ui->m_txtMysqlUsername->setStatus(LineEditWithStatus::Warning, tr("Username is empty."));
  }
  else {
    m_ui->m_txtMysqlUsername->setStatus(LineEditWithStatus::Ok, tr("Username looks ok."));
  }
}

void FormSettings::onMysqlPasswordChanged(const QString &new_password) {
  if (new_password.isEmpty()) {
    m_ui->m_txtMysqlPassword->setStatus(LineEditWithStatus::Warning, tr("Password is empty."));
  }
  else {
    m_ui->m_txtMysqlPassword->setStatus(LineEditWithStatus::Ok, tr("Password looks ok."));
  }
}

void FormSettings::onMysqlDatabaseChanged(const QString &new_database) {
  if (new_database.isEmpty()) {
    m_ui->m_txtMysqlDatabase->setStatus(LineEditWithStatus::Warning, tr("Working database is empty."));
  }
  else {
    m_ui->m_txtMysqlDatabase->setStatus(LineEditWithStatus::Ok, tr("Working database is ok."));
  }
}

void FormSettings::onMysqlDataStorageEdited() {
  m_initialSettings.m_dataStorageDataChanged = true;
}

void FormSettings::selectSqlBackend(int index) {
  const QString selected_db_driver = m_ui->m_cmbDatabaseDriver->itemData(index).toString();

  if (selected_db_driver == APP_DB_SQLITE_DRIVER) {
    m_ui->m_stackedDatabaseDriver->setCurrentIndex(0);
  }
  else if (selected_db_driver == APP_DB_MYSQL_DRIVER) {
    m_ui->m_stackedDatabaseDriver->setCurrentIndex(1);
  }
  else {
    qWarning("GUI for given database driver '%s' is not available.", qPrintable(selected_db_driver));
  }
}

void FormSettings::switchMysqlPasswordVisiblity(bool visible) {
  m_ui->m_txtMysqlPassword->lineEdit()->setEchoMode(visible ? QLineEdit::Normal : QLineEdit::Password);
}

void FormSettings::loadGeneral() {
  m_ui->m_checkAutostart->setText(m_ui->m_checkAutostart->text().arg(APP_NAME));
  m_ui->m_checkForUpdatesOnStart->setChecked(m_settings->value(GROUP(General), SETTING(General::UpdateOnStartup)).toBool());

  // Load auto-start status.
  const SystemFactory::AutoStartStatus autostart_status = qApp->system()->getAutoStartStatus();

  switch (autostart_status) {
    case SystemFactory::Enabled:
      m_ui->m_checkAutostart->setChecked(true);
      break;

    case SystemFactory::Disabled:
      m_ui->m_checkAutostart->setChecked(false);
      break;

    default:
      m_ui->m_checkAutostart->setEnabled(false);
      m_ui->m_checkAutostart->setText(m_ui->m_checkAutostart->text() + tr(" (not supported on this platform)"));
      break;
  }

#if defined(Q_OS_WIN)
  m_ui->m_checkRemoveTrolltechJunk->setVisible(true);
  m_ui->m_checkRemoveTrolltechJunk->setChecked(m_settings->value(GROUP(General), SETTING(General::RemoveTrolltechJunk)).toBool());
#else
  m_ui->m_checkRemoveTrolltechJunk->setVisible(false);
#endif
}

void FormSettings::saveGeneral() {
  // If auto-start feature is available and user wants to turn it on, then turn it on.
  if (m_ui->m_checkAutostart->isChecked()) {
    qApp->system()->setAutoStartStatus(SystemFactory::Enabled);
  }
  else {
    qApp->system()->setAutoStartStatus(SystemFactory::Disabled);
  }

  m_settings->setValue(GROUP(General), General::UpdateOnStartup, m_ui->m_checkForUpdatesOnStart->isChecked());
  m_settings->setValue(GROUP(General), General::RemoveTrolltechJunk, m_ui->m_checkRemoveTrolltechJunk->isChecked());
}

void FormSettings::loadInterface() {
  // Load settings of tray icon.
  if (SystemTrayIcon::isSystemTrayAvailable()) {
    m_ui->m_grpTray->setChecked(m_settings->value(GROUP(GUI), SETTING(GUI::UseTrayIcon)).toBool());
  }
  // Tray icon is not supported on this machine.
  else {
    m_ui->m_grpTray->setTitle(m_ui->m_grpTray->title() + QL1C(' ') + tr("(Tray icon is not available.)"));
    m_ui->m_grpTray->setChecked(false);
  }

  m_ui->m_checkHidden->setChecked(m_settings->value(GROUP(GUI), SETTING(GUI::MainWindowStartsHidden)).toBool());
  m_ui->m_checkHideWhenMinimized->setChecked(m_settings->value(GROUP(GUI), SETTING(GUI::HideMainWindowWhenMinimized)).toBool());

  // Load fancy notification settings.
  m_ui->m_checkEnableNotifications->setChecked(m_settings->value(GROUP(GUI), SETTING(GUI::EnableNotifications)).toBool());

  // Load settings of icon theme.
  const QString current_theme = qApp->icons()->currentIconTheme();

  foreach (const QString &icon_theme_name, qApp->icons()->installedIconThemes()) {
    if (icon_theme_name == APP_NO_THEME) {
      // Add just "no theme" on other systems.
      //: Label for disabling icon theme.
      m_ui->m_cmbIconTheme->addItem(tr("no icon theme/system icon theme"), APP_NO_THEME);
    }
    else {
      m_ui->m_cmbIconTheme->addItem(icon_theme_name, icon_theme_name);
    }
  }

  // Mark active theme.
  if (current_theme == QSL(APP_NO_THEME)) {
    // Because "no icon theme" lies at the index 0.
    m_ui->m_cmbIconTheme->setCurrentIndex(0);
  }
  else {
    m_ui->m_cmbIconTheme->setCurrentText(current_theme);
  }

  // Load skin.
  const QString selected_skin = qApp->skins()->selectedSkinName();

  foreach (const Skin &skin, qApp->skins()->installedSkins()) {
    QTreeWidgetItem *new_item = new QTreeWidgetItem(QStringList() <<
                                                    skin.m_visibleName <<
                                                    skin.m_version <<
                                                    skin.m_author <<
                                                    skin.m_email);
    new_item->setData(0, Qt::UserRole, QVariant::fromValue(skin));

    // Add this skin and mark it as active if its active now.
    m_ui->m_treeSkins->addTopLevelItem(new_item);

    if (skin.m_baseName == selected_skin) {
      m_ui->m_treeSkins->setCurrentItem(new_item);
      m_ui->m_lblActiveContents->setText(skin.m_visibleName);
    }
  }

  if (m_ui->m_treeSkins->currentItem() == nullptr &&
      m_ui->m_treeSkins->topLevelItemCount() > 0) {
    // Currently active skin is NOT available, select another one as selected
    // if possible.
    m_ui->m_treeSkins->setCurrentItem(m_ui->m_treeSkins->topLevelItem(0));
  }

  // Load tab settings.
  m_ui->m_checkCloseTabsMiddleClick->setChecked(m_settings->value(GROUP(GUI), SETTING(GUI::TabCloseMiddleClick)).toBool());
  m_ui->m_checkCloseTabsDoubleClick->setChecked(m_settings->value(GROUP(GUI), SETTING(GUI::TabCloseDoubleClick)).toBool());
  m_ui->m_checkNewTabDoubleClick->setChecked(m_settings->value(GROUP(GUI), SETTING(GUI::TabNewDoubleClick)).toBool());
  m_ui->m_hideTabBarIfOneTabVisible->setChecked(m_settings->value(GROUP(GUI), SETTING(GUI::HideTabBarIfOnlyOneTab)).toBool());

  // Load toolbar button style.
  m_ui->m_cmbToolbarButtonStyle->addItem(tr("Icon only"), Qt::ToolButtonIconOnly);
  m_ui->m_cmbToolbarButtonStyle->addItem(tr("Text only"), Qt::ToolButtonTextOnly);
  m_ui->m_cmbToolbarButtonStyle->addItem(tr("Text beside icon"), Qt::ToolButtonTextBesideIcon);
  m_ui->m_cmbToolbarButtonStyle->addItem(tr("Text under icon"), Qt::ToolButtonTextUnderIcon);
  m_ui->m_cmbToolbarButtonStyle->addItem(tr("Follow OS style"), Qt::ToolButtonFollowStyle);

  m_ui->m_cmbToolbarButtonStyle->setCurrentIndex(m_ui->m_cmbToolbarButtonStyle->findData(m_settings->value(GROUP(GUI),
                                                                                                           SETTING(GUI::ToolbarStyle)).toInt()));

  // Load toolbars.
  m_ui->m_editorFeedsToolbar->loadFromToolBar(qApp->mainForm()->tabWidget()->feedMessageViewer()->feedsToolBar());
  m_ui->m_editorMessagesToolbar->loadFromToolBar(qApp->mainForm()->tabWidget()->feedMessageViewer()->messagesToolBar());
  m_ui->m_editorStatusbar->loadFromToolBar(qApp->mainForm()->statusBar());
}

void FormSettings::saveInterface() {
  // Save toolbar.
  m_settings->setValue(GROUP(GUI), GUI::ToolbarStyle, m_ui->m_cmbToolbarButtonStyle->itemData(m_ui->m_cmbToolbarButtonStyle->currentIndex()));

  // Save tray icon.
  if (SystemTrayIcon::isSystemTrayAvailable()) {
    m_settings->setValue(GROUP(GUI), GUI::UseTrayIcon, m_ui->m_grpTray->isChecked());

    if (m_ui->m_grpTray->isChecked()) {
      qApp->showTrayIcon();
    }
    else {
      qApp->deleteTrayIcon();
    }
  }

  m_settings->setValue(GROUP(GUI), GUI::MainWindowStartsHidden, m_ui->m_checkHidden->isChecked());
  m_settings->setValue(GROUP(GUI), GUI::HideMainWindowWhenMinimized, m_ui->m_checkHideWhenMinimized->isChecked());

  // Save notifications.
  m_settings->setValue(GROUP(GUI), GUI::EnableNotifications, m_ui->m_checkEnableNotifications->isChecked());

  // Save selected icon theme.
  QString selected_icon_theme = m_ui->m_cmbIconTheme->itemData(m_ui->m_cmbIconTheme->currentIndex()).toString();
  QString original_icon_theme = qApp->icons()->currentIconTheme();
  qApp->icons()->setCurrentIconTheme(selected_icon_theme);

  // Check if icon theme was changed.
  if (selected_icon_theme != original_icon_theme) {
    m_changedDataTexts.append(tr("icon theme changed"));
  }

  // Save and activate new skin.
  if (m_ui->m_treeSkins->selectedItems().size() > 0) {
    const Skin active_skin = m_ui->m_treeSkins->currentItem()->data(0, Qt::UserRole).value<Skin>();

    if (qApp->skins()->selectedSkinName() != active_skin.m_baseName) {
      qApp->skins()->setCurrentSkinName(active_skin.m_baseName);
      m_changedDataTexts.append(tr("skin changed"));
    }
  }

  // Save tab settings.
  m_settings->setValue(GROUP(GUI), GUI::TabCloseMiddleClick,  m_ui->m_checkCloseTabsMiddleClick->isChecked());
  m_settings->setValue(GROUP(GUI), GUI::TabCloseDoubleClick, m_ui->m_checkCloseTabsDoubleClick->isChecked());
  m_settings->setValue(GROUP(GUI), GUI::TabNewDoubleClick, m_ui->m_checkNewTabDoubleClick->isChecked());
  m_settings->setValue(GROUP(GUI), GUI::HideTabBarIfOnlyOneTab, m_ui->m_hideTabBarIfOneTabVisible->isChecked());

  m_ui->m_editorFeedsToolbar->saveToolBar();
  m_ui->m_editorMessagesToolbar->saveToolBar();
  m_ui->m_editorStatusbar->saveToolBar();

  qApp->mainForm()->tabWidget()->checkTabBarVisibility();
  qApp->mainForm()->tabWidget()->feedMessageViewer()->refreshVisualProperties();
}

bool FormSettings::eventFilter(QObject *obj, QEvent *e) {
  Q_UNUSED(obj)

  if (e->type() == QEvent::Drop) {
    QDropEvent *drop_event = static_cast<QDropEvent*>(e);

    if (drop_event->keyboardModifiers() != Qt::NoModifier) {
      drop_event->setDropAction(Qt::MoveAction);
    }
  }

  return false;
}
