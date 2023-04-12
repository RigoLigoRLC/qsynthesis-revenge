#ifndef HOMEWINDOWADDON_H
#define HOMEWINDOWADDON_H

#include <QPushButton>

#include "CoreApi/ActionItem.h"
#include "CoreApi/IWindowAddOn.h"

namespace Core {

    namespace Internal {

        class HomeWindowAddOnFactory : public IWindowAddOnFactory {
        public:
            bool predicate(IWindow *handle) const override;
            IWindowAddOn *create(QObject *parent) override;
        };

        class HomeWindowAddOn : public IWindowAddOn {
            Q_OBJECT
        public:
            explicit HomeWindowAddOn(QObject *parent = nullptr);
            ~HomeWindowAddOn();

            void initialize() override;
            void extensionsInitialized() override;

        public:
            void reloadStrings();
            void initActions();

        private:
            ActionItem *fileItem;
            ActionItem *helpItem;

            ActionItem *openGroupItem;
            ActionItem *newFileItem;
            ActionItem *openFileItem;

            ActionItem *preferenceGroupItem;
            ActionItem *settingsItem;

            ActionItem *aboutGroupItem;
            ActionItem *aboutPluginsItem;
            ActionItem *aboutAppItem;
            ActionItem *aboutQtItem;

            QAbstractButton *recentWidgetButton;

        private:
            void _q_newButtonClicked();
            void _q_openButtonClicked();

        };

    }

}

#endif // HOMEWINDOWADDON_H
