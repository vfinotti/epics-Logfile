/* SPDX-License-Identifier: MIT */
#include "QLogList.h"
#include "QLogViewer.h"
#include "logger.h"

#include <QScrollBar>
#include <QTimer>

SuS::logfile::QLogViewer::QLogViewer(QWidget *_parent) : QTreeView(_parent) {
   m_loglist = new QLogList();
   setModel(m_loglist);
   setUniformRowHeights(true);
   setRootIsDecorated(false);

   setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);

   connect(verticalScrollBar(), SIGNAL(valueChanged(int)), this,
         SLOT(scrolled(int)));
   m_autoScroll = true;

   m_timer = new QTimer();
   connect(m_timer, SIGNAL(timeout()), this, SLOT(timeout()));
   // autoscroll four times a second
   m_timer->start(250);
} // QLogViewer constructor

void SuS::logfile::QLogViewer::scrolled(int _value) {
   // enable auto scrolling, when the user manually scrolled to the last
   // entry.
   m_autoScroll = (_value == verticalScrollBar()->maximum());
} // QLogViewer::scrolled

void SuS::logfile::QLogViewer::timeout() {
   if (!m_autoScroll) {
      return;
   } // if

   verticalScrollBar()->setValue(verticalScrollBar()->maximum());
} // QLogViewer::timeout
