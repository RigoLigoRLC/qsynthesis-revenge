#ifndef PIANOROLLPRIVATE_H
#define PIANOROLLPRIVATE_H

#include <CDockFrame.h>

#include "PianoRoll.h"

#include "IPianoKeyWidget.h"
#include "Internal/Widgets/FloatingTitleBar.h"
#include "Internal/Widgets/PianoKeyContainer.h"

namespace Core {

    class PianoRollPrivate {
        Q_DECLARE_PUBLIC(PianoRoll)
    public:
        PianoRollPrivate();
        virtual ~PianoRollPrivate();

        void init();

        void readSettings();
        void saveSettings() const;

        PianoRoll *q_ptr;

        // Floating panels
        struct FloatingPanelControlBlock {
            QString id;
            QWidget *panel;
            Internal::FloatingTitleBar *titleBar;
            PianoRoll::FloatingPanelState state;
        };
        QHash<QString, FloatingPanelControlBlock> floatingPanels;

        // Piano key widgets
        QHash<QString, IPianoKeyWidgetFactory *> pianoKeyWidgets;
        QString currentPianoKeyWidget;


        // Layout
        QGridLayout *m_layout;

        Internal::PianoKeyContainer *m_pianoKeyContainer;
        QWidget *m_sectionContainer;
        QsApi::SynthVSplitter *m_canvas;
    };

}

#endif // PIANOROLLPRIVATE_H