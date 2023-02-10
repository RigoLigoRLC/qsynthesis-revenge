#include "MidiPorter.h"

#include "QMidiFile.h"
#include "Utau/Utils/QUtaUtils.h"

#include "Dialogs/ImportDialog.h"
#include "QsConsole.h"
#include "QsLinq.h"
#include "QsStartInfo.h"
#include "QsSystem.h"

#include <QChar>
#include <QDebug>
#include <QFile>
#include <QTextCodec>

#include <set>

using namespace QDspx;

MidiPorter::MidiPorter(QObject *parent) : IDsPorter(parent) {
    dd.setDir(QsFs::PathFindDirPath(path));
    dd.loadDefault("MidiPorter");
}

MidiPorter::~MidiPorter() {
}

IDsPorter::SupportedFormat MidiPorter::format() const {
    return {tr("Standard MIDI Files"), "mid"};
};

bool MidiPorter::load(const QString &filename, QDspxModel *out, QObject *parent) {
    QMidiFile midi;

    if (!midi.load(filename)) {
        qCs->MsgBox(parent, QsConsole::Critical, qIStup->mainTitle(),
                    QObject::tr("Fail to open MIDI file!"));
        return false;
    }

    // midi种类、四分音符ticks数、轨道数、时间类型
    int midiFormat = midi.fileFormat();
    int resolution = midi.resolution();
    int tracksCount = midi.tracks().size();
    int divType = midi.divisionType();

    // 校验tracks数量、midi种类
    if (tracksCount == 0) {
        qCs->MsgBox(parent, QsConsole::Critical, qIStup->mainTitle(),
                    QObject::tr("No track found!"));
        return false;
    } else if (midiFormat == 0 && tracksCount > 1) {
        qCs->MsgBox(parent, QsConsole::Critical, qIStup->mainTitle(),
                    QObject::tr("The number of MIDI tracks is illegal!"));
        return false;
    } else if (midiFormat == 2 || divType != QMidiFile::PPQ) {
        qCs->MsgBox(parent, QsConsole::Critical, qIStup->mainTitle(),
                    QObject::tr("Unsupported MIDI file type!"));
        return false;
    }

    // 解析Tempo Map
    QVector<QPair<int, double>> tempos;
    QVector<QPair<int, QByteArray>> markers;

    QMap<int, QPoint> timeSign;
    timeSign[0] = QPoint(4, 4);

    QList<QMidiEvent *> tempMap = midi.eventsForTrack(0);

    for (auto e : qAsConst(tempMap)) {
        const auto &data = e->data();
        if (e->type() == QMidiEvent::Meta) {
            switch (e->number()) {
                case QMidiEvent::Tempo:
                    tempos.append(qMakePair(e->tick(), e->tempo()));
                    break;
                case QMidiEvent::Marker:
                    markers.append(qMakePair(e->tick(), data));
                    break;
                case QMidiEvent::TimeSignature:
                    timeSign[e->tick()] = QPoint(data[0], 2 * data[1]);
                    break;
                    break;
                default:
                    break;
            }
        }
    }

#pragma pack(1)
    struct LogicTrack {
        quint8 key;     // 0~16
        quint8 channel; // 16~24
        quint16 track;  // 24~32

        LogicTrack() : LogicTrack(0, 0, 0){};
        LogicTrack(quint16 track, quint8 channel, quint8 key) {
            this->track = track;
            this->channel = channel;
            this->key = key;
        };

        [[nodiscard]] qint32 toInt() const {
            return *reinterpret_cast<const qint32 *>(this);
        }

        static LogicTrack fromInt(qint32 n) {
            return *reinterpret_cast<LogicTrack *>(&n);
        }
    };
#pragma pack()

    Q_STATIC_ASSERT(sizeof(LogicTrack) == sizeof(qint32));

    struct TrackNameAndLyrics {
        QByteArray name;
        qint32 trackEnd;
        QMap<qint32, QByteArray> lyrics; // key: pos; value: lyric;
        TrackNameAndLyrics() : trackEnd(0){};
    };

    // key: pack(track, channel, 0); ordered
    std::set<qint32> trackAndChannelIndexSet;

    // key: track index; value: track name;
    QHash<qint32, TrackNameAndLyrics> trackNameAndLyrics;

    // key: pack(track, channel, key);  value: ...;
    QHash<qint32, QPair<QList<qint32>, QList<qint32>>> noteMap;

    // 解析元数据
    for (int i = midiFormat; i < tracksCount; ++i) {
        QList<QMidiEvent *> list = midi.eventsForTrack(i);
        qint32 trackIndex = i - midiFormat + 1;

        TrackNameAndLyrics cur;

        // 以track、channel、note为索引打包数据
        for (auto e : list) {
            // midi元事件
            switch (e->type()) {
                case QMidiEvent::Meta: {
                    switch (e->number()) {
                        case QMidiEvent::TrackName:
                            cur.name = e->data();
                            break;
                        case QMidiEvent::Lyric:
                            cur.lyrics.insert(e->tick(), e->data());
                            break;
                        // End of Track
                        case 0x2F:
                            cur.trackEnd = e->tick();
                            break;
                        default:
                            break;
                    }
                    break;
                }
                case QMidiEvent::NoteOn: {
                    // Add packed(track, channel, 0)
                    trackAndChannelIndexSet.insert(LogicTrack(trackIndex, e->voice(), 0).toInt());
                    // Add packed(track, channel, key)
                    noteMap[LogicTrack(trackIndex, e->voice(), e->note()).toInt()].first.append(
                        e->tick());
                    break;
                }
                case QMidiEvent::NoteOff: {
                    // Add packed(track, channel, key)
                    noteMap[LogicTrack(trackIndex, e->voice(), e->note()).toInt()].second.append(
                        e->tick());
                    break;
                }
                default:
                    break;
            }
        }

        // Add
        trackNameAndLyrics.insert(trackIndex, cur);
    }

    struct LogicTrackInfo {
        ImportDialog::TrackInfo option;
    };

    // 解析轨道
    // key: pack(track, channel, 0); value: ...;
    QMap<qint32, LogicTrackInfo> logicTrackInfos;

    struct LogicNote {
        int pos;
        int len;
        int key;
        QByteArray lyric;

        LogicNote() : LogicNote(0, 0, 0){};
        LogicNote(int pos, int len, int key) : pos(pos), len(len), key(key) {
        }

        bool operator<(const LogicNote &other) const {
            if (pos == other.pos) {
                return key < other.key;
            }
            return pos < other.pos;
        }
    };

    // key: pack(track, channel, 0); value: (pos, LogicNote)
    QMap<qint32, std::set<LogicNote>> logicTrackNotes;

    // 现存逻辑轨道
    for (const auto &packData : qAsConst(trackAndChannelIndexSet)) {
        LogicTrack trackAndChannelIndex = LogicTrack::fromInt(packData);

        qint32 noteCount = 0;
        std::set<qint32> staticKeyNum;

        auto &currentNoteSet = logicTrackNotes[packData];

        // 遍历keyNum
        for (int key = 0; key < 128; ++key) {
            LogicTrack tempIndex(trackAndChannelIndex.track, trackAndChannelIndex.channel, key);

            auto it = noteMap.find(tempIndex.toInt());
            if (it == noteMap.end()) {
                // 当前trackAndChannelIndex的当前key没有音符
                continue;
            }

            // 以track,channel,note为索引，取出noteMap
            const auto &noteListPair = it.value();

            // 校验各keyNum的NoteOn/Off事件配对
            if (noteListPair.first.size() != noteListPair.second.size()) {
                qCs->MsgBox(parent, QsConsole::Critical, qIStup->mainTitle(),
                            QObject::tr("The number of note-on and note-off are not match!"));
                return false;
            }

            // 存储出现的key
            staticKeyNum.insert(key);

            // 逻辑轨道封装Note
            for (int i = 0; i < noteListPair.first.size(); ++i) {
                noteCount++;

                LogicNote note;
                note.pos = noteListPair.first[i];
                note.len = noteListPair.second[i] - note.pos;
                note.key = key;

                {
                    const auto &lyricsMap = trackNameAndLyrics[trackAndChannelIndex.track].lyrics;
                    auto it2 = lyricsMap.find(note.pos);
                    if (it2 != lyricsMap.end()) {
                        note.lyric = it2.value();
                    }
                }

                currentNoteSet.insert(note);
            }
        }

        // 获取逻辑轨道音域
        QString logicTrackPitchRange;
        if (!staticKeyNum.empty()) {
            logicTrackPitchRange = QUtaUtils::ToneNumToToneName(*staticKeyNum.begin()) + "-" +
                                   QUtaUtils::ToneNumToToneName(*staticKeyNum.rbegin());
        }


        // 逻辑轨道名称
        auto nameBytes = trackNameAndLyrics[trackAndChannelIndex.track].name;
        ImportDialog::TrackInfo info(
            nameBytes, trackNameAndLyrics[trackAndChannelIndex.track].lyrics.values());

        info.format =
            QObject::tr("%1(%2): (%3 notes, %4)")
                .arg("%1", QString::number(trackAndChannelIndex.channel),
                     QString::number(noteCount),
                     logicTrackPitchRange.isEmpty() ? QObject::tr("None") : logicTrackPitchRange);

        logicTrackInfos.insert(packData, LogicTrackInfo{info});
    }


    QList<qint32> selectID;
    QTextCodec *codec = nullptr;

    // 获取选中轨道
    {
        ImportDialog dlg(qobject_cast<QWidget *>(parent));
        dlg.setWindowTitle(QObject::tr("Import MIDI file"));

        ImportDialog::ImportOptions opt;
        opt.minTracks = markers.isEmpty() ? 1 : 0;
        opt.maxTracks = 32;
        opt.labels = QsLinq::Select<QPair<qint32, QByteArray>, QByteArray>(
            markers, [&](const QPair<qint32, QByteArray> &pair) { return pair.second; });

        QList<qint32> logicIndexList;
        for (auto it = logicTrackInfos.begin(); it != logicTrackInfos.end(); ++it) {
            logicIndexList.append(it.key());
            opt.tracks.append(it.value().option);
        }
        if (logicIndexList.empty()) {
            qCs->MsgBox(parent, QsConsole::Warning, qIStup->mainTitle(),
                        QObject::tr("This file doesn't contain any notes."));
        }

        dlg.setImportOptions(opt);
        if (dlg.exec() == 0) {
            return false;
        }
        codec = dlg.codecResult();

        auto selectResult = dlg.selectResult();

        for (auto index : qAsConst(selectResult)) {
            selectID.append(logicIndexList[index]);
        }
    }

    // 缩放系数
    double scaleFactor = 480.0 / resolution;

    // TempoMap轨道数据
    QDspx::Timeline timeLine;

    TimeSignature timeSignature;
    for (auto it = timeSign.begin(); it != timeSign.end(); ++it) {
        timeSignature.pos = int(it.key() * scaleFactor);
        timeSignature.num = it->x();
        timeSignature.den = it->y();
        timeLine.timeSignatures.append(timeSignature);
    }

    QDspx::Tempo tempo;
    for (auto &it : tempos) {
        tempo.pos = int(it.first * scaleFactor);
        tempo.value = it.second;
        timeLine.tempos.append(tempo);
    }

    QDspx::Label label;
    for (auto &it : markers) {
        label.pos = int(it.first * scaleFactor);
        label.text = codec->toUnicode(it.second);
        timeLine.labels.append(label);
    }

    QDspxModel model;
    model.content.timeline = timeLine;

    // Track数据
    for (auto &logicID : selectID) {
        LogicTrack tempIndex = LogicTrack::fromInt(logicID);
        QDspx::Track track;
        auto clip = SingingClipRef::create();
        auto logicNotes = logicTrackNotes[logicID];

        int clipEnd = int(trackNameAndLyrics[tempIndex.track].trackEnd * scaleFactor);
        ClipTime clipTime(0, clipEnd);
        clip->time = clipTime;
        clip->name = codec->toUnicode(logicTrackInfos[logicID].option.title);

        // 填充音符
        for (const auto &logicNote : qAsConst(logicNotes)) {
            Note note;

            note.pos = int(logicNote.pos * scaleFactor);
            note.length = int(logicNote.len * scaleFactor);
            note.keyNum = logicNote.key;
            note.lyric = codec->toUnicode(logicNote.lyric);
            clip->notes.append(note);
        }

        // 填充track信息
        track.name = clip->name;
        track.clips.append(clip);
        model.content.tracks.append(track);
    }

    *out = std::move(model);

    return true;
}

bool MidiPorter::save(const QString &filename, const QDspxModel &in, QObject *parent) {
    return false;
}
