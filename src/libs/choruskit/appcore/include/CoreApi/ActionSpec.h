#ifndef ACTIONSPEC_H
#define ACTIONSPEC_H

#include <QIcon>
#include <QKeySequence>
#include <QObject>

#include "CkAppCoreGlobal.h"

namespace Core {

    class ActionSpecPrivate;

    class CKAPPCORE_API ActionSpec : public QObject {
        Q_OBJECT
        Q_DECLARE_PRIVATE(ActionSpec)
    public:
        explicit ActionSpec(const QString &id, QObject *parent = nullptr);
        ~ActionSpec();

    public:
        QString id() const;

        QString commandName() const;
        void setCommandName(const QString &name);

        QString commandDisplayName() const;
        void setCommandDisplayName(const QString &displayName);

        QList<QKeySequence> shortcuts() const;
        void setShortcuts(const QList<QKeySequence> &shortcuts);

        QIcon icon() const;
        void setIcon(const QIcon &icon);

    signals:
        void shortcutsChanged(const QList<QKeySequence> &shortcuts);
        void iconChanged(const QIcon &icon);

    protected:
        ActionSpec(ActionSpecPrivate &d, const QString &id, QObject *parent = nullptr);

        QScopedPointer<ActionSpecPrivate> d_ptr;
    };

}

#endif // ACTIONSPEC_H