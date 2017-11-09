/* SPDX-License-Identifier: MIT */
#include "QFilteredLogViewer.h"
#include "QLogList.h"
#include "QLogViewer.h"

#include <QCheckBox>
#include <QHBoxLayout>
#include <QSortFilterProxyModel>
#include <QStringList>
#include <QVBoxLayout>

SuS::logfile::QFilteredLogViewer::QFilteredLogViewer(QWidget *_parent)
   : QWidget(_parent) {
   auto vlayout = new QVBoxLayout{this};
   m_view = new QLogViewer{this};
   vlayout->addWidget(m_view);
   m_filter = new QSortFilterProxyModel{this};
   m_filter->setSourceModel(m_view->m_loglist);
   m_view->setModel(m_filter);

   auto hlayout = new QHBoxLayout{};
   vlayout->addLayout(hlayout);

   for (const auto &i : logger::all_levels()) {
      auto box =
            new QCheckBox(QString::fromStdString(logger::level_name(i)), this);
      box->setChecked(true);
      connect(box, SIGNAL(toggled(bool)), this, SLOT(boxToggled(bool)));
      hlayout->addWidget(box);
      m_levelBoxes.append(box);
   }

   m_filter->setFilterKeyColumn(1);
   boxToggled(false);
} // QFilteredLogViewer constructor

void SuS::logfile::QFilteredLogViewer::boxToggled(bool) {
   QStringList list;
   for (const auto &i : m_levelBoxes) {
      if (i->isChecked())
         list.push_back(i->text());
   } // if
   auto s = list.join("|");
   s.prepend("^(");
   s.append(")$");
   m_filter->setFilterRegExp(s);
} // QFilteredLogViewer::boxToggled
