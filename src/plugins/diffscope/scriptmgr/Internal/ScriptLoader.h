#ifndef CHORUSKIT_SCRIPTLOADER_H
#define CHORUSKIT_SCRIPTLOADER_H

#include <QJSEngine>

namespace ScriptMgr::Internal {

    class ScriptMgrPlugin;
    class LoaderInjectedObject;
    class JsInternalObject;

    struct ScriptEntry {
        enum Type { Script, ScriptSet };
        QString id;
        Type type;
        QStringList role;
        QString shortcut;
        QStringList childrenId;
        QStringList childrenShortcut;
    };

    class ScriptLoader: public QObject {
        Q_OBJECT
    public:
        static ScriptLoader *instance();

        bool reloadEngine();

        [[nodiscard]] QString createHandles(JsInternalObject *internalObject);

        void invoke(const QString &windowKey, const QString &id);
        void invoke(const QString &windowKey, const QString &id, int index);

        QString getName(const QString &id);
        QString getName(const QString &id, int index);

        QJSValue getInfo(const QString &id);

        void removeHandles(const QString &windowKey);

        QList<ScriptEntry> builtInScriptEntries() const;
        QList<ScriptEntry> scriptEntries() const;

        QString userScriptDir();

        QJSEngine *engine();
    signals:
        void engineWillReload();
        void engineReloaded();
    private:
        friend class ScriptMgrPlugin;
        friend class LoaderInjectedObject;

        ScriptLoader(QObject *parent = nullptr);
        ~ScriptLoader();

        static void alertJsUncaughtError(const QJSValue &error);
        static void warningCannotLoadFile(const QString &path);
        static void warningCannotGetName(const QString &id);
        static void criticalCannotInitializeEngine();
        static void criticalScriptExecutionFailed(const QString &name);

        struct EngineDeleter {
            static inline void cleanup(QJSEngine *pointer) { if (pointer) { pointer->setInterrupted(true); pointer->deleteLater(); } }
        };

        QScopedPointer<QJSEngine, EngineDeleter> m_engine;
        QJSValue m_jsCallbacks;
        QList<ScriptEntry> m_builtInScriptEntries;
        QList<ScriptEntry> m_scriptEntries;
    };

} // Internal

#endif // CHORUSKIT_SCRIPTLOADER_H
