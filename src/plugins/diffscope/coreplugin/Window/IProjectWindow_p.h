#ifndef IPROJECTWINDOW_P_H
#define IPROJECTWINDOW_P_H

#include <QToolBar>

#include <CoreApi/ActionContext.h>

#include "ICoreWindow_p.h"
#include "IProjectWindow.h"
#include "Internal/Widgets/FloatingTitleBar.h"
#include "Internal/Widgets/ProjectWidget.h"

namespace Core {

    class IProjectWindowPrivate : public ICoreWindowPrivate {
        Q_OBJECT
        Q_DECLARE_PUBLIC(IProjectWindow)
    public:
        IProjectWindowPrivate();

        void init();

        void readSettings();
        void saveSettings() const;

        void reloadStrings();

        void reloadMainToolbar();

        ActionContext *mainToolbarCtx;

        DspxDocument *m_doc;

        bool m_forceClose;

        Internal::ProjectWidget *m_projectWidget;

        // Floating panels
        struct FloatingPanelState {
            QString id;
            bool isOpen;
        };
        QList<FloatingPanelState> floatingPanelsState;

        struct FloatingPanelControlBlock {
            QString id;
            QWidget *panel;
            Internal::FloatingTitleBar *titleBar;
            QObject *obj;
        };
        QHash<QString, FloatingPanelControlBlock> floatingPanels;

        // Piano key widgets
        QHash<QString, IPianoKeyWidgetFactory *> pianoKeyWidgets;
        QString currentPianoKeyWidget;

        void setPianoKeyWidget(const QString &id);

    private:
        void _q_documentChanged();
        void _q_documentRaiseRequested();
        void _q_documentCloseRequested();
    };

}


#endif // IPROJECTWINDOW_P_H
