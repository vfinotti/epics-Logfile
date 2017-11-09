/* SPDX-License-Identifier: MIT */
#ifndef QLogList_h_
#define QLogList_h_

#include "log_event.h"
#include "output_stream.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wlong-long"
#include <QAbstractListModel>
#include <QList>
#pragma GCC diagnostic pop

class QTimer;

namespace SuS {
namespace logfile {
class QLogList : public QAbstractListModel, public output_stream {
   Q_OBJECT

 public:
   QLogList();

   virtual std::string name() override;

   virtual bool do_write(const log_event &_le) override;

 private slots:
   void expire();

 private:
   virtual int columnCount(const QModelIndex &_parent = QModelIndex()) const override;

   virtual int rowCount(const QModelIndex &_parent = QModelIndex()) const override;

   virtual QVariant data(const QModelIndex &_index, int _role) const override;

   virtual QVariant headerData(int _section, Qt::Orientation _orientation,
         int _role = Qt::DisplayRole) const override;

   typedef QList<log_event> log_list_t;
   log_list_t m_log_events;

   QTimer *const m_timer;
}; // class QLogList
} // namespace logfile
} // namespace SuS

#endif
