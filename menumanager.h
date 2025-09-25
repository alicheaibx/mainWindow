// menumanager.h (updated)
#ifndef MENUMANAGER_H
#define MENUMANAGER_H

#include <QObject>
#include <QMainWindow>

class QToolBar;
class QPushButton;
class QComboBox;

class MenuManager : public QObject
{
    Q_OBJECT

public:
    explicit MenuManager(QMainWindow *parent = nullptr);
    QString currentLayoutFile() const;

private:
    void setupMenuBar();
    void setupLayoutToolBar();
    void refreshLayoutList();

signals:
    void saveLayoutRequested();
    void saveLayoutAsRequested();
    void loadLayoutRequested();
    void layoutSelected(const QString& filename);
    void allowResizeChanged(bool allowed);
    void SpyClicked();
    void refreshLayoutRequested();
private:
    QMainWindow *m_mainWindow;
    QToolBar *m_layoutToolBar;
    QComboBox *m_layoutComboBox;
    QString m_layoutDirectory;
};

#endif // MENUMANAGER_H
