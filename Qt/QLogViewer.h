/* SPDX-License-Identifier: MIT */
#ifndef SUS_QLOGVIEWER_H_
#define SUS_QLOGVIEWER_H_

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wlong-long"
#pragma GCC diagnostic ignored "-Wshadow"
#include <QTreeView>
#pragma GCC diagnostic push

class QTimer;

namespace SuS {
namespace logfile {
class QLogList;

class QLogViewer : public QTreeView {
   Q_OBJECT

 public:
   QLogViewer(QWidget *_parent = 0);

 private:
   friend class QFilteredLogViewer;

   QLogList *m_loglist;
   bool m_autoScroll;
   QTimer *m_timer;

 private slots:
   void scrolled(int _value);
   void timeout();
}; // class QLogViewer
} // namespace logfile
} // namespace SuS

#endif
