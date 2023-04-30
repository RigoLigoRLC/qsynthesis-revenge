#ifndef IWINDOW_P_H
#define IWINDOW_P_H

#include <QHash>
#include <QSet>

#include "IWindow.h"
#include "IWindowAddOn.h"

namespace Core {

    class WindowSystemPrivate;

    class IWindowFactoryPrivate {
    public:
        QString id;
        IWindowFactory::AvailableCreator creator;
    };

    class CKAPPCORE_API IWindowPrivate : public QObject {
        Q_OBJECT
        Q_DECLARE_PUBLIC(IWindow)
    public:
        IWindowPrivate();
        ~IWindowPrivate();

        void init();

        void setWindow(QWidget *w, WindowSystemPrivate *d);

        IWindow *q_ptr;

        QString id;
        bool m_closed;

        QHash<QString, ActionItem *> actionItemMap;
        std::list<IWindowAddOn *> addOns;

        QHash<QString, QWidget *> widgetMap;

        struct DragFileHandler {
            QObject *obj;
            const char *member;
            int max;
        };
        QHash<QString, DragFileHandler> dragFileHandlerMap;

        void initAllAddOns();
        void deleteAllAddOns();

    private:
        void _q_windowClosed(QWidget *w);

        friend class WindowSystem;
    };
}



#endif // IWINDOW_P_H
