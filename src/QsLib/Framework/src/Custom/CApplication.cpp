#include "CApplication.h"
#include "QMetaTypeImpl.h"

#include "Private/CApplication_p.h"

#include <QDateTime>
#include <QMouseEvent>
#include <QWidget>

#ifdef Q_QDOC
CApplication::CApplication(int m_argc, char **argv)
    : CApplication(*new CApplicationPrivate(), argc, argv) {
#else
CApplication::CApplication(int &argc, char **argv, int _internal)
    : CApplication(*new CApplicationPrivate(), argc, argv, _internal) {
#endif
}

CApplication::~CApplication() {
    removeAllNotifyFilters();
}

extern Q_DECL_IMPORT QWidget *qt_button_down;

QWidget *CApplication::implicitMouseGrabber() const {
    return qt_button_down;
}

QObjectList CApplication::notifyFilters() const {
    Q_D(const CApplication);
    QObjectList list;
    for (auto it = d->notifyFilters.begin(); it != d->notifyFilters.end(); ++it) {
        auto of = *it;
        list.append(of->obj);
    }
    return list;
}

bool CApplication::installNotifyFilter(CAppNotifyFilter *nf) {
    Q_D(CApplication);
    if (notifyFilters().contains(nf->obj)) {
        delete nf;
        return false;
    }
    d->notifyFilters.append(nf);
    return true;
}

bool CApplication::removeNotifyFilter(QObject *obj) {
    Q_D(CApplication);

    CAppNotifyFilter *nf = nullptr;
    for (auto it = d->notifyFilters.begin(); it != d->notifyFilters.end(); ++it) {
        auto cur = *it;
        if (cur->obj == obj) {
            nf = cur;
            d->notifyFilters.erase(it);
            break;
        }
    }
    if (nf) {
        delete nf;
        return true;
    }
    return false;
}

void CApplication::removeAllNotifyFilters() {
    Q_D(CApplication);
    for (auto it = d->notifyFilters.begin(); it != d->notifyFilters.end(); ++it) {
        delete *it;
        break;
    }
    d->notifyFilters.clear();
}

#ifdef Q_QDOC
CApplication::CApplication(CApplicationPrivate &d, int &argc, char **argv)
    : QApplication(argc, argv), d_ptr(&d) {
#else
CApplication::CApplication(CApplicationPrivate &d, int &argc, char **argv, int _internal)
    : QApplication(argc, argv, _internal), d_ptr(&d) {
#endif
    d.q_ptr = this;
    d.init();
}

bool CApplication::notify(QObject *obj, QEvent *event) {
    Q_D(CApplication);
    for (auto it = d->notifyFilters.begin(); it != d->notifyFilters.end(); ++it) {
        auto cur = *it;
        bool res = cur->notifyFilter(obj, event);
        if (res) {
            return true;
        }
    }
    return QApplication::notify(obj, event);
}
