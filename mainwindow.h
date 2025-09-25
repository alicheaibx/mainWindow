// mainwindow.h (updated)
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

class DockManager;
class LayoutManager;
class MenuManager;
class CInternalSpy;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr, Qt::WindowFlags flags = {});
    ~MainWindow();

signals:
    void shown();

public slots:
    void toggleSpy();

protected:
    void closeEvent(QCloseEvent *event) override;
    void showEvent(QShowEvent *event) override;

public slots:
    void saveLayout();
private slots:
    void saveLayoutAs();
    void loadLayout();
    void loadSelectedLayout(const QString& filename);
    void onAllowResizeChanged(bool allowed);
    void refreshLayout();

private:
    void setupCentralWidget();

    CInternalSpy* m_pInternalSpy;
    DockManager *m_dockManager;
    LayoutManager *m_layoutManager;
    MenuManager *m_menuManager;
};

#endif // MAINWINDOW_H
