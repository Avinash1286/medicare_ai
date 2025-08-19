#ifndef MODERNWIDGETS_H
#define MODERNWIDGETS_H

#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QPropertyAnimation>
#include <QEnterEvent>

class ModernButton : public QPushButton
{
    Q_OBJECT
    Q_PROPERTY(qreal hoverProgress READ hoverProgress WRITE setHoverProgress)

public:
    explicit ModernButton(const QString &text, QWidget *parent = nullptr);
    void setButtonType(const QString &type);

protected:
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

private:
    qreal hoverProgress() const { return m_hoverProgress; }
    void setHoverProgress(qreal progress) { m_hoverProgress = progress; update(); }

    QPropertyAnimation *m_hoverAnimation;
    qreal m_hoverProgress;
    QString m_buttonType;
    bool m_isPressed;
};

class StatsCard : public QWidget
{
    Q_OBJECT

public:
    explicit StatsCard(const QString &title, const QString &icon, const QColor &color, QWidget *parent = nullptr);
    void updateValue(const QString &value);

private:
    QLabel *m_titleLabel;
    QLabel *m_valueLabel;
    QLabel *m_iconLabel;
};

#endif // MODERNWIDGETS_H
