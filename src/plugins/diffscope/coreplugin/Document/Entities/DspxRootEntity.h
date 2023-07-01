#ifndef CHORUSKIT_DSPXROOTENTITY_H
#define CHORUSKIT_DSPXROOTENTITY_H

#include "DspxBaseEntity.h"
#include "DspxTimelineEntity.h"
#include "DspxTrackEntity.h"

namespace Core {

    class DspxContentEntity;
    class DspxFileMetaEntity;
    class DspxMasterEntity;

    //===========================================================================
    // Content
    class CORE_EXPORT DspxContentEntity : public AceTreeEntityMapping {
        Q_OBJECT
    public:
        explicit DspxContentEntity(QObject *parent = nullptr);
        ~DspxContentEntity();

    public:
        DspxFileMetaEntity *metadata() const;
        DspxMasterEntity *master() const;
        DspxTimelineEntity *timeline() const;
        DspxTrackListEntity *tracks() const;
    };
    //===========================================================================

    //===========================================================================
    // FileMeta
    class CORE_EXPORT DspxFileMetaEntity : public AceTreeEntityMapping {
        Q_OBJECT
    public:
        explicit DspxFileMetaEntity(QObject *parent = nullptr);
        ~DspxFileMetaEntity();

    public:
        QString author() const;
        void setAuthor(const QString &author);

        QString projectName() const;
        void setProjectName(const QString &projectName);

    Q_SIGNALS:
        void authorChanged(const QString &author);
        void projectNameChanged(const QString &projectName);
    };
    //===========================================================================

    //===========================================================================
    // Master
    class CORE_EXPORT DspxMasterEntity : public AceTreeEntityMapping {
        Q_OBJECT
    public:
        explicit DspxMasterEntity(QObject *parent = nullptr);
        ~DspxMasterEntity();

    public:
        DspxBusControlEntity *control() const;
    };
    //===========================================================================

}

#endif // CHORUSKIT_DSPXROOTENTITY_H
