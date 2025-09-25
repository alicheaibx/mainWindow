#include "mainwindow.h"
#include "dockmanager.h"
#include "layoutmanager.h"
#include "menumanager.h"
#include <QTextEdit>
#include <QFile>
#include <QMessageBox>
#include <QFileDialog>
#include <QCloseEvent>
#include <QShowEvent>
#include <QDebug>
#include <QTimer>
#include <QCoreApplication>
#include "InternalSpy.h"

MainWindow::MainWindow(QWidget *parent, Qt::WindowFlags flags)
    : QMainWindow(parent, flags)
{
    setObjectName("MainWindow");
    setWindowTitle("Qt Main Window Example");
    setDockNestingEnabled(true);

    setupCentralWidget();

    m_pInternalSpy = new CInternalSpy(this);
    m_pInternalSpy->hide();
    m_dockManager = new DockManager(this);
    m_layoutManager = new LayoutManager(this);
    m_menuManager = new MenuManager(this);

    connect(m_menuManager, &MenuManager::saveLayoutRequested, this, &MainWindow::saveLayout);
    connect(m_menuManager, &MenuManager::saveLayoutAsRequested, this, &MainWindow::saveLayoutAs);
    connect(m_menuManager, &MenuManager::loadLayoutRequested, this, &MainWindow::loadLayout);
    connect(m_menuManager, &MenuManager::layoutSelected, this, &MainWindow::loadSelectedLayout);
    connect(m_menuManager, &MenuManager::SpyClicked, this, &MainWindow::toggleSpy);
    connect(m_menuManager, &MenuManager::refreshLayoutRequested, this, &MainWindow::refreshLayout);

    connect(m_layoutManager, &LayoutManager::saveDockWidgetsLayoutRequested,
            m_dockManager, &DockManager::saveDockWidgetsLayout);
    connect(m_layoutManager, &LayoutManager::loadDockWidgetsLayoutRequested,
            m_dockManager, &DockManager::loadDockWidgetsLayout);

    connect(this, &MainWindow::shown, this, [this]() {
        QTimer::singleShot(100, this, [this]() {
            QFile layoutFile("layout.xml");
            if (layoutFile.exists()) {
                m_dockManager->setSizesFixed(true);
                m_layoutManager->loadLayoutFromFile("layout.xml");
            }
        });
    }, Qt::QueuedConnection);

    connect(m_menuManager, &MenuManager::allowResizeChanged, this, &MainWindow::onAllowResizeChanged);
    connect(m_layoutManager, &LayoutManager::allowResizeChanged, this, &MainWindow::onAllowResizeChanged);

    connect(qApp, &QCoreApplication::aboutToQuit, this, &MainWindow::saveLayout);
}

MainWindow::~MainWindow()
{
    delete m_dockManager;
    delete m_layoutManager;
    delete m_menuManager;
}

void MainWindow::onAllowResizeChanged(bool allowed)
{
    m_dockManager->setSizesFixed(!allowed);

    if (QWidget *central = centralWidget()) {
        if (allowed) {
            QSize currentSize = central->size();
            central->setMinimumSize(50, 50);
            central->setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
        } else {
            QSize currentSize = central->size();
            central->setMinimumSize(currentSize);
            central->setMaximumSize(currentSize);
        }
    }
}

void MainWindow::setupCentralWidget()
{
    auto* center = new QTextEdit(nullptr);
    center->setReadOnly(true);
    center->setMinimumSize(0, 0);
    center->setText(tr("This is the central widget.\n\n"
                       "You can dock other widgets around this area.\n"
                       "Use the View menu to toggle dock widgets.\n"
                       "Layouts can be saved and loaded from the File menu.\n"
                       "Use the dropdown above to switch between layouts."));
    setCentralWidget(center);
}

void MainWindow::saveLayout()
{
    m_layoutManager->saveLayoutToFile("layout.xml");
}

void MainWindow::saveLayoutAs()
{
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save Layout As"), "", tr("XML Files (*.xml)"));
    if (!fileName.isEmpty()) {
        m_layoutManager->saveLayoutToFile(fileName);
        QMessageBox::information(this, tr("Save Layout As"), tr("Layout saved to %1").arg(fileName));
    }
}

void MainWindow::loadLayout()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Load Layout"), "", tr("XML Files (*.xml)"));
    if (!fileName.isEmpty()) {
        m_dockManager->setSizesFixed(true);
        m_layoutManager->loadLayoutFromFile(fileName);
        QMessageBox::information(this, tr("Load Layout"), tr("Layout loaded from %1").arg(fileName));
    }
}

void MainWindow::loadSelectedLayout(const QString& filename)
{
    if (!filename.isEmpty() && QFile::exists(filename)) {
        m_dockManager->destroyDockWidgets();
        m_dockManager->setupDockWidgets();
        m_dockManager->setSizesFixed(true);
        m_layoutManager->loadLayoutFromFile(filename);
    } else {
        QMessageBox::warning(this, tr("Error"), tr("Selected layout file not found: %1").arg(filename));
    }
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    saveLayout();
    event->accept();
}

void MainWindow::showEvent(QShowEvent *event)
{
    QMainWindow::showEvent(event);
    emit shown();
}

void MainWindow::toggleSpy()
{
    m_pInternalSpy->show();
}

void MainWindow::refreshLayout()
{
    QString currentFile = m_menuManager->currentLayoutFile();
    if (!currentFile.isEmpty()) {
        loadSelectedLayout(currentFile);
    }
}
