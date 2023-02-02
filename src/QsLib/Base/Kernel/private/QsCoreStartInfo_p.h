#include "../QsCoreStartInfo.h"

#include "../QsCoreConfig.h"
#include "../QsLocaleDir.h"

#include "singleapplication.h"

class QsCoreStartInfoPrivate {
    Q_DECLARE_PUBLIC(QsCoreStartInfo)
public:
    QsCoreStartInfo *q_ptr;

    // Decorator instance
    QsCoreDecorator *dec;

    // DistConfig instance
    QsCoreConfig *coreConfig;

    // SingleApplication hook
    SingleApplication *hSingleApp;

    bool isAboutToQuit;

    void init();

    void load_helper();

private:
    QsLocaleDir ld;
};
