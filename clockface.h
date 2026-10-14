#pragma once

#include <QWidget>
#include <QTime>
#include <QFont>
#include <QPixmap>
#include <QColor>

class QIcon;

// 表盘控件：透明背景，自绘阴影 / 表圈 / 指针，静态部分缓存为位图
class ClockFace : public QWidget
{
    Q_OBJECT
public:
    enum Mode { Analog, Digital };
    enum Theme { Dark, Light };

    explicit ClockFace(QWidget *parent = nullptr);

    void setMode(Mode m);
    Mode mode() const { return m_mode; }

    void setTheme(Theme t);
    Theme theme() const { return m_theme; }

    void setShowSeconds(bool on) { m_showSeconds = on; update(); }
    bool showSeconds() const { return m_showSeconds; }

    void setSmoothSeconds(bool on) { m_smooth = on; update(); }
    bool smoothSeconds() const { return m_smooth; }

    void setShowDate(bool on) { m_showDate = on; update(); }
    bool showDate() const { return m_showDate; }

    void setDigitalFont(const QFont &f);
    QFont digitalFont() const { return m_font; }

    void setTime(const QTime &t) { m_time = t; update(); }

    // 程序 / 托盘图标（运行时绘制）
    static QIcon appIcon();

protected:
    void paintEvent(QPaintEvent *) override;
    void resizeEvent(QResizeEvent *) override;

private:
    struct Colors {
        QColor bezelTop, bezelBottom, faceCenter, faceEdge;
        QColor tickMajor, tickMinor, numeral, hand, lume, accent;
        QColor dateBg, dateText, textMain, textSub;
    };
    Colors colors() const;

    void rebuildCache();
    void paintAnalogStatic(QPainter &p);
    void paintAnalogDynamic(QPainter &p);
    void paintDigitalStatic(QPainter &p);
    void paintDigitalDynamic(QPainter &p);
    QRectF panelRect() const;

    Mode m_mode = Analog;
    Theme m_theme = Dark;
    bool m_showSeconds = true;
    bool m_smooth = true;
    bool m_showDate = true;
    QFont m_font;
    QTime m_time;
    QPixmap m_cache;
};
