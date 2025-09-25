#include "menumanager.h"
#include <QMenuBar>
#include <QAction>
#include <QMainWindow>
#include <QToolBar>
#include <QPushButton>
#include <QLabel>
#include <QCheckBox>
#include <QComboBox>
#include <QDir>
#include <QCoreApplication>

MenuManager::MenuManager(QMainWindow *parent)
    : QObject(parent),
    m_mainWindow(parent),
    m_layoutToolBar(nullptr),
    m_layoutComboBox(nullptr),
    m_layoutDirectory(QCoreApplication::applicationDirPath())
{
    setupMenuBar();
    setupLayoutToolBar();
}

void MenuManager::setupMenuBar()
{
    QMenu *fileMenu = m_mainWindow->menuBar()->addMenu(tr("&File"));

    QAction *saveLayoutAction = fileMenu->addAction(tr("Save Layout"));
    connect(saveLayoutAction, &QAction::triggered, this, &MenuManager::saveLayoutRequested);

    QAction *saveLayoutAsAction = fileMenu->addAction(tr("Save Layout As..."));
    connect(saveLayoutAsAction, &QAction::triggered, this, &MenuManager::saveLayoutAsRequested);

    QAction *loadLayoutAction = fileMenu->addAction(tr("Load Layout..."));
    connect(loadLayoutAction, &QAction::triggered, this, &MenuManager::loadLayoutRequested);

    fileMenu->addSeparator();
    fileMenu->addAction(tr("&Quit"), m_mainWindow, &QWidget::close);
}

void MenuManager::setupLayoutToolBar()
{
    m_layoutToolBar = new QToolBar(tr("Layouts"), m_mainWindow);
    m_layoutToolBar->setObjectName("LayoutToolBar");
    m_layoutToolBar->setMovable(false);
    m_layoutToolBar->setFloatable(false);
    m_layoutToolBar->setAllowedAreas(Qt::TopToolBarArea);
    m_mainWindow->addToolBar(Qt::TopToolBarArea, m_layoutToolBar);

    m_layoutToolBar->addWidget(new QLabel(tr("Layouts:")));
    m_layoutComboBox = new QComboBox(m_mainWindow);
    m_layoutComboBox->setMinimumWidth(150);
    refreshLayoutList();
    connect(m_layoutComboBox, QOverload<int>::of(&QComboBox::activated), this, [this](int index) {
        if (index >= 0) {
            QString filename = m_layoutComboBox->itemData(index).toString();
            emit layoutSelected(filename);
        }
    });
    m_layoutToolBar->addWidget(m_layoutComboBox);

    m_layoutToolBar->addSeparator();

    QPushButton *saveBtn = new QPushButton(tr("Save Current"), m_mainWindow);
    connect(saveBtn, &QPushButton::clicked, this, &MenuManager::saveLayoutAsRequested);
    m_layoutToolBar->addWidget(saveBtn);

    QPushButton *loadBtn = new QPushButton(tr("Load Custom"), m_mainWindow);
    connect(loadBtn, &QPushButton::clicked, this, &MenuManager::loadLayoutRequested);
    m_layoutToolBar->addWidget(loadBtn);

    QPushButton *refreshBtn = new QPushButton(tr("Refresh"), m_mainWindow);
    connect(refreshBtn, &QPushButton::clicked, this, &MenuManager::refreshLayoutRequested);
    m_layoutToolBar->addWidget(refreshBtn);

    QCheckBox *resizeCheckbox = new QCheckBox(tr("Allow Resize"), m_mainWindow);
    resizeCheckbox->setChecked(false);
    connect(resizeCheckbox, &QCheckBox::toggled, this, &MenuManager::allowResizeChanged);
    m_layoutToolBar->addWidget(resizeCheckbox);

    QPushButton *internalSpy = new QPushButton(tr("Internal Spy"), m_mainWindow);
    connect(internalSpy, &QPushButton::clicked, this, &MenuManager::SpyClicked);
    m_layoutToolBar->addWidget(internalSpy);
}

void MenuManager::refreshLayoutList()
{
    m_layoutComboBox->clear();

    QDir dir(m_layoutDirectory);
    QStringList filters;
    filters << "*.xml";
    QStringList files = dir.entryList(filters, QDir::Files, QDir::Name);

    for (const QString &file : files) {
        QString fullPath = dir.filePath(file);
        QFileInfo fileInfo(file);
        QString layoutName = fileInfo.baseName().replace("_", " ");
        m_layoutComboBox->addItem(layoutName, fullPath);
    }

    if (files.isEmpty()) {
        m_layoutComboBox->addItem(tr("No layouts found"));
        m_layoutComboBox->setEnabled(false);
    } else {
        m_layoutComboBox->setEnabled(true);
    }
}

QString MenuManager::currentLayoutFile() const
{
    if (m_layoutComboBox->currentIndex() >= 0) {
        return m_layoutComboBox->currentData().toString();
    }
    return QString();
}
