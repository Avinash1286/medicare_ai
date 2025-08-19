#include "modernwidgets.h"
#include <QPainter>
#include <QPainterPath>
#include <QGraphicsDropShadowEffect>
#include <QHBoxLayout>
#include <QVBoxLayout>

// ModernButton Implementation
ModernButton::ModernButton(const QString &text, QWidget *parent)
    : QPushButton(text, parent), m_hoverProgress(0.0), m_buttonType("default"), m_isPressed(false)
{
    setFont(QFont("Segoe UI", 10, QFont::Bold));
    setCursor(Qt::PointingHandCursor);

    m_hoverAnimation = new QPropertyAnimation(this, "hoverProgress");
    m_hoverAnimation->setDuration(250);
    m_hoverAnimation->setEasingCurve(QEasingCurve::OutCubic);

    QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect(this);
    shadow->setBlurRadius(15);
    shadow->setColor(QColor(0, 0, 0, 40));
    shadow->setOffset(0, 2);
    setGraphicsEffect(shadow);
}

void ModernButton::setButtonType(const QString &type) { m_buttonType = type; update(); }
void ModernButton::enterEvent(QEnterEvent *event) { QPushButton::enterEvent(event); m_hoverAnimation->setDirection(QAbstractAnimation::Forward); m_hoverAnimation->start(); }
void ModernButton::leaveEvent(QEvent *event) { QPushButton::leaveEvent(event); m_hoverAnimation->setDirection(QAbstractAnimation::Backward); m_hoverAnimation->start(); }
void ModernButton::mousePressEvent(QMouseEvent *event) { QPushButton::mousePressEvent(event); m_isPressed = true; update(); }
void ModernButton::mouseReleaseEvent(QMouseEvent *event) { QPushButton::mouseReleaseEvent(event); m_isPressed = false; update(); }

void ModernButton::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    QRectF rect = this->rect();
    if (m_isPressed) rect.adjust(1, 1, -1, -1);

    QColor baseColor, hoverColor;
    if (m_buttonType == "primary") { baseColor = QColor("#48bb78"); hoverColor = QColor("#38a169"); }
    else if (m_buttonType == "danger") { baseColor = QColor("#f56565"); hoverColor = QColor("#e53e3e"); }
    else if (m_buttonType == "secondary") { baseColor = QColor("#4a5568"); hoverColor = QColor("#2d3748"); }
    else { baseColor = QColor("#667eea"); hoverColor = QColor("#5a67d8"); }

    QColor currentColor;
    currentColor.setRedF(baseColor.redF() + (hoverColor.redF() - baseColor.redF()) * m_hoverProgress);
    currentColor.setGreenF(baseColor.greenF() + (hoverColor.greenF() - baseColor.greenF()) * m_hoverProgress);
    currentColor.setBlueF(baseColor.blueF() + (hoverColor.blueF() - baseColor.blueF()) * m_hoverProgress);

    QLinearGradient gradient(0, 0, 0, rect.height());
    gradient.setColorAt(0, currentColor);
    gradient.setColorAt(1, currentColor.darker(110));

    QPainterPath path;
    path.addRoundedRect(rect, 8, 8);
    painter.fillPath(path, gradient);

    painter.setPen(Qt::white);
    painter.setFont(font());
    painter.drawText(rect, Qt::AlignCenter, text());
}


// StatsCard Implementation
StatsCard::StatsCard(const QString &title, const QString &icon, const QColor &color, QWidget *parent)
    : QWidget(parent)
{
    setMinimumSize(180, 80);
    setStyleSheet("background: transparent;");

    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setContentsMargins(15, 10, 15, 10);
    layout->setSpacing(10);

    m_iconLabel = new QLabel(icon);
    m_iconLabel->setStyleSheet(QString("color: %1; font-size: 24px;").arg(color.name()));
    m_iconLabel->setFixedSize(30, 40);
    m_iconLabel->setAlignment(Qt::AlignCenter);

    QVBoxLayout *textLayout = new QVBoxLayout();
    textLayout->setSpacing(0);
    m_valueLabel = new QLabel("0");
    m_valueLabel->setStyleSheet(QString("color: white; font-size: 20px; font-weight: bold;"));
    m_titleLabel = new QLabel(title);
    m_titleLabel->setStyleSheet("color: #a0aec0; font-size: 11px;");

    textLayout->addWidget(m_valueLabel);
    textLayout->addWidget(m_titleLabel);

    layout->addWidget(m_iconLabel);
    layout->addLayout(textLayout);
}

void StatsCard::updateValue(const QString &value) { m_valueLabel->setText(value); }
