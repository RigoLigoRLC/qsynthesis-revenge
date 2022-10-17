#ifndef PIANOWINDOW_H
#define PIANOWINDOW_H

#include "Controls/Windows/NativeWindow.h"

class PianoWindowPrivate;

class PianoWindow : public NativeWindow {
    Q_OBJECT
    Q_DECLARE_PRIVATE(PianoWindow)
public:
    PianoWindow(QWidget *parent = nullptr);
    ~PianoWindow();

public:
    void reloadStrings();

protected:
    PianoWindow(PianoWindowPrivate &d, QWidget *parent = nullptr);

    QScopedPointer<PianoWindowPrivate> d_ptr;

    bool event(QEvent *event) override;

    void closeEvent(QCloseEvent *event) override;

private:
    void _q_actionTriggered(int actionId);
};

#endif // PIANOWINDOW_H
