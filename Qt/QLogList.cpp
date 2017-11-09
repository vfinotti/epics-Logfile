/* SPDX-License-Identifier: MIT */
#include "QLogList.h"

#include "log_event.h"
#include "logger.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wshadow"
#include <QBrush>
#include <QTimer>
#pragma GCC diagnostic pop

SuS::logfile::QLogList::QLogList() : QAbstractListModel(), m_timer(new QTimer{}) {
   // expire old entries every 30 seconds
   connect(m_timer, SIGNAL(timeout()), SLOT(expire()));
   m_timer->start(30000 /* ms */);

   SuS::logfile::logger::instance()->add_output_stream(this);
} // QLogList constructor

std::string SuS::logfile::QLogList::name() {
   return "QLogList";
} // QLogList::name

bool SuS::logfile::QLogList::do_write(const log_event &_le) {
   beginInsertRows(QModelIndex(), m_log_events.size(), m_log_events.size());
   m_log_events.append(_le);
   endInsertRows();
   return true;
} // QLogList::do_write

void SuS::logfile::QLogList::expire() {
   auto first_row = -1;
   log_list_t::iterator first_it;
   const auto now = std::chrono::system_clock::now();

   auto row = 0;
   for (log_list_t::iterator i = m_log_events.begin(); i != m_log_events.end();
         ++i) {
      // expire debug entries after one hour
      // TODO: different expiry times by log levels
      if (std::chrono::duration_cast<std::chrono::seconds>(i->time - now)
                  .count() > 3600) {
         if (first_row != -1) {
            beginRemoveRows(QModelIndex(), first_row, row - 1);
            m_log_events.erase(first_it, i);
            endRemoveRows();
         } // if
         return;
      } // if

      if (i->level == logger::log_level::finest ||
            i->level == logger::log_level::finer ||
            i->level == logger::log_level::fine) {
         if (first_row == -1) {
            first_row = row;
            first_it = i;
         } // if
      }    // if
      else {
         if (first_row != -1) {
            beginRemoveRows(QModelIndex(), first_row, row - 1);
            m_log_events.erase(first_it, i);
            endRemoveRows();
            first_row = -1;
         } // if
      }    // else

      ++row;
   } // for i

   if (first_row != -1) {
      beginRemoveRows(QModelIndex(), first_row, row - 1);
      m_log_events.erase(first_it, m_log_events.end());
      endRemoveRows();
   } // if
} // QLogList::expire

int SuS::logfile::QLogList::columnCount(const QModelIndex & // _parent
      ) const {
   return 4;
} // QLogList::columnCount

int SuS::logfile::QLogList::rowCount(const QModelIndex & // _parent
      ) const {
   return m_log_events.size();
} // QLogList::rowCount

QVariant SuS::logfile::QLogList::data(
      const QModelIndex &_index, int _role) const {
   if (!_index.isValid()) {
      return QVariant{};
   } // if

   if (_index.row() >= m_log_events.size()) {
      return QVariant{};
   } // if

   const log_event &le = m_log_events.at(_index.row());

   if (_role == Qt::DisplayRole) {
      switch (_index.column()) {
      case 0:
         return QString::fromStdString(le.time_string);
      case 1:
         return QString::fromStdString(logger::level_name(le.level));
      case 2:
         return QString::fromStdString(le.subsystem_string);
      case 3:
         return QString::fromStdString(le.message);
      default:
         return QVariant{};
      } // switch
   }    // if
   else if (_role == Qt::ForegroundRole) {
      // lowest levels are gray
      if (le.level < logger::log_level::info)
         return QBrush{Qt::gray};
      else
         return QBrush{Qt::black};
   } // else if
   else if (_role == Qt::BackgroundRole) {
      if (le.level == logger::log_level::warning)
         return QBrush{Qt::yellow};
      else if (le.level == logger::log_level::severe)
         return QBrush{Qt::red};
      else
         return QBrush{Qt::white};
   } // else if
   else {
      return QVariant{};
   } // else
} // QLogList::data

QVariant SuS::logfile::QLogList::headerData(int _section,
      Qt::Orientation /* _orientation */
      ,
      int _role) const {
   if (_role != Qt::DisplayRole) {
      return QVariant{};
   } // if

   switch (_section) {
   case 0:
      return QString{"Time"};
   case 1:
      return QString{"Severity"};
   case 2:
      return QString{"Component"};
   case 3:
      return QString{"Message"};
   default:
      return QVariant{};
   } // switch
} // QLogList::headerData
