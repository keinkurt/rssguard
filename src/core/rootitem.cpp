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

#include "core/rootitem.h"

#include "core/category.h"
#include "core/feed.h"
#include "core/recyclebin.h"
#include "miscellaneous/application.h"

#include <QVariant>


RootItem::RootItem(RootItem *parent_item)
  : m_kind(RootItem::Root),
    m_id(NO_PARENT_CATEGORY),
    m_title(QString()),
    m_description(QString()),
    m_icon(QIcon()),
    m_creationDate(QDateTime()),
    m_childItems(QList<RootItem*>()),
    m_parentItem(parent_item) {
  setupFonts();
}

RootItem::~RootItem() {
  qDeleteAll(m_childItems);
}

void RootItem::setupFonts() {
  m_normalFont = Application::font("FeedsView");
  m_boldFont = m_normalFont;
  m_boldFont.setBold(true);
}

int RootItem::row() const {
  if (m_parentItem) {
    return m_parentItem->m_childItems.indexOf(const_cast<RootItem*>(this));
  }
  else {
    // This item has no parent. Therefore, its row index is 0.
    return 0;
  }
}

QVariant RootItem::data(int column, int role) const {
  Q_UNUSED(column)
  Q_UNUSED(role)

  // Do not return anything for the root item.
  return QVariant();
}

int RootItem::countOfAllMessages() const {
  int total_count = 0;

  foreach (RootItem *child_item, m_childItems) {
    if (child_item->kind() != RootItem::Bin) {
      total_count += child_item->countOfAllMessages();
    }
  }

  return total_count;
}

QList<RootItem*> RootItem::getRecursiveChildren() {
  QList<RootItem*> children;

  if (kind() == RootItem::Feeed) {
    // Root itself is a FEED.
    children.append(this);
  }
  else {
    // Root itself is a CATEGORY or ROOT item.
    QList<RootItem*> traversable_items;

    traversable_items.append(this);

    // Iterate all nested categories.
    while (!traversable_items.isEmpty()) {
      RootItem *active_category = traversable_items.takeFirst();

      foreach (RootItem *child, active_category->childItems()) {
        if (child->kind() == RootItem::Feeed) {
          // This child is feed.
          children.append(child);
        }
        else if (child->kind() == RootItem::Cattegory) {
          // This child is category, add its child feeds too.
          traversable_items.append(child);
        }
      }
    }
  }

  return children;
}

bool RootItem::removeChild(RootItem *child) {
  return m_childItems.removeOne(child);
}

RecycleBin *RootItem::toRecycleBin() {
  return static_cast<RecycleBin*>(this);
}

Category *RootItem::toCategory() {
  return static_cast<Category*>(this);
}

Feed *RootItem::toFeed() {
  return static_cast<Feed*>(this);
}

RootItem *RootItem::child(RootItem::Kind kind_of_child, const QString &identifier) {
  foreach (RootItem *child, childItems()) {
    if (child->kind() == kind_of_child) {
      if ((kind_of_child == Cattegory && child->title() == identifier) ||
          (kind_of_child == Feeed && child->toFeed()->url() == identifier)) {
        return child;
      }
    }
  }

  return NULL;
}

int RootItem::countOfUnreadMessages() const {
  int total_count = 0;

  foreach (RootItem *child_item, m_childItems) {
    if (child_item->kind() != RootItem::Bin) {
      total_count += child_item->countOfUnreadMessages();
    }
  }

  return total_count;
}

bool RootItem::removeChild(int index) {
  if (index >= 0 && index < m_childItems.size()) {
    m_childItems.removeAt(index);
    return true;
  }
  else {
    return false;
  }
}

bool RootItem::isEqual(RootItem *lhs, RootItem *rhs) {
  return (lhs->kind() == rhs->kind()) && (lhs->id() == rhs->id());
}

bool RootItem::lessThan(RootItem *lhs, RootItem *rhs) {
  if (lhs->kind() == rhs->kind()) {
    return lhs->id() < rhs->id();
  }
  else {
    return false;
  }
}