// This file is part of RSS Guard.
//
// Copyright (C) 2011-2015 by Martin Rotter <rotter.martinos@gmail.com>
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

#include "services/tt-rss/ttrssfeed.h"

#include "definitions/definitions.h"
#include "miscellaneous/application.h"
#include "miscellaneous/databasefactory.h"
#include "miscellaneous/iconfactory.h"
#include "miscellaneous/textfactory.h"
#include "services/tt-rss/definitions.h"
#include "services/tt-rss/ttrssserviceroot.h"
#include "services/tt-rss/network/ttrssnetworkfactory.h"

#include <QSqlQuery>


TtRssFeed::TtRssFeed(RootItem *parent)
  : Feed(parent), m_customId(NO_PARENT_CATEGORY), m_totalCount(0), m_unreadCount(0) {
}

TtRssFeed::TtRssFeed(const QSqlRecord &record) : Feed(NULL), m_totalCount(0), m_unreadCount(0) {
  setTitle(record.value(FDS_DB_TITLE_INDEX).toString());
  setId(record.value(FDS_DB_ID_INDEX).toInt());
  setIcon(qApp->icons()->fromByteArray(record.value(FDS_DB_ICON_INDEX).toByteArray()));
  setAutoUpdateType(static_cast<Feed::AutoUpdateType>(record.value(FDS_DB_UPDATE_TYPE_INDEX).toInt()));
  setAutoUpdateInitialInterval(record.value(FDS_DB_UPDATE_INTERVAL_INDEX).toInt());
  setCustomId(record.value(FDS_DB_CUSTOM_ID_INDEX).toInt());
}

TtRssFeed::~TtRssFeed() {
}

TtRssServiceRoot *TtRssFeed::serviceRoot() {
  return qobject_cast<TtRssServiceRoot*>(getParentServiceRoot());
}

void TtRssFeed::updateCounts(bool including_total_count) {
  QSqlDatabase database = qApp->database()->connection(metaObject()->className(), DatabaseFactory::FromSettings);
  QSqlQuery query_all(database);

  query_all.setForwardOnly(true);

  if (including_total_count) {
    if (query_all.exec(QString("SELECT count(*) FROM Messages WHERE feed = '%1' AND is_deleted = 0 AND account_id = %2;").arg(QString::number(customId()),
                                                                                                                              QString::number(serviceRoot()->accountId()))) && query_all.next()) {
      m_totalCount = query_all.value(0).toInt();
    }
  }

  // Obtain count of unread messages.
  if (query_all.exec(QString("SELECT count(*) FROM Messages WHERE feed = '%1' AND is_deleted = 0 AND is_read = 0 AND account_id = %2;").arg(QString::number(customId()),
                                                                                                                                            QString::number(serviceRoot()->accountId()))) && query_all.next()) {
    int new_unread_count = query_all.value(0).toInt();

    if (status() == NewMessages && new_unread_count < m_unreadCount) {
      setStatus(Normal);
    }

    m_unreadCount = new_unread_count;
  }
}

int TtRssFeed::countOfAllMessages() {
  return m_totalCount;
}

int TtRssFeed::countOfUnreadMessages() {
  return m_unreadCount;
}

int TtRssFeed::update() {
  QNetworkReply::NetworkError error;
  QList<Message> messages;
  int newly_added_messages = 0;
  int limit = MAX_MESSAGES;
  int skip = 0;

  do {
    TtRssGetHeadlinesResponse headlines = serviceRoot()->network()->getHeadlines(customId(), true, limit, skip,
                                                                                 true, true, false, error);

    if (error != QNetworkReply::NoError) {
      setStatus(Feed::NetworkError);
      return 0;
    }
    else {
      QList<Message> new_messages = headlines.messages();

      messages.append(new_messages);
      newly_added_messages = new_messages.size();
      skip += newly_added_messages;
    }
  }
  while (newly_added_messages > 0);

  return updateMessages(messages);
}

QList<Message> TtRssFeed::undeletedMessages() const {
  QList<Message> messages;
  int account_id = const_cast<TtRssFeed*>(this)->serviceRoot()->accountId();
  QSqlDatabase database = qApp->database()->connection(metaObject()->className(), DatabaseFactory::FromSettings);
  QSqlQuery query_read_msg(database);
  query_read_msg.setForwardOnly(true);
  query_read_msg.prepare("SELECT title, url, author, date_created, contents, enclosures, custom_id "
                         "FROM Messages "
                         "WHERE is_deleted = 0 AND feed = :feed AND account_id = :account_id;");

  query_read_msg.bindValue(QSL(":feed"), id());
  query_read_msg.bindValue(QSL(":account_id"), account_id);

  // FIXME: Fix those const functions, this is fucking ugly.

  if (query_read_msg.exec()) {
    while (query_read_msg.next()) {
      Message message;

      message.m_feedId = QString::number(account_id);
      message.m_title = query_read_msg.value(0).toString();
      message.m_url = query_read_msg.value(1).toString();
      message.m_author = query_read_msg.value(2).toString();
      message.m_created = TextFactory::parseDateTime(query_read_msg.value(3).value<qint64>());
      message.m_contents = query_read_msg.value(4).toString();
      message.m_enclosures = Enclosures::decodeEnclosuresFromString(query_read_msg.value(5).toString());
      message.m_customId = query_read_msg.value(6).toString();

      messages.append(message);
    }
  }

  return messages;
}

int TtRssFeed::customId() const {
  return m_customId;
}

void TtRssFeed::setCustomId(int custom_id) {
  m_customId = custom_id;
}

int TtRssFeed::updateMessages(const QList<Message> &messages) {
  // TODO: pokračovat tady

  return 0;
}