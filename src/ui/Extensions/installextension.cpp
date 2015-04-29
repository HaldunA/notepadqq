#include "include/Extensions/installextension.h"
#include "include/Extensions/extension.h"
#include "include/notepadqq.h"
#include "ui_installextension.h"
#include <QJsonObject>
#include <QJsonParseError>
#include <QProcess>
#include <QSettings>
#include <QDir>

namespace Extensions {

    InstallExtension::InstallExtension(const QString &extensionFilename, QWidget *parent) :
        QDialog(parent),
        ui(new Ui::InstallExtension),
        m_extensionFilename(extensionFilename)
    {
        ui->setupUi(this);

        QFont f = ui->lblName->font();
        f.setPointSizeF(f.pointSizeF() * 1.2);
        ui->lblName->setFont(f);

        //setFixedSize(this->width(), this->height());
        setWindowFlags((windowFlags() | Qt::CustomizeWindowHint) & ~Qt::WindowMinMaxButtonsHint);

        QString manifestStr = readExtensionManifest(extensionFilename);
        if (!manifestStr.isNull()) {
            QJsonParseError err;
            QJsonDocument manifestDoc = QJsonDocument::fromJson(manifestStr.toUtf8(), &err);

            if (err.error != QJsonParseError::NoError) {
                // FIXME Failed to load
            }

            QJsonObject manifest = manifestDoc.object();

            m_uniqueName = manifest.value("unique_name").toString();
            m_runtime = manifest.value("runtime").toString();
            ui->lblName->setText(manifest.value("name").toString());
            ui->lblVersionAuthor->setText(tr("Version %1, %2")
                                          .arg(manifest.value("version").toString(tr("unknown version")))
                                          .arg(manifest.value("author").toString(tr("unknown author"))));
            ui->lblDescription->setText(manifest.value("description").toString());

            // Tell the user if this is an update
            QString alreadyInstalledPath = getAbsoluteExtensionFolder(Notepadqq::extensionsPath(), m_uniqueName, false);
            if (!alreadyInstalledPath.isNull()) {
                QJsonObject manifest = Extension::getManifest(alreadyInstalledPath);
                if (!manifest.isEmpty()) {
                    QString currentVersion = manifest.value("version").toString(tr("unknown version"));
                    ui->lblVersionAuthor->setText(ui->lblVersionAuthor->text() + " " +
                                                  tr("(current version is %1)").arg(currentVersion));
                    ui->btnInstall->setText(tr("Update"));
                }
            }

        } else {
            // FIXME Error reading manifest from archive
        }
    }

    InstallExtension::~InstallExtension()
    {
        delete ui;
    }

    QString InstallExtension::getAbsoluteExtensionFolder(const QString &extensionsPath, const QString &extensionUniqueName, bool create)
    {
        if (extensionUniqueName.length() <= 3)
            return QString();

        QDir path(extensionsPath);

        QString escapedExtName = QString(extensionUniqueName).replace(QRegExp("[^a-zA-Z\\d._-]"), "_");

        Q_ASSERT(escapedExtName.contains(QRegExp("[^a-zA-Z\\d._-]")) == false);
        QString extPath = path.absoluteFilePath(escapedExtName);

        if (create && !path.mkpath(extPath))
            return QString();

        return extPath;
    }

    void InstallExtension::installRuby1_2Extension(const QString &extensionPath)
    {
        QProcess process;
        process.setWorkingDirectory(extensionPath);

        QSettings s;
        QString bundlerPath = s.value("Extensions/Runtime_Bundler", "").toString();

        process.start(bundlerPath, QStringList() << "install" << "--deployment");
        process.waitForFinished(-1);
    }

    void InstallExtension::extractExtension(const QString &archivePath, const QString &destination)
    {
        // FIXME Use a cross-platform library

        QProcess process;
        process.setWorkingDirectory(destination);
        process.start("tar", QStringList() << "--gzip" << "-xf" << archivePath);
        process.waitForFinished(-1);
    }

    QString InstallExtension::readExtensionManifest(const QString &archivePath)
    {
        // FIXME Use a cross-platform library

        QProcess process;
        QByteArray output;
        process.start("tar", QStringList() << "--gzip" << "-xOf" << archivePath << "manifest.json");

        if (process.waitForStarted(20000)) {
            while (process.waitForReadyRead(30000)) {
                output.append(process.readAllStandardOutput());
            }

            return QString(output);
        }

        return QString();
    }

}

void Extensions::InstallExtension::on_btnCancel_clicked()
{
    reject();
}

void Extensions::InstallExtension::on_btnInstall_clicked()
{
    // FIXME Show progress bar

    QString extFolder = getAbsoluteExtensionFolder(Notepadqq::extensionsPath(), m_uniqueName, true);
    if (!extFolder.isNull()) {
        QDir extFolderDir(extFolder);

        if (extFolderDir.exists()) {
            extFolderDir.removeRecursively();
            extFolderDir.mkpath(extFolder);

            extractExtension(m_extensionFilename, extFolder);

            if (m_runtime == "ruby") {
                installRuby1_2Extension(extFolder);

                accept();
            } else {
                // FIXME Error: unknown runtime
            }

        } else {
            // FIXME Error: unable to create extension folder in Notepadqq::extensionsPath()
        }
    } else {
        // FIXME Error: unable to create extension folder in Notepadqq::extensionsPath()
    }

}
