#ifndef CTOOLBUTTON_H
#define CTOOLBUTTON_H

#include <QDebug>
#include <QToolButton>

#include "QMWidgetsGlobal.h"
#include "QSvgUri.h"

class CToolButtonPrivate;

class QMWIDGETS_EXPORT CToolButton : public QToolButton {
    Q_OBJECT

    /* Define all possible button icons at different state in qss init */
    Q_PROPERTY(QSvgUri iconUp READ iconUp WRITE setIconUp NOTIFY iconChanged)
    Q_PROPERTY(QSvgUri iconOver READ iconOver WRITE setIconOver NOTIFY iconChanged)
    Q_PROPERTY(QSvgUri iconDown READ iconDown WRITE setIconDown NOTIFY iconChanged)
    Q_PROPERTY(QSvgUri iconUpChecked READ iconUpChecked WRITE setIconUpChecked NOTIFY iconChanged)
    Q_PROPERTY(QSvgUri iconOverChecked READ iconOverChecked WRITE setIconOverChecked NOTIFY iconChanged)
    Q_PROPERTY(QSvgUri iconDownChecked READ iconDownChecked WRITE setIconDownChecked NOTIFY iconChanged)
    Q_PROPERTY(QSvgUri iconDisabled READ iconDisabled WRITE setIconDisabled NOTIFY iconChanged)

public:
    explicit CToolButton(QWidget *parent = nullptr);
    explicit CToolButton(const QString &text, QWidget *parent = nullptr);
    CToolButton(const QIcon &icon, const QString &text, QWidget *parent = nullptr);
    ~CToolButton();

public:
    QSvgUri iconUp() const;
    void setIconUp(const QSvgUri &iconUp);

    QSvgUri iconOver() const;
    void setIconOver(const QSvgUri &iconOver);

    QSvgUri iconDown() const;
    void setIconDown(const QSvgUri &iconDown);

    QSvgUri iconUpChecked() const;
    void setIconUpChecked(const QSvgUri &iconUpChecked);

    QSvgUri iconOverChecked() const;
    void setIconOverChecked(const QSvgUri &iconOverChecked);

    QSvgUri iconDownChecked() const;
    void setIconDownChecked(const QSvgUri &iconDownChecked);

    QSvgUri iconDisabled() const;
    void setIconDisabled(const QSvgUri &iconDisabled);

    bool autoCheck() const;
    void setAutoCheck(bool autoCheck);

protected:
    bool event(QEvent *event) override;

    void paintEvent(QPaintEvent *event) override;

    void nextCheckState() override;
    void checkStateSet() override;

    QColor currentTextColor(const QSize &hint = {}) const;

protected:
    CToolButton(CToolButtonPrivate &d, QWidget *parent = nullptr);
    QScopedPointer<CToolButtonPrivate> d_ptr;

    Q_DECLARE_PRIVATE(CToolButton);

signals:
    void iconChanged();
};

#endif // CTOOLBUTTON_H
