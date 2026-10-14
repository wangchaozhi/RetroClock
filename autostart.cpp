#include "autostart.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSettings>
#include <QStandardPaths>

namespace {

const char *kAppId = "RetroClock";

// 可执行文件路径；Linux 下若以 AppImage 运行则使用 AppImage 本身
QString programPath()
{
#if defined(Q_OS_LINUX)
    const QByteArray appImage = qgetenv("APPIMAGE");
    if (!appImage.isEmpty())
        return QString::fromLocal8Bit(appImage);
#endif
    return QCoreApplication::applicationFilePath();
}

#if defined(Q_OS_MACOS)
QString plistPath()
{
    return QDir::homePath() + "/Library/LaunchAgents/com.retroclock.app.plist";
}
#elif defined(Q_OS_UNIX)
QString desktopPath()
{
    return QStandardPaths::writableLocation(QStandardPaths::ConfigLocation)
           + "/autostart/retroclock.desktop";
}
#endif

bool writeFile(const QString &path, const QByteArray &data)
{
    QDir().mkpath(QFileInfo(path).absolutePath());
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return false;
    return f.write(data) == data.size();
}

} // namespace

namespace AutoStart {

bool isSupported()
{
#if defined(Q_OS_WIN) || defined(Q_OS_MACOS) || defined(Q_OS_UNIX)
    return true;
#else
    return false;
#endif
}

bool isEnabled()
{
#if defined(Q_OS_WIN)
    QSettings run("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run",
                  QSettings::NativeFormat);
    return run.contains(kAppId);
#elif defined(Q_OS_MACOS)
    return QFile::exists(plistPath());
#elif defined(Q_OS_UNIX)
    return QFile::exists(desktopPath());
#else
    return false;
#endif
}

bool setEnabled(bool on)
{
    const QString exe = QDir::toNativeSeparators(programPath());

#if defined(Q_OS_WIN)
    QSettings run("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run",
                  QSettings::NativeFormat);
    if (on)
        run.setValue(kAppId, QString("\"%1\"").arg(exe));
    else
        run.remove(kAppId);
    run.sync();
    return run.status() == QSettings::NoError;

#elif defined(Q_OS_MACOS)
    if (!on)
        return !QFile::exists(plistPath()) || QFile::remove(plistPath());
    const QString plist = QString(
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        "<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" "
        "\"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\n"
        "<plist version=\"1.0\">\n<dict>\n"
        "  <key>Label</key><string>com.retroclock.app</string>\n"
        "  <key>ProgramArguments</key><array><string>%1</string></array>\n"
        "  <key>RunAtLoad</key><true/>\n"
        "</dict>\n</plist>\n").arg(exe.toHtmlEscaped());
    return writeFile(plistPath(), plist.toUtf8());

#elif defined(Q_OS_UNIX)
    if (!on)
        return !QFile::exists(desktopPath()) || QFile::remove(desktopPath());
    const QString desktop = QString(
        "[Desktop Entry]\n"
        "Type=Application\n"
        "Name=RetroClock\n"
        "Comment=Desktop clock\n"
        "Exec=\"%1\"\n"
        "Terminal=false\n"
        "X-GNOME-Autostart-enabled=true\n").arg(exe);
    return writeFile(desktopPath(), desktop.toUtf8());

#else
    Q_UNUSED(on);
    Q_UNUSED(exe);
    return false;
#endif
}

} // namespace AutoStart
