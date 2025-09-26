#include "dockmanager.h"
#include <QTextEdit>
#include <QAction>
#include <QMessageBox>
#include <QDebug>
#include <QTimer>
#include <QEvent>
#include <QApplication>
#include <QMainWindow>
#include <QSplitter>
#include <QTextEdit>
#include <QEvent>

DockWidgetEventFilter::DockWidgetEventFilter(ColorSwatch* dockWidget, QMainWindow* mainWindow)
    : QObject(mainWindow), m_dockWidget(dockWidget), m_mainWindow(mainWindow)
{
}

bool DockWidgetEventFilter::eventFilter(QObject *watched, QEvent *event)
{
    if (event->type() == QEvent::MouseButtonPress) {
        if (auto* centralWidget = qobject_cast<QTextEdit*>(m_mainWindow->centralWidget())) {
            QString info = QString("Object: %1\n").arg(m_dockWidget->objectName());
            info.append(QString("State: %1\n").arg(m_dockWidget->isFloating() ? "Floating" : "Docked"));
            info.append(QString("Geometry (x,y,w,h): %1, %2, %3, %4\n")
                            .arg(m_dockWidget->geometry().x())
                            .arg(m_dockWidget->geometry().y())
                            .arg(m_dockWidget->geometry().width())
                            .arg(m_dockWidget->geometry().height()));
            info.append(QString("Tabbed: %1\n").arg(m_mainWindow->tabifiedDockWidgets(m_dockWidget).isEmpty() ? "No" : "Yes"));
            if (m_dockWidget->parentWidget() && m_dockWidget->parentWidget()->inherits("QSplitter")) {
                QSplitter* splitter = static_cast<QSplitter*>(m_dockWidget->parentWidget());
                info.append(QString("In Splitter: Yes\n"));
                info.append(QString("Splitter Orientation: %1\n").arg(splitter->orientation() == Qt::Horizontal ? "Horizontal" : "Vertical"));
            } else {
                info.append(QString("In Splitter: No\n"));
            }
            centralWidget->setText(info);
        }
    }
    return QObject::eventFilter(watched, event);
}


void DockManager::destroyDockWidgets()
{
    for (ColorSwatch* swatch : m_dockWidgets) {
        m_mainWindow->removeDockWidget(swatch);
        delete swatch;
    }
    m_dockWidgets.clear();
    m_actionToDockWidgetMap.clear();
    m_dockWidgetSizes.clear();
    m_dockWidgetAreas.clear();
    m_viewMenu->clear();
}


DockManager::DockManager(QMainWindow *parent)
    : QObject(parent), m_mainWindow(parent), m_viewMenu(new QMenu(tr("&View"), parent))
{
    setupDockWidgets();
}

DockManager::~DockManager()
{
    qDeleteAll(m_dockWidgets);
}

ColorSwatch* DockManager::dockWidget(const QString &name) const
{
    for (ColorSwatch *swatch : m_dockWidgets) {
        if (swatch->objectName() == name)
            return swatch;
    }
    return nullptr;
}

ColorSwatch* DockManager::createColorSwatch(const QString &colorName, Qt::DockWidgetArea area)
{
    ColorSwatch *swatch = new ColorSwatch(colorName, m_mainWindow);
    swatch->setObjectName(colorName + "Dock");
    swatch->setMinimumSize(0, 0);
    swatch->setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);

    m_dockWidgetAreas[swatch] = area;
    m_mainWindow->addDockWidget(area, swatch);
    m_dockWidgets.append(swatch);

    connect(swatch, &QDockWidget::dockLocationChanged,
            this, &DockManager::handleDockLocationChanged);
    connect(swatch, &QDockWidget::topLevelChanged,
            this, [this, swatch](bool floating) {
                if (!floating) {
                    QTimer::singleShot(0, this, [this, swatch]() {
                        updateDockWidgetSizeConstraints(swatch);
                    });
                }
            });

    // Find the title bar widget and install the event filter
    QList<QWidget*> allChildren = swatch->findChildren<QWidget*>();
    for(QWidget* child : allChildren)
    {
        if(QString(child->metaObject()->className()) == "QDockWidgetTitleBar")
        {
            child->installEventFilter(new DockWidgetEventFilter(swatch, m_mainWindow));
            break;
        }
    }

    emit dockWidgetCreated(swatch);
    return swatch;
}

void DockManager::setupDockWidgets()
{
    static const struct {
        const char *name;
        Qt::DockWidgetArea area;
    } dockSettings[] = {
        {"Black", Qt::LeftDockWidgetArea},
        {"White", Qt::RightDockWidgetArea},
        {"Red", Qt::TopDockWidgetArea},
        {"Green", Qt::TopDockWidgetArea},
        {"Blue", Qt::BottomDockWidgetArea},
        {"Yellow", Qt::BottomDockWidgetArea}
    };

    for (const auto &setting : dockSettings) {
        ColorSwatch *swatch = createColorSwatch(setting.name, setting.area);

        QAction *action = m_viewMenu->addAction(setting.name);
        action->setCheckable(true);
        action->setChecked(true);
        m_actionToDockWidgetMap[action] = swatch;
        connect(action, &QAction::toggled, this, &DockManager::toggleDockWidgetVisibility);
    }
}

void DockManager::handleDockWidgetResized(ColorSwatch *swatch)
{
    Qt::DockWidgetArea area = m_dockWidgetAreas.value(swatch, Qt::NoDockWidgetArea);
    m_dockWidgetSizes[swatch] = swatch->frameGeometry().size();
    updateTabbedGroupSizes(swatch);
}

void DockManager::updateTabbedGroupSizes(ColorSwatch *swatch)
{
    QList<QDockWidget*> tabbedGroup = m_mainWindow->tabifiedDockWidgets(swatch);
    if (!tabbedGroup.isEmpty()) {
        m_blockResizeUpdates = true;
        for (QDockWidget *tabbedDock : tabbedGroup) {
            if (auto* tabbedSwatch = qobject_cast<ColorSwatch*>(tabbedDock)) {
                tabbedSwatch->resize(swatch->size());
                m_dockWidgetSizes[tabbedSwatch] = swatch->frameGeometry().size();
            }
        }
        m_blockResizeUpdates = false;
    }
}

void DockManager::toggleDockWidgetVisibility(bool checked)
{
    if (auto* action = qobject_cast<QAction*>(sender())) {
        if (m_actionToDockWidgetMap.contains(action)) {
            ColorSwatch *swatch = m_actionToDockWidgetMap[action];
            swatch->setVisible(checked);
            emit dockWidgetVisibilityChanged(swatch->objectName(), checked);
        }
    }
}

void DockManager::handleDockLocationChanged(Qt::DockWidgetArea area)
{
    if (auto* swatch = qobject_cast<ColorSwatch*>(sender())) {
        m_dockWidgetAreas[swatch] = area;
        updateDockWidgetSizeConstraints(swatch);
    }
}

void DockManager::updateDockWidgetSizeConstraints(ColorSwatch *swatch)
{
    if (!swatch) return;

    if (m_sizesFixed) {
        if (m_dockWidgetSizes.contains(swatch)) {
            QSize fixedSize = m_dockWidgetSizes[swatch];
            swatch->setFixedSize(fixedSize);
        }
    } else {
        swatch->setMinimumSize(0, 0);
        swatch->setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
        m_dockWidgetSizes[swatch] = swatch->size();
    }
}

void DockManager::saveDockWidgetsLayout(QXmlStreamWriter &xmlWriter)
{
    xmlWriter.writeStartElement("DockWidgets");
    xmlWriter.writeAttribute("state", m_mainWindow->saveState().toBase64());

    QSet<QDockWidget*> savedDockWidgets;
    for (ColorSwatch *dockWidget : m_dockWidgets) {
        if (savedDockWidgets.contains(dockWidget)) continue;

        xmlWriter.writeStartElement("DockWidget");
        xmlWriter.writeAttribute("name", dockWidget->objectName());

        saveWidgetProperties(xmlWriter, dockWidget->widget());

        QSize size = dockWidget->frameGeometry().size();
        xmlWriter.writeStartElement("Size");
        xmlWriter.writeAttribute("width", QString::number(size.width()));
        xmlWriter.writeAttribute("height", QString::number(size.height()));
        xmlWriter.writeEndElement();

        xmlWriter.writeTextElement("Title", dockWidget->windowTitle());
        xmlWriter.writeTextElement("Visible", dockWidget->isVisible() ? "true" : "false");
        xmlWriter.writeTextElement("Floating", dockWidget->isFloating() ? "true" : "false");
        xmlWriter.writeTextElement("Features", QString::number(static_cast<int>(dockWidget->features())));
        xmlWriter.writeTextElement("AllowedAreas", QString::number(static_cast<int>(dockWidget->allowedAreas())));

        if (dockWidget->isFloating()) {
            QRect geo = dockWidget->frameGeometry();
            xmlWriter.writeStartElement("Geometry");
            xmlWriter.writeAttribute("x", QString::number(geo.x()));
            xmlWriter.writeAttribute("y", QString::number(geo.y()));
            xmlWriter.writeAttribute("width", QString::number(geo.width()));
            xmlWriter.writeAttribute("height", QString::number(geo.height()));
            xmlWriter.writeEndElement();
        } else {
            QString areaStr;
            switch (m_mainWindow->dockWidgetArea(dockWidget)) {
            case Qt::LeftDockWidgetArea: areaStr = "Left"; break;
            case Qt::RightDockWidgetArea: areaStr = "Right"; break;
            case Qt::TopDockWidgetArea: areaStr = "Top"; break;
            case Qt::BottomDockWidgetArea: areaStr = "Bottom"; break;
            default: areaStr = "Floating";
            }
            xmlWriter.writeTextElement("DockArea", areaStr);
        }

        QList<QDockWidget*> tabbedGroup = m_mainWindow->tabifiedDockWidgets(dockWidget);
        if (!tabbedGroup.isEmpty()) {
            xmlWriter.writeStartElement("TabbedGroup");
            for (QDockWidget *tabbedDock : tabbedGroup) {
                xmlWriter.writeTextElement("DockWidget", tabbedDock->objectName());
                savedDockWidgets.insert(tabbedDock);
            }
            xmlWriter.writeEndElement();
        }

        xmlWriter.writeEndElement();
    }
    xmlWriter.writeEndElement();
}

void DockManager::saveWidgetProperties(QXmlStreamWriter &xmlWriter, QWidget *widget)
{
    if (!widget) return;

    xmlWriter.writeStartElement("WidgetProperties");
    xmlWriter.writeAttribute("objectName", widget->objectName());

    xmlWriter.writeStartElement("Geometry");
    xmlWriter.writeAttribute("x", QString::number(widget->geometry().x()));
    xmlWriter.writeAttribute("y", QString::number(widget->geometry().y()));
    xmlWriter.writeAttribute("width", QString::number(widget->geometry().width()));
    xmlWriter.writeAttribute("height", QString::number(widget->geometry().height()));
    xmlWriter.writeEndElement();

    xmlWriter.writeStartElement("MinimumSize");
    xmlWriter.writeAttribute("width", QString::number(widget->minimumSize().width()));
    xmlWriter.writeAttribute("height", QString::number(widget->minimumSize().height()));
    xmlWriter.writeEndElement();

    xmlWriter.writeStartElement("MaximumSize");
    xmlWriter.writeAttribute("width", QString::number(widget->maximumSize().width()));
    xmlWriter.writeAttribute("height", QString::number(widget->maximumSize().height()));
    xmlWriter.writeEndElement();

    if (auto* colorDock = qobject_cast<ColorDock*>(widget)) {
        xmlWriter.writeTextElement("Color", colorDock->property("colorName").toString());
    }

    xmlWriter.writeEndElement();
}

void DockManager::loadDockWidgetsLayout(QXmlStreamReader &xmlReader)
{
    qDebug() << "Loading dock widgets layout with fixed sizes...";
    m_sizesFixed = true;
    m_blockResizeUpdates = true;

    QMap<QString, ColorSwatch*> dockWidgetMap;
    for (ColorSwatch *dockWidget : m_dockWidgets) {
        dockWidgetMap[dockWidget->objectName()] = dockWidget;
    }

    QMap<ColorSwatch*, QList<ColorSwatch*>> tabbedGroups;

    if (xmlReader.attributes().hasAttribute("state")) {
        m_mainWindow->restoreState(QByteArray::fromBase64(xmlReader.attributes().value("state").toUtf8()));
    }

    while (xmlReader.readNextStartElement()) {
        if (xmlReader.name() == QLatin1String("DockWidget")) {
            QString name = xmlReader.attributes().value("name").toString();
            QString title;
            bool visible = true;
            bool floating = false;
            QSize size;
            QString dockAreaStr;
            QRect floatingGeo;
            QDockWidget::DockWidgetFeatures features = QDockWidget::DockWidgetClosable |
                                                       QDockWidget::DockWidgetMovable |
                                                       QDockWidget::DockWidgetFloatable;
            Qt::DockWidgetAreas allowedAreas = Qt::AllDockWidgetAreas;

            while (xmlReader.readNextStartElement()) {
                QString elementName = xmlReader.name().toString();
                if (elementName == QLatin1String("WidgetProperties")) {
                    if (dockWidgetMap.contains(name)) {
                        loadWidgetProperties(xmlReader, dockWidgetMap[name]->widget());
                    } else {
                        xmlReader.skipCurrentElement();
                    }
                } else if (elementName == QLatin1String("Title")) {
                    title = xmlReader.readElementText();
                } else if (elementName == QLatin1String("Visible")) {
                    visible = xmlReader.readElementText() == "true";
                } else if (elementName == QLatin1String("Floating")) {
                    floating = xmlReader.readElementText() == "true";
                } else if (elementName == QLatin1String("Features")) {
                    features = static_cast<QDockWidget::DockWidgetFeatures>(xmlReader.readElementText().toInt());
                } else if (elementName == QLatin1String("AllowedAreas")) {
                    allowedAreas = static_cast<Qt::DockWidgetAreas>(xmlReader.readElementText().toInt());
                } else if (elementName == QLatin1String("Size")) {
                    size.setWidth(xmlReader.attributes().value("width").toInt());
                    size.setHeight(xmlReader.attributes().value("height").toInt());
                    xmlReader.skipCurrentElement();
                } else if (elementName == QLatin1String("DockArea")) {
                    dockAreaStr = xmlReader.readElementText();
                } else if (elementName == QLatin1String("Geometry")) {
                    floatingGeo.setX(xmlReader.attributes().value("x").toInt());
                    floatingGeo.setY(xmlReader.attributes().value("y").toInt());
                    floatingGeo.setWidth(xmlReader.attributes().value("width").toInt());
                    floatingGeo.setHeight(xmlReader.attributes().value("height").toInt());
                    xmlReader.skipCurrentElement();
                } else if (elementName == QLatin1String("TabbedGroup")) {
                    QList<ColorSwatch*> tabbedGroup;
                    while (xmlReader.readNextStartElement()) {
                        if (xmlReader.name() == QLatin1String("DockWidget")) {
                            QString tabbedName = xmlReader.readElementText();
                            if (dockWidgetMap.contains(tabbedName)) {
                                tabbedGroup.append(dockWidgetMap[tabbedName]);
                            }
                        }
                    }
                    if (dockWidgetMap.contains(name)) {
                        tabbedGroups[dockWidgetMap[name]] = tabbedGroup;
                    }
                } else {
                    xmlReader.skipCurrentElement();
                }
            }

            if (dockWidgetMap.contains(name)) {
                ColorSwatch *dockWidget = dockWidgetMap[name];
                dockWidget->setWindowTitle(title);
                dockWidget->setVisible(visible);
                dockWidget->setFeatures(features);
                dockWidget->setAllowedAreas(allowedAreas);

                if (size.isValid()) {
                    dockWidget->setFixedSize(size);
                    m_dockWidgetSizes[dockWidget] = size;
                }

                if (floating) {
                    dockWidget->setFloating(true);
                    if (floatingGeo.isValid()) {
                        dockWidget->setGeometry(floatingGeo);
                    }
                } else {
                    Qt::DockWidgetArea area = Qt::LeftDockWidgetArea;
                    if (dockAreaStr == "Right") area = Qt::RightDockWidgetArea;
                    else if (dockAreaStr == "Top") area = Qt::TopDockWidgetArea;
                    else if (dockAreaStr == "Bottom") area = Qt::BottomDockWidgetArea;

                    m_mainWindow->addDockWidget(area, dockWidget);
                    m_dockWidgetAreas[dockWidget] = area;
                }
            }
        }
    }

    for (auto it = tabbedGroups.begin(); it != tabbedGroups.end(); ++it) {
        for (ColorSwatch *tabbedDock : it.value()) {
            m_mainWindow->tabifyDockWidget(it.key(), tabbedDock);
        }
    }


    m_blockResizeUpdates = false;
    enforceDockAreaSizeConstraints();
    qDebug() << "Dock widgets layout loaded with fixed sizes";
}

void DockManager::enforceDockAreaSizeConstraints()
{
    QMap<Qt::DockWidgetArea, QList<QDockWidget*>> areaDocks;
    for (ColorSwatch* swatch : m_dockWidgets) {
        if (!swatch->isFloating()) {
            areaDocks[m_mainWindow->dockWidgetArea(swatch)].append(swatch);
        }
    }

    int minCentralWidth = 100;
    int minCentralHeight = 100;

    QRect centralGeometry = m_mainWindow->centralWidget()->geometry();
    int availableWidth = m_mainWindow->width() - centralGeometry.left() - (m_mainWindow->width() - centralGeometry.right());
    int availableHeight = m_mainWindow->height() - centralGeometry.top() - (m_mainWindow->height() - centralGeometry.bottom());

    for (auto it = areaDocks.begin(); it != areaDocks.end(); ++it) {
        Qt::DockWidgetArea area = it.key();
        QList<QDockWidget*> docks = it.value();
        int totalSize = 0;

        if (area == Qt::LeftDockWidgetArea || area == Qt::RightDockWidgetArea) {
            for (QDockWidget* dock : docks) {
                totalSize += dock->width();
            }
            if (availableWidth - totalSize < minCentralWidth) {
                int newTotalSize = availableWidth - minCentralWidth;
                double scale = (double)newTotalSize / totalSize;
                for (QDockWidget* dock : docks) {
                    dock->setFixedWidth(dock->width() * scale);
                }
            }
        } else { // Top or Bottom
            for (QDockWidget* dock : docks) {
                totalSize += dock->height();
            }
            if (availableHeight - totalSize < minCentralHeight) {
                int newTotalSize = availableHeight - minCentralHeight;
                double scale = (double)newTotalSize / totalSize;
                for (QDockWidget* dock : docks) {
                    dock->setFixedHeight(dock->height() * scale);
                }
            }
        }
    }
}

void DockManager::loadWidgetProperties(QXmlStreamReader &xmlReader, QWidget *widget)
{
    if (!widget) return;

    QString objectName = xmlReader.attributes().value("objectName").toString();
    widget->setObjectName(objectName);

    while (xmlReader.readNextStartElement()) {
        QString elementName = xmlReader.name().toString();
        if (elementName == QLatin1String("Geometry")) {
            QRect geometry(
                xmlReader.attributes().value("x").toInt(),
                xmlReader.attributes().value("y").toInt(),
                xmlReader.attributes().value("width").toInt(),
                xmlReader.attributes().value("height").toInt()
            );
            widget->setGeometry(geometry);
            xmlReader.skipCurrentElement();
        } else if (elementName == QLatin1String("MinimumSize")) {
            widget->setMinimumSize(
                xmlReader.attributes().value("width").toInt(),
                xmlReader.attributes().value("height").toInt()
            );
            xmlReader.skipCurrentElement();
        } else if (elementName == QLatin1String("MaximumSize")) {
            widget->setMaximumSize(
                xmlReader.attributes().value("width").toInt(),
                xmlReader.attributes().value("height").toInt()
            );
            xmlReader.skipCurrentElement();
        } else if (elementName == QLatin1String("Color")) {
            if (auto* colorDock = qobject_cast<ColorDock*>(widget)) {
                colorDock->setProperty("colorName", xmlReader.readElementText());
            }
        } else {
            xmlReader.skipCurrentElement();
        }
    }
}

void DockManager::setSizesFixed(bool fixed)
{
    if (m_sizesFixed != fixed) {
        m_sizesFixed = fixed;
        for (ColorSwatch *dockWidget : m_dockWidgets) {
            updateDockWidgetSizeConstraints(dockWidget);
        }
    }
}

void DockManager::saveDockWidgetSize(ColorSwatch *swatch)
{
    if (swatch) {
        m_dockWidgetSizes[swatch] = swatch->frameGeometry().size();
        updateTabbedGroupSizes(swatch);
    }
}

QSize DockManager::savedDockWidgetSize(const QString &name) const
{
    if (ColorSwatch *swatch = dockWidget(name)) {
        return m_dockWidgetSizes.value(swatch, swatch->frameGeometry().size());
    }
    return QSize();
}
