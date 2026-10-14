#pragma once

#include <QWidget>
#include <QPoint>

class QTimer;
class QAction;
class QMenu;
class QSystemTrayIcon;
class ClockFace;

// 无标题栏、透明背景的顶层窗口
class MainWindow : public QWidget
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);

protected:
    void closeEvent(QCloseEvent *e) override;
    void mousePressEvent(QMouseEvent *e) override;
    void mouseMoveEvent(QMouseEvent *e) override;
    void mouseDoubleClickEvent(QMouseEvent *e) override;
    void wheelEvent(QWheelEvent *e) override;
    void contextMenuEvent(QContextMenuEvent *e) override;

private slots:
    void tick();
    void chooseFont();
    void showAbout();
    void toggleVisible();
    void saveSettings();

private:
    void buildMenu();
    void styleMenu();
    void setMode(int mode);
    void applySize();
    void applyOnTop(bool on);
    void applyTaskbar(bool show);
    void setupTray();
    void loadSettings();

    ClockFace *m_face;
    QTimer *m_timer;
    QMenu *m_menu;
    QSystemTrayIcon *m_tray = nullptr;

    QAction *m_actAnalog, *m_actDigital;
    QAction *m_actDark, *m_actLight;
    QAction *m_actSeconds, *m_actSmooth, *m_actDate, *m_actOnTop;
    QAction *m_actShowHide, *m_actTaskbar, *m_actAutostart;
    QList<QAction *> m_opacityActs;

    int m_analogSide = 280;     // 模拟模式窗口边长
    int m_digitalWidth = 380;   // 数字模式窗口宽度
    QPoint m_dragOffset;
    int m_lastMinute = -1;      // 托盘提示每分钟更新一次
};
