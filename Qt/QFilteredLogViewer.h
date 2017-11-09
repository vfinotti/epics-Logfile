/* SPDX-License-Identifier: MIT */
#ifndef SUS_QFILTEREDLOGVIEWER_H_
#define SUS_QFILTEREDLOGVIEWER_H_

class QCheckBox;
class QSortFilterProxyModel;

#include "logfile_export.h"

#include <QList>
#include <QWidget>

namespace SuS {
namespace logfile {
class QLogViewer;

class LOGFILE_EXPORT QFilteredLogViewer : public QWidget {
   Q_OBJECT

 public:
   QFilteredLogViewer(QWidget *_parent = 0);

 private:
   QLogViewer *m_view;
   QSortFilterProxyModel *m_filter;
   QList<QCheckBox *> m_levelBoxes;

 private slots:
   void boxToggled(bool);
}; // class QFilteredLogViewer
} // namespace logfile
} // namespace SuS

#endif
