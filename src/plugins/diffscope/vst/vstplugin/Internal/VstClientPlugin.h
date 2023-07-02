#ifndef VSTCLIENTPLUGIN_H
#define VSTCLIENTPLUGIN_H

#include <extensionsystem/iplugin.h>

namespace Vst::Internal {

    class VstClientPlugin : public ExtensionSystem::IPlugin {
        Q_OBJECT
        Q_PLUGIN_METADATA(IID "org.ChorusKit.DiffScope.Plugin" FILE "plugin.json")
    public:
        VstClientPlugin();
        ~VstClientPlugin();

        bool initialize(const QStringList &arguments, QString *errorMessage) override;
        void extensionsInitialized() override;
        bool delayedInitialize() override;

        QObject *remoteCommand(const QStringList &options, const QString &workingDirectory,
                               const QStringList &args) override;
    private:
        static void generateVstConfig();
    };

} // namespace Core


#endif // VSTCLIENTPLUGIN_H
