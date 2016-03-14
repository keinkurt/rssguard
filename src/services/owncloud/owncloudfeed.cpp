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

#include "owncloudfeed.h"

#include "miscellaneous/iconfactory.h"
#include "services/owncloud/owncloudserviceroot.h"
#include "services/owncloud/network/owncloudnetworkfactory.h"


OwnCloudFeed::OwnCloudFeed(RootItem *parent) : Feed(parent) {
}

OwnCloudFeed::OwnCloudFeed(const QSqlRecord &record) : Feed(NULL) {
  setTitle(record.value(FDS_DB_TITLE_INDEX).toString());
  setId(record.value(FDS_DB_ID_INDEX).toInt());
  setIcon(qApp->icons()->fromByteArray(record.value(FDS_DB_ICON_INDEX).toByteArray()));
  setAutoUpdateType(static_cast<Feed::AutoUpdateType>(record.value(FDS_DB_UPDATE_TYPE_INDEX).toInt()));
  setAutoUpdateInitialInterval(record.value(FDS_DB_UPDATE_INTERVAL_INDEX).toInt());
  setCustomId(record.value(FDS_DB_CUSTOM_ID_INDEX).toInt());
}

OwnCloudFeed::~OwnCloudFeed() {
}

bool OwnCloudFeed::markAsReadUnread(RootItem::ReadStatus status) {
  QStringList ids = getParentServiceRoot()->customIDSOfMessagesForItem(this);
  QNetworkReply::NetworkError response = serviceRoot()->network()->markMessagesRead(status, ids);

  if (response != QNetworkReply::NoError) {
    return false;
  }
  else {
    return getParentServiceRoot()->markFeedsReadUnread(QList<Feed*>() << this, status);
  }
}

bool OwnCloudFeed::cleanMessages(bool clear_only_read) {
  return getParentServiceRoot()->cleanFeeds(QList<Feed*>() << this, clear_only_read);
}

OwnCloudServiceRoot *OwnCloudFeed::serviceRoot() const {
  return qobject_cast<OwnCloudServiceRoot*>(getParentServiceRoot());
}

QList<Message> OwnCloudFeed::obtainNewMessages() {
  OwnCloudGetMessagesResponse messages = serviceRoot()->network()->getMessages(customId());

  if (serviceRoot()->network()->lastError() != QNetworkReply::NoError) {
    setStatus(Feed::Error);
    serviceRoot()->itemChanged(QList<RootItem*>() << this);
    return QList<Message>();
  }
  else {
    return messages.messages();
  }

  return QList<Message>();
}
