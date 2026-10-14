#include "clockface.h"

#include <QPainter>
#include <QPainterPath>
#include <QDate>
#include <QLocale>
#include <QtMath>
#include <QFontDatabase>
#include <QIcon>

// 各平台的界面字体
static QString uiFamily()
{
#if defined(Q_OS_WIN)
    return QStringLiteral("Segoe UI");
#elif defined(Q_OS_MACOS)
    return QStringLiteral("Helvetica Neue");
#else
    return QFontDatabase::systemFont(QFontDatabase::GeneralFont).family();
#endif
}

static QFont defaultDigitalFont()
{
    QFont f(uiFamily(), 48);
    f.setWeight(QFont::Light);
    return f;
}

ClockFace::ClockFace(QWidget *parent)
    : QWidget(parent), m_font(defaultDigitalFont()), m_time(QTime::currentTime())
{
    setAttribute(Qt::WA_TranslucentBackground);
}

void ClockFace::setMode(Mode m)  { m_mode = m;  rebuildCache(); update(); }
void ClockFace::setTheme(Theme t) { m_theme = t; rebuildCache(); update(); }
void ClockFace::setDigitalFont(const QFont &f) { m_font = f; update(); }

void ClockFace::resizeEvent(QResizeEvent *)
{
    rebuildCache();
}

ClockFace::Colors ClockFace::colors() const
{
    if (m_theme == Dark) {
        return {
            QColor(0x6b, 0x71, 0x7e), QColor(0x15, 0x17, 0x1c),   // 表圈
            QColor(0x2e, 0x33, 0x3e), QColor(0x12, 0x14, 0x19),   // 表盘
            QColor(0xf1, 0xf3, 0xf6), QColor(0x5d, 0x64, 0x72),   // 刻度
            QColor(0xc9, 0xcd, 0xd5),                             // 数字
            QColor(0xf4, 0xf5, 0xf7), QColor(0x7f, 0xe8, 0xc8),   // 指针 / 夜光
            QColor(0xff, 0x5a, 0x36),                             // 强调色（秒针）
            QColor(0x0b, 0x0c, 0x10), QColor(0xc9, 0xcd, 0xd5),
            QColor(0xf4, 0xf5, 0xf7), QColor(0x8a, 0x91, 0x9e),
        };
    }
    return {
        QColor(0xf4, 0xf5, 0xf7), QColor(0x9a, 0xa0, 0xab),
        QColor(0xff, 0xff, 0xff), QColor(0xe4, 0xe7, 0xec),
        QColor(0x1f, 0x23, 0x2b), QColor(0xa3, 0xa9, 0xb4),
        QColor(0x3a, 0x40, 0x4c),
        QColor(0x1f, 0x23, 0x2b), QColor(0x1f, 0x23, 0x2b),
        QColor(0xe8, 0x40, 0x1c),
        QColor(0xee, 0xf0, 0xf3), QColor(0x3a, 0x40, 0x4c),
        QColor(0x1f, 0x23, 0x2b), QColor(0x6b, 0x71, 0x7e),
    };
}

// 以路径方式绘制文字：透明背景上不会出现 ClearType 彩边
static void drawTextPath(QPainter &p, const QRectF &box, const QFont &f, const QString &text, const QColor &color)
{
    const QFontMetricsF fm(f);
    const QPointF base(box.center().x() - fm.horizontalAdvance(text) / 2,
                       box.center().y() + fm.capHeight() / 2);
    QPainterPath path;
    path.addText(base, f, text);
    p.save();
    p.setPen(Qt::NoPen);
    p.setBrush(color);
    p.drawPath(path);
    p.restore();
}

// ---------------------------------------------------------------- 缓存

void ClockFace::rebuildCache()
{
    if (width() <= 0 || height() <= 0)
        return;
    const qreal dpr = devicePixelRatioF();
    m_cache = QPixmap(size() * dpr);
    m_cache.setDevicePixelRatio(dpr);
    m_cache.fill(Qt::transparent);

    QPainter p(&m_cache);
    p.setRenderHint(QPainter::Antialiasing);
    p.setRenderHint(QPainter::TextAntialiasing);
    if (m_mode == Analog)
        paintAnalogStatic(p);
    else
        paintDigitalStatic(p);
}

void ClockFace::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.setRenderHint(QPainter::TextAntialiasing);
    p.drawPixmap(0, 0, m_cache);
    if (m_mode == Analog)
        paintAnalogDynamic(p);
    else
        paintDigitalDynamic(p);
}

// ---------------------------------------------------------------- 模拟表盘
// 逻辑坐标：表盘中心为原点，表圈外半径 = 100

static qreal faceRadius(const QWidget *w) { return qMin(w->width(), w->height()) / 2.0 * 0.88; }

void ClockFace::paintAnalogStatic(QPainter &p)
{
    const Colors c = colors();
    const QPointF center(width() / 2.0, height() / 2.0);
    const qreal R = faceRadius(this);
    const qreal shadowR = qMin(width(), height()) / 2.0;

    // 柔和投影
    {
        QRadialGradient g(center + QPointF(0, R * 0.03), shadowR);
        g.setColorAt(R / shadowR * 0.92, QColor(0, 0, 0, 120));
        g.setColorAt(1.0, QColor(0, 0, 0, 0));
        p.setPen(Qt::NoPen);
        p.setBrush(g);
        p.drawEllipse(center + QPointF(0, R * 0.03), shadowR, shadowR);
    }

    p.translate(center);
    p.scale(R / 100.0, R / 100.0);
    p.setPen(Qt::NoPen);

    // 金属表圈：外圈上亮下暗，内圈反向，形成斜面
    QLinearGradient bezel(0, -100, 0, 100);
    bezel.setColorAt(0, c.bezelTop);
    bezel.setColorAt(1, c.bezelBottom);
    p.setBrush(bezel);
    p.drawEllipse(QPointF(0, 0), 100, 100);

    QLinearGradient bezelIn(0, -94, 0, 94);
    bezelIn.setColorAt(0, c.bezelBottom);
    bezelIn.setColorAt(1, c.bezelTop);
    p.setBrush(bezelIn);
    p.drawEllipse(QPointF(0, 0), 94, 94);

    // 表盘
    QRadialGradient face(QPointF(0, -35), 135);
    face.setColorAt(0, c.faceCenter);
    face.setColorAt(1, c.faceEdge);
    p.setBrush(face);
    p.drawEllipse(QPointF(0, 0), 91, 91);

    // 刻度
    for (int i = 0; i < 60; ++i) {
        p.save();
        p.rotate(i * 6.0);
        if (i % 5 == 0) {
            const bool quarter = (i % 15 == 0);
            const qreal w = quarter ? 3.6 : 2.4;
            const qreal len = quarter ? 12 : 9;
            p.setBrush(c.tickMajor);
            p.drawRoundedRect(QRectF(-w / 2, -86, w, len), w / 2, w / 2);
        } else {
            p.setBrush(c.tickMinor);
            p.drawRoundedRect(QRectF(-0.5, -86, 1.0, 4), 0.5, 0.5);
        }
        p.restore();
    }

    // 数字
    QFont nf(uiFamily(), 1);
    nf.setPointSizeF(11);
    nf.setWeight(QFont::Light);
    for (int h = 1; h <= 12; ++h) {
        const qreal a = qDegreesToRadians(h * 30.0);
        const QPointF pos(qSin(a) * 64, -qCos(a) * 64);
        drawTextPath(p, QRectF(pos.x() - 12, pos.y() - 9, 24, 18), nf, QString::number(h), c.numeral);
    }
}

// 指针：先画偏移的阴影，再画本体
static void drawHand(QPainter &p, qreal angle, const QRectF &shape, const QBrush &brush,
                     const QRectF &lume = QRectF(), const QColor &lumeColor = QColor())
{
    const qreal r = shape.width() / 2;

    p.save();
    p.translate(1.2, 2.2);
    p.rotate(angle);
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(0, 0, 0, 70));
    p.drawRoundedRect(shape, r, r);
    p.restore();

    p.save();
    p.rotate(angle);
    p.setPen(Qt::NoPen);
    p.setBrush(brush);
    p.drawRoundedRect(shape, r, r);
    if (!lume.isNull()) {
        p.setBrush(lumeColor);
        p.drawRoundedRect(lume, lume.width() / 2, lume.width() / 2);
    }
    p.restore();
}

void ClockFace::paintAnalogDynamic(QPainter &p)
{
    const Colors c = colors();
    const qreal R = faceRadius(this);
    p.translate(width() / 2.0, height() / 2.0);
    p.scale(R / 100.0, R / 100.0);

    // 日期小窗
    if (m_showDate) {
        const QDate d = QDate::currentDate();
        const QString s = d.toString("MM-dd") + "  " + QLocale::system().dayName(d.dayOfWeek(), QLocale::ShortFormat);
        const QRectF box(-24, 30, 48, 14);
        p.setPen(QPen(c.tickMinor, 0.5));
        p.setBrush(c.dateBg);
        p.drawRoundedRect(box, 3, 3);
        QFont df(uiFamily(), 1);
        df.setPointSizeF(6.5);
        drawTextPath(p, box, df, s, c.dateText);
    }

    const qreal sec  = m_time.second() + (m_smooth ? m_time.msec() / 1000.0 : 0.0);
    const qreal min  = m_time.minute() + sec / 60.0;
    const qreal hour = (m_time.hour() % 12) + min / 60.0;

    const QColor lume = (m_theme == Dark) ? c.lume : QColor(0xff, 0xff, 0xff, 200);
    drawHand(p, hour * 30.0, QRectF(-3.2, -50, 6.4, 60), c.hand, QRectF(-1.2, -46, 2.4, 28), lume);
    drawHand(p, min * 6.0,  QRectF(-2.4, -80, 4.8, 92), c.hand, QRectF(-0.9, -76, 1.8, 44), lume);

    if (m_showSeconds) {
        p.save();
        p.translate(1.0, 2.0);
        p.rotate(sec * 6.0);
        p.setPen(QPen(QColor(0, 0, 0, 60), 1.2, Qt::SolidLine, Qt::RoundCap));
        p.drawLine(QPointF(0, 20), QPointF(0, -84));
        p.restore();

        p.save();
        p.rotate(sec * 6.0);
        p.setPen(QPen(c.accent, 1.2, Qt::SolidLine, Qt::RoundCap));
        p.drawLine(QPointF(0, 20), QPointF(0, -84));
        p.setPen(Qt::NoPen);
        p.setBrush(c.accent);
        p.drawEllipse(QPointF(0, 15), 3.2, 3.2);     // 配重
        p.restore();
    }

    // 中心轴
    p.setPen(Qt::NoPen);
    p.setBrush(m_showSeconds ? c.accent : c.hand);
    p.drawEllipse(QPointF(0, 0), 4.5, 4.5);
    p.setBrush(c.faceEdge);
    p.drawEllipse(QPointF(0, 0), 1.6, 1.6);

    // 玻璃高光
    QPainterPath glass;
    glass.addEllipse(QRectF(-80, -88, 160, 100));
    QPainterPath faceClip;
    faceClip.addEllipse(QPointF(0, 0), 91, 91);
    QLinearGradient gl(0, -88, 0, 12);
    gl.setColorAt(0, QColor(255, 255, 255, m_theme == Dark ? 26 : 60));
    gl.setColorAt(1, QColor(255, 255, 255, 0));
    p.setBrush(gl);
    p.drawPath(glass.intersected(faceClip));
}

// ---------------------------------------------------------------- 数字面板

QRectF ClockFace::panelRect() const
{
    const qreal m = qMin(width(), height()) * 0.10;
    return QRectF(rect()).adjusted(m, m * 0.8, -m, -m * 1.2);
}

void ClockFace::paintDigitalStatic(QPainter &p)
{
    const Colors c = colors();
    const QRectF r = panelRect();
    const qreal radius = r.height() * 0.18;

    // 层叠半透明圆角矩形模拟柔和阴影
    p.setPen(Qt::NoPen);
    const int layers = 16;
    const qreal spread = qMin(width(), height()) * 0.09;
    for (int i = layers; i >= 1; --i) {
        const qreal grow = spread * i / layers;
        p.setBrush(QColor(0, 0, 0, 5));
        p.drawRoundedRect(r.adjusted(-grow, -grow * 0.6, grow, grow * 1.2), radius + grow, radius + grow);
    }

    QLinearGradient border(r.topLeft(), r.bottomLeft());
    border.setColorAt(0, c.bezelTop);
    border.setColorAt(1, c.bezelBottom);
    p.setBrush(border);
    p.drawRoundedRect(r, radius, radius);

    const QRectF inner = r.adjusted(2, 2, -2, -2);
    QLinearGradient bg(inner.topLeft(), inner.bottomLeft());
    bg.setColorAt(0, c.faceCenter);
    bg.setColorAt(1, c.faceEdge);
    p.setBrush(bg);
    p.drawRoundedRect(inner, radius - 2, radius - 2);

    // 上半部玻璃反光
    QLinearGradient gl(inner.topLeft(), QPointF(inner.left(), inner.center().y()));
    gl.setColorAt(0, QColor(255, 255, 255, m_theme == Dark ? 18 : 50));
    gl.setColorAt(1, QColor(255, 255, 255, 0));
    p.setBrush(gl);
    p.drawRoundedRect(inner, radius - 2, radius - 2);
}

static QFont fitFont(QFont f, const QString &text, const QSizeF &box)
{
    f.setStyleStrategy(QFont::NoSubpixelAntialias);
    f.setPointSizeF(100);
    const QFontMetricsF fm(f);
    const qreal s = qMin(box.width() / fm.horizontalAdvance(text), box.height() / fm.capHeight());
    f.setPointSizeF(qMax(4.0, 100 * s));
    return f;
}

void ClockFace::paintDigitalDynamic(QPainter &p)
{
    const Colors c = colors();
    const QRectF r = panelRect();
    const QRectF content = r.adjusted(r.height() * 0.22, r.height() * 0.16, -r.height() * 0.22, -r.height() * 0.16);

    const QString hm = m_time.toString("HH:mm");
    const QString ss = m_showSeconds ? m_time.toString("ss") : QString();

    // 时间区占上部，日期在下
    const qreal timeH = content.height() * (m_showDate ? 0.62 : 0.80);
    QFont big = fitFont(m_font, hm + (ss.isEmpty() ? "" : " 00"), QSizeF(content.width(), timeH));
    QFont small = big;
    small.setPointSizeF(big.pointSizeF() * 0.42);
    const QFontMetricsF fmBig(big), fmSmall(small);

    const qreal wBig = fmBig.horizontalAdvance(hm);
    const qreal gap = fmBig.horizontalAdvance(' ') * 0.6;
    const qreal wSmall = ss.isEmpty() ? 0 : gap + fmSmall.horizontalAdvance(ss);
    const qreal x0 = content.center().x() - (wBig + wSmall) / 2;
    const qreal baseline = m_showDate
            ? content.top() + timeH * 0.5 + fmBig.capHeight() / 2
            : content.center().y() + fmBig.capHeight() / 2;

    p.setFont(big);
    p.setPen(c.textMain);
    p.drawText(QPointF(x0, baseline), hm);

    if (!ss.isEmpty()) {
        p.setFont(small);
        p.setPen(c.accent);
        p.drawText(QPointF(x0 + wBig + gap, baseline), ss);
    }

    if (m_showDate) {
        const QDate d = QDate::currentDate();
        const QString ds = QLocale::system().toString(d, "yyyy-MM-dd  dddd");
        QFont df(uiFamily(), 1);
        df = fitFont(df, ds, QSizeF(content.width() * 0.8, content.height() * 0.13));
        p.setFont(df);
        p.setPen(c.textSub);
        const QRectF dateBox(content.left(), content.top() + timeH, content.width(), content.height() - timeH);
        p.drawText(dateBox, Qt::AlignCenter, ds);
    }
}

// ---------------------------------------------------------------- 程序图标

QIcon ClockFace::appIcon()
{
    QIcon icon;
    for (int sz : {16, 24, 32, 48, 64, 128, 256, 512}) {
        QPixmap pm(sz, sz);
        pm.fill(Qt::transparent);
        QPainter p(&pm);
        p.setRenderHint(QPainter::Antialiasing);
        p.translate(sz / 2.0, sz / 2.0);
        p.scale(sz / 200.0, sz / 200.0);
        p.setPen(Qt::NoPen);

        QLinearGradient bezel(0, -96, 0, 96);
        bezel.setColorAt(0, QColor(0x8a, 0x90, 0x9c));
        bezel.setColorAt(1, QColor(0x15, 0x17, 0x1c));
        p.setBrush(bezel);
        p.drawEllipse(QPointF(0, 0), 96, 96);

        QRadialGradient face(QPointF(0, -30), 120);
        face.setColorAt(0, QColor(0x2e, 0x33, 0x3e));
        face.setColorAt(1, QColor(0x12, 0x14, 0x19));
        p.setBrush(face);
        p.drawEllipse(QPointF(0, 0), 84, 84);

        // 小尺寸只画四个主刻度，避免糊成一圈
        p.setBrush(QColor(0xf1, 0xf3, 0xf6));
        const int step = sz < 32 ? 3 : 1;
        for (int i = 0; i < 12; i += step) {
            p.save();
            p.rotate(i * 30);
            p.drawRoundedRect(QRectF(-4, -78, 8, i % 3 == 0 ? 18 : 12), 4, 4);
            p.restore();
        }

        auto hand = [&](qreal angle, qreal len, qreal w, const QColor &col) {
            p.save();
            p.rotate(angle);
            p.setBrush(col);
            p.drawRoundedRect(QRectF(-w / 2, -len, w, len + w), w / 2, w / 2);
            p.restore();
        };
        hand(-60, 46, 14, QColor(0xf4, 0xf5, 0xf7));    // 10 点
        hand(60, 66, 10, QColor(0xf4, 0xf5, 0xf7));     // 10 分
        hand(180, 70, 5, QColor(0xff, 0x5a, 0x36));
        p.setBrush(QColor(0xff, 0x5a, 0x36));
        p.drawEllipse(QPointF(0, 0), 10, 10);
        p.end();
        icon.addPixmap(pm);
    }
    return icon;
}
