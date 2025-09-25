#include "layoutmanager.h"
#include <QXmlStreamWriter>
#include <QXmlStreamReader>
#include <QMainWindow>
#include <QMessageBox>
#include <QFile>
#include <QTextEdit>
#include <QVariant>
#include <QScreen>
#include <QGuiApplication>

LayoutManager::LayoutManager(QMainWindow *parent)
    : QObject(parent), m_mainWindow(parent)
{
}

void LayoutManager::saveLayoutToFile(const QString &fileName)
{
    QFile file(fileName);
    if (!file.open(QFile::WriteOnly | QFile::Text)) {
        QMessageBox::warning(m_mainWindow, tr("Error"), tr("Failed to open %1 for writing").arg(fileName));
        return;
    }

    QXmlStreamWriter xmlWriter(&file);
    xmlWriter.setAutoFormatting(true);
    xmlWriter.writeStartDocument();
    xmlWriter.writeStartElement("MainWindowLayout");

    xmlWriter.writeStartElement("ResizeSettings");
    xmlWriter.writeAttribute("allowResize",
                               m_mainWindow->centralWidget()->minimumSize() == m_mainWindow->centralWidget()->maximumSize() ? "false" : "true");
    xmlWriter.writeEndElement();

    saveMainWindowGeometry(xmlWriter);
    saveCentralWidgetProperties(xmlWriter);
    emit saveDockWidgetsLayoutRequested(xmlWriter);

    xmlWriter.writeEndElement(); // MainWindowLayout
    xmlWriter.writeEndDocument();
    file.close();
}

void LayoutManager::loadLayoutFromFile(const QString &fileName)
{
    QFile file(fileName);
    if (!file.open(QFile::ReadOnly | QFile::Text)) {
        QMessageBox::warning(m_mainWindow, tr("Error"), tr("Failed to open %1 for reading").arg(fileName));
        return;
    }

    QXmlStreamReader xmlReader(&file);
    while (xmlReader.readNextStartElement()) {
        if (xmlReader.name() == QLatin1String("MainWindowLayout")) {
            while (xmlReader.readNextStartElement()) {
                QStringView name = xmlReader.name();
                if (name == QLatin1String("ResizeSettings")) {
                    bool allowResize = (xmlReader.attributes().value("allowResize") == QLatin1String("true"));
                    emit allowResizeChanged(allowResize);
                    xmlReader.skipCurrentElement();
                } else if (name == QLatin1String("MainWindowGeometry")) {
                    loadMainWindowGeometry(xmlReader);
                } else if (name == QLatin1String("CentralWidget")) {
                    loadCentralWidgetProperties(xmlReader);
                } else if (name == QLatin1String("DockWidgets")) {
                    emit loadDockWidgetsLayoutRequested(xmlReader);
                } else {
                    xmlReader.skipCurrentElement();
                }
            }
        } else {
            xmlReader.skipCurrentElement();
        }
    }

    if (xmlReader.hasError()) {
        QMessageBox::warning(m_mainWindow, tr("Error"), tr("Failed to parse XML file: %1").arg(xmlReader.errorString()));
    }

    file.close();
}

void LayoutManager::saveMainWindowGeometry(QXmlStreamWriter &xmlWriter)
{
    xmlWriter.writeStartElement("MainWindowGeometry");
    QRect geometry = m_mainWindow->geometry();
    xmlWriter.writeAttribute("x", QString::number(geometry.x()));
    xmlWriter.writeAttribute("y", QString::number(geometry.y()));
    xmlWriter.writeAttribute("width", QString::number(geometry.width()));
    xmlWriter.writeAttribute("height", QString::number(geometry.height()));
    xmlWriter.writeAttribute("nestedDocking", m_mainWindow->isDockNestingEnabled() ? "true" : "false");
    xmlWriter.writeAttribute("groupMovement", (m_mainWindow->dockOptions() & QMainWindow::AllowNestedDocks) ? "true" : "false");
    xmlWriter.writeEndElement(); // MainWindowGeometry
}

void LayoutManager::loadMainWindowGeometry(QXmlStreamReader &xmlReader)
{
    int x = xmlReader.attributes().value("x").toInt();
    int y = xmlReader.attributes().value("y").toInt();
    int width = xmlReader.attributes().value("width").toInt();
    int height = xmlReader.attributes().value("height").toInt();
    bool nestedDocking = (xmlReader.attributes().value("nestedDocking") == QLatin1String("true"));
    bool groupMovement = (xmlReader.attributes().value("groupMovement") == QLatin1String("true"));

    QRect screenGeometry = QGuiApplication::primaryScreen()->availableGeometry();
    if (width > screenGeometry.width()) {
        width = screenGeometry.width();
    }
    if (height > screenGeometry.height()) {
        height = screenGeometry.height();
    }
    if (x < screenGeometry.x()) {
        x = screenGeometry.x();
    }
    if (y < screenGeometry.y()) {
        y = screenGeometry.y();
    }

    m_mainWindow->setGeometry(x, y, width, height);
    m_mainWindow->setDockNestingEnabled(nestedDocking);

    QMainWindow::DockOptions options = m_mainWindow->dockOptions();
    if (groupMovement) {
        options |= QMainWindow::AllowNestedDocks;
    } else {
        options &= ~QMainWindow::AllowNestedDocks;
    }
    m_mainWindow->setDockOptions(options);
    xmlReader.skipCurrentElement();
}

void LayoutManager::saveCentralWidgetProperties(QXmlStreamWriter &xmlWriter)
{
    QWidget *central = m_mainWindow->centralWidget();
    if (!central) return;

    xmlWriter.writeStartElement("CentralWidget");
    xmlWriter.writeAttribute("objectName", central->objectName());

    xmlWriter.writeStartElement("Geometry");
    xmlWriter.writeAttribute("x", QString::number(central->geometry().x()));
    xmlWriter.writeAttribute("y", QString::number(central->geometry().y()));
    xmlWriter.writeAttribute("width", QString::number(central->geometry().width()));
    xmlWriter.writeAttribute("height", QString::number(central->geometry().height()));
    xmlWriter.writeEndElement();

    xmlWriter.writeStartElement("MinimumSize");
    xmlWriter.writeAttribute("width", QString::number(central->minimumSize().width()));
    xmlWriter.writeAttribute("height", QString::number(central->minimumSize().height()));
    xmlWriter.writeEndElement();

    xmlWriter.writeStartElement("MaximumSize");
    xmlWriter.writeAttribute("width", QString::number(central->maximumSize().width()));
    xmlWriter.writeAttribute("height", QString::number(central->maximumSize().height()));
    xmlWriter.writeEndElement();

    if (auto* textEdit = qobject_cast<QTextEdit*>(central)) {
        xmlWriter.writeTextElement("Text", textEdit->toPlainText());
        xmlWriter.writeAttribute("readOnly", textEdit->isReadOnly() ? "true" : "false");
    }

    xmlWriter.writeEndElement(); // CentralWidget
}

void LayoutManager::loadCentralWidgetProperties(QXmlStreamReader &xmlReader)
{
    QWidget *central = m_mainWindow->centralWidget();
    if (!central) return;

    central->setObjectName(xmlReader.attributes().value("objectName").toString());

    while (xmlReader.readNextStartElement()) {
        QStringView name = xmlReader.name();
        if (name == QLatin1String("Geometry")) {
            QRect geometry(
                xmlReader.attributes().value("x").toInt(),
                xmlReader.attributes().value("y").toInt(),
                xmlReader.attributes().value("width").toInt(),
                xmlReader.attributes().value("height").toInt()
            );
            central->setGeometry(geometry);
            central->resize(geometry.size());
            xmlReader.skipCurrentElement();
        } else if (name == QLatin1String("MinimumSize")) {
            central->setMinimumSize(
                xmlReader.attributes().value("width").toInt(),
                xmlReader.attributes().value("height").toInt()
            );
            xmlReader.skipCurrentElement();
        } else if (name == QLatin1String("MaximumSize")) {
            central->setMaximumSize(
                xmlReader.attributes().value("width").toInt(),
                xmlReader.attributes().value("height").toInt()
            );
            xmlReader.skipCurrentElement();
        } else if (name == QLatin1String("Text")) {
            if (auto* textEdit = qobject_cast<QTextEdit*>(central)) {
                textEdit->setPlainText(xmlReader.readElementText());
                textEdit->setReadOnly(xmlReader.attributes().value("readOnly") == QLatin1String("true"));
            }
        } else {
            xmlReader.skipCurrentElement();
        }
    }
}
