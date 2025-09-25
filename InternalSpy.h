#pragma once

/* INCLUDE FILES **************************************************************/
#include <QMainWindow>

class QTreeWidget;
class QTreeWidgetItem;
class QEvent;

class CInternalSpy : public QMainWindow
{
   Q_OBJECT
public:
   CInternalSpy(QWidget *parent = 0, Qt::WindowFlags flags = Qt::WindowFlags());
   ~CInternalSpy() override;
   bool eventFilter( QObject*, QEvent* ) override;
   void findItemForWidget( QWidget* pWidget );
   bool event( QEvent* ) override;
public slots:
   void slotUpdateObjectTree();
   void slotStartTimedUpdate();
   void slotDoTimedUpdate();
   void slotShowObject();
   void slotSetWlMode(int);
   void slotObjectClicked( QTreeWidgetItem*, int column );
   void slotObjectDoubleClicked( QTreeWidgetItem*, int column );
   void onItemChanged(QTreeWidgetItem* pItem, int nCol);
   void onLogAll();
   void slotFinishHighlighting();
   void slotStartSearchForWidget();
   void slotShowKeyMessages();
   void slotShowTheme();
private:
   class Data;
   Data* d;
};



/******************************************************************************/
/* END OF FILE                                                                */
/******************************************************************************/
