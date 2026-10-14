#include "mainwindow.h"
#include "clockface.h"
#include "autostart.h"

#include <QVBoxLayout>
#include <QMenu>
#include <QActionGroup>
#include <QTimer>
#include <QTime>
#include <QDate>
#include <QLocale>
#include <QSettings>
#include <QFontDialog>
#include <QMessageBox>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QContextMenuEvent>
#include <QCloseEvent>
#include <QScreen>
#include <QApplication>
#include <QSystemTrayIcon>

MainWindow::MainWindow(QWidget *parent)
    : QWidget(parent, Qt::Window | Qt::FramelessWindowHint),
      m_face(new ClockFace(this)), m_timer(new QTimer(this))
{
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_MacAlwaysShowToolWindow);   // macOS 下 Tool 窗口失焦也不隐藏
    setWindowTitle(tr("时钟"));
    setWindowIcon(ClockFace::appIcon());

    auto *lay = new QVBoxLayout(this);
    lay->setContentsMargins(0, 0, 0, 0);
    lay->addWidget(m_face);

    m_timer->setSingleShot(true);
    m_timer->setTimerType(Qt::PreciseTimer);
    connect(m_timer, &QTimer::timeout, this, &MainWindow::tick);

    buildMenu();
    setupTray();
    loadSettings();
    tick();

    connect(qApp, &QApplication::aboutToQuit, this, &MainWindow::saveSettings);
}

// ---------------------------------------------------------------- 菜单

void MainWindow::buildMenu()
{
    m_menu = new QMenu(this);
    connect(m_menu, &QMenu::aboutToShow, this, [this] {
        m_actShowHide->setText(isVisible() ? tr("隐藏时钟") : tr("显示时钟"));
    });

    m_actShowHide = m_menu->addAction(tr("隐藏时钟"), this, &MainWindow::toggleVisible);
    m_menu->addSeparator();

    auto *modeGroup = new QActionGroup(this);
    m_actAnalog  = m_menu->addAction(tr("指针表盘"));
    m_actDigital = m_menu->addAction(tr("数字时钟"));
    for (QAction *a : {m_actAnalog, m_actDigital}) {
        a->setCheckable(true);
        modeGroup->addAction(a);
    }
    m_actAnalog->setChecked(true);
    connect(m_actAnalog,  &QAction::triggered, this, [this] { setMode(ClockFace::Analog); });
    connect(m_actDigital, &QAction::triggered, this, [this] { setMode(ClockFace::Digital); });

    QMenu *themeMenu = m_menu->addMenu(tr("主题"));
    auto *themeGroup = new QActionGroup(this);
    m_actDark  = themeMenu->addAction(tr("深色"));
    m_actLight = themeMenu->addAction(tr("浅色"));
    for (QAction *a : {m_actDark, m_actLight}) {
        a->setCheckable(true);
        themeGroup->addAction(a);
    }
    m_actDark->setChecked(true);
    connect(m_actDark,  &QAction::triggered, this, [this] { m_face->setTheme(ClockFace::Dark);  styleMenu(); });
    connect(m_actLight, &QAction::triggered, this, [this] { m_face->setTheme(ClockFace::Light); styleMenu(); });

    QMenu *opMenu = m_menu->addMenu(tr("不透明度"));
    auto *opGroup = new QActionGroup(this);
    for (int v : {100, 90, 80, 70, 60, 50}) {
        QAction *a = opMenu->addAction(QString("%1%").arg(v));
        a->setCheckable(true);
        a->setData(v);
        opGroup->addAction(a);
        m_opacityActs << a;
        connect(a, &QAction::triggered, this, [this, v] { setWindowOpacity(v / 100.0); });
    }
    m_opacityActs.first()->setChecked(true);

    m_menu->addSeparator();

    m_actSeconds = m_menu->addAction(tr("显示秒"));
    m_actSeconds->setCheckable(true);
    m_actSeconds->setChecked(true);
    connect(m_actSeconds, &QAction::toggled, this, [this](bool on) {
        m_face->setShowSeconds(on);
        m_actSmooth->setEnabled(on);
        tick();
    });

    m_actSmooth = m_menu->addAction(tr("平滑秒针"));
    m_actSmooth->setCheckable(true);
    m_actSmooth->setChecked(true);
    connect(m_actSmooth, &QAction::toggled, this, [this](bool on) {
        m_face->setSmoothSeconds(on);
        tick();
    });

    m_actDate = m_menu->addAction(tr("显示日期"));
    m_actDate->setCheckable(true);
    m_actDate->setChecked(true);
    connect(m_actDate, &QAction::toggled, m_face, &ClockFace::setShowDate);

    m_actOnTop = m_menu->addAction(tr("总在最前"));
    m_actOnTop->setCheckable(true);
    connect(m_actOnTop, &QAction::toggled, this, &MainWindow::applyOnTop);

    m_menu->addAction(tr("数字字体..."), this, &MainWindow::chooseFont);

    m_menu->addSeparator();

    m_actTaskbar = m_menu->addAction(tr("在任务栏显示"));
    m_actTaskbar->setCheckable(true);
    connect(m_actTaskbar, &QAction::toggled, this, &MainWindow::applyTaskbar);

    m_actAutostart = m_menu->addAction(tr("开机自启动"));
    m_actAutostart->setCheckable(true);
    m_actAutostart->setEnabled(AutoStart::isSupported());
    m_actAutostart->setChecked(AutoStart::isEnabled());
    connect(m_actAutostart, &QAction::triggered, this, [this](bool on) {
        if (!AutoStart::setEnabled(on)) {
            QMessageBox::warning(this, tr("开机自启动"), tr("设置开机自启动失败。"));
        }
        m_actAutostart->setChecked(AutoStart::isEnabled());
    });

    m_menu->addSeparator();
    m_menu->addAction(tr("关于"), this, &MainWindow::showAbout);
    QAction *quit = m_menu->addAction(tr("退出"), qApp, &QApplication::quit);
    quit->setShortcut(QKeySequence("Ctrl+Q"));
    addAction(quit);

    styleMenu();
}

// 圆角、跟随主题配色的菜单
void MainWindow::styleMenu()
{
    const bool dark = m_face->theme() == ClockFace::Dark;
    const QString qss = QString(R"(
        QMenu { background:%1; color:%2; border:1px solid %3; border-radius:10px; padding:6px; }
        QMenu::item { padding:6px 28px 6px 26px; border-radius:6px; }
        QMenu::item:selected { background:%4; color:white; }
        QMenu::item:disabled { color:%5; }
        QMenu::separator { height:1px; background:%3; margin:5px 10px; }
        QMenu::indicator { width:12px; height:12px; left:8px; }
    )").arg(dark ? "#22252d" : "#ffffff",
            dark ? "#e8eaee" : "#1f232b",
            dark ? "#3a3f4b" : "#dde0e6",
            dark ? "#ff5a36" : "#e8401c",
            dark ? "#5d6472" : "#a3a9b4");

    QList<QMenu *> menus = m_menu->findChildren<QMenu *>();
    menus << m_menu;
    for (QMenu *m : menus) {
        m->setWindowFlags(m->windowFlags() | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint);
        m->setAttribute(Qt::WA_TranslucentBackground);
        m->setStyleSheet(qss);
    }
}

// ---------------------------------------------------------------- 行为

void MainWindow::setMode(int mode)
{
    m_face->setMode(static_cast<ClockFace::Mode>(mode));
    (mode == ClockFace::Analog ? m_actAnalog : m_actDigital)->setChecked(true);
    applySize();
}

// 按模式固定宽高比，以窗口中心为基准调整大小
void MainWindow::applySize()
{
    const QPoint c = geometry().center();
    QSize s = (m_face->mode() == ClockFace::Analog)
            ? QSize(m_analogSide, m_analogSide)
            : QSize(m_digitalWidth, qRound(m_digitalWidth * 0.46));
    setFixedSize(s);
    if (isVisible())
        move(c - QPoint(s.width() / 2, s.height() / 2));
}

void MainWindow::applyOnTop(bool on)
{
    const bool vis = isVisible();
    setWindowFlag(Qt::WindowStaysOnTopHint, on);
    if (vis)
        show();
}

// 不在任务栏显示时用 Qt::Tool 窗口；没有托盘时必须保留任务栏入口
void MainWindow::applyTaskbar(bool showInTaskbar)
{
    const bool vis = isVisible();
    setWindowFlag(Qt::Tool, !showInTaskbar);
    if (vis)
        show();
}

void MainWindow::setupTray()
{
    if (!QSystemTrayIcon::isSystemTrayAvailable()) {
        m_actTaskbar->setChecked(true);
        m_actTaskbar->setEnabled(false);
        return;
    }
    m_tray = new QSystemTrayIcon(ClockFace::appIcon(), this);
    m_tray->setContextMenu(m_menu);
    connect(m_tray, &QSystemTrayIcon::activated, this, [this](QSystemTrayIcon::ActivationReason r) {
        if (r == QSystemTrayIcon::Trigger || r == QSystemTrayIcon::DoubleClick)
            toggleVisible();
    });
    m_tray->show();
}

void MainWindow::toggleVisible()
{
    if (isVisible()) {
        hide();
    } else {
        show();
        raise();
        activateWindow();
    }
}

void MainWindow::tick()
{
    const QTime now = QTime::currentTime();
    m_face->setTime(now);

    if (m_tray && now.minute() != m_lastMinute) {
        m_lastMinute = now.minute();
        m_tray->setToolTip(QLocale::system().toString(QDate::currentDate(), "yyyy-MM-dd dddd")
                           + "\n" + now.toString("HH:mm"));
    }

    int next;
    if (m_actSeconds->isChecked() && m_actSmooth->isChecked() && m_face->mode() == ClockFace::Analog)
        next = 33;                                              // ~30 fps 平滑秒针
    else if (m_actSeconds->isChecked())
        next = 1000 - now.msec() + 2;                           // 对齐整秒
    else
        next = (59 - now.second()) * 1000 + 1000 - now.msec() + 2;  // 对齐整分
    m_timer->start(next);
}

void MainWindow::chooseFont()
{
    bool ok = false;
    const QFont f = QFontDialog::getFont(&ok, m_face->digitalFont(), this, tr("数字时钟字体"));
    if (ok) {
        m_face->setDigitalFont(f);
        setMode(ClockFace::Digital);
    }
}

void MainWindow::showAbout()
{
    QMessageBox::about(this, tr("关于时钟"),
                       tr("<b>时钟</b> %2<br>基于 Qt %1<br><br>"
                          "左键拖动：移动窗口<br>"
                          "滚轮：缩放大小<br>"
                          "双击：切换指针 / 数字<br>"
                          "右键 / 托盘图标右键：设置菜单<br>"
                          "单击托盘图标：显示 / 隐藏<br>"
                          "Ctrl+Q：退出").arg(qVersion(), QApplication::applicationVersion()));
}

// ---------------------------------------------------------------- 鼠标

void MainWindow::mousePressEvent(QMouseEvent *e)
{
    if (e->button() == Qt::LeftButton)
        m_dragOffset = e->globalPosition().toPoint() - frameGeometry().topLeft();
}

void MainWindow::mouseMoveEvent(QMouseEvent *e)
{
    if (e->buttons() & Qt::LeftButton)
        move(e->globalPosition().toPoint() - m_dragOffset);
}

void MainWindow::mouseDoubleClickEvent(QMouseEvent *e)
{
    if (e->button() == Qt::LeftButton)
        setMode(m_face->mode() == ClockFace::Analog ? ClockFace::Digital : ClockFace::Analog);
}

void MainWindow::wheelEvent(QWheelEvent *e)
{
    const int dy = e->angleDelta().y();
    if (dy == 0)
        return;
    const qreal f = dy > 0 ? 1.08 : 1 / 1.08;
    if (m_face->mode() == ClockFace::Analog)
        m_analogSide = qBound(120, qRound(m_analogSide * f), 900);
    else
        m_digitalWidth = qBound(200, qRound(m_digitalWidth * f), 1400);
    applySize();
}

void MainWindow::contextMenuEvent(QContextMenuEvent *e)
{
    m_menu->exec(e->globalPos());
}

// ---------------------------------------------------------------- 设置

void MainWindow::loadSettings()
{
    QSettings s;
    m_analogSide   = s.value("analogSide", m_analogSide).toInt();
    m_digitalWidth = s.value("digitalWidth", m_digitalWidth).toInt();

    QFont f;
    if (f.fromString(s.value("font").toString()))
        m_face->setDigitalFont(f);

    if (s.value("light", false).toBool())
        m_actLight->trigger();
    m_actSeconds->setChecked(s.value("seconds", true).toBool());
    m_actSmooth->setChecked(s.value("smooth", true).toBool());
    m_actDate->setChecked(s.value("date", true).toBool());

    const int op = s.value("opacity", 100).toInt();
    for (QAction *a : m_opacityActs)
        if (a->data().toInt() == op)
            a->trigger();

    setMode(s.value("digital", false).toBool() ? ClockFace::Digital : ClockFace::Analog);

    // 默认放在主屏右上角
    const QRect avail = screen()->availableGeometry();
    QPoint pos = s.value("pos", QPoint(avail.right() - width() - 40, avail.top() + 40)).toPoint();
    move(pos);

    m_actOnTop->setChecked(s.value("onTop", false).toBool());
    if (m_tray)
        m_actTaskbar->setChecked(s.value("taskbar", false).toBool());
    applyTaskbar(m_actTaskbar->isChecked());

    if (s.value("visible", true).toBool() || !m_tray)
        show();
}

void MainWindow::saveSettings()
{
    QSettings s;
    s.setValue("pos", pos());
    s.setValue("analogSide", m_analogSide);
    s.setValue("digitalWidth", m_digitalWidth);
    s.setValue("digital", m_face->mode() == ClockFace::Digital);
    s.setValue("light", m_face->theme() == ClockFace::Light);
    s.setValue("seconds", m_actSeconds->isChecked());
    s.setValue("smooth", m_actSmooth->isChecked());
    s.setValue("date", m_actDate->isChecked());
    s.setValue("onTop", m_actOnTop->isChecked());
    s.setValue("opacity", qRound(windowOpacity() * 100));
    s.setValue("font", m_face->digitalFont().toString());
    s.setValue("taskbar", m_actTaskbar->isChecked());
    s.setValue("visible", isVisible());
}

// Alt+F4：有托盘时只隐藏，否则退出
void MainWindow::closeEvent(QCloseEvent *e)
{
    e->ignore();
    if (m_tray)
        hide();
    else
        qApp->quit();
}
