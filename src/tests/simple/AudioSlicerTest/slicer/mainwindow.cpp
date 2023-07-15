#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QIODevice>
#include <QMessageBox>
#include <QString>
#include <QStringList>
#include <QRegularExpression>
#include <QValidator>
#include <QThreadPool>
#include <QTextStream>
#include <QRunnable>
#include <QSysInfo>
#include <QDateTime>

#include "choruskit_buildinfo.h"

#include "mainwindow.h"
#include "mainwindow_ui.h"
#include "workthread.h"
#include "waveformat.h"


MainWindow::MainWindow(QWidget *parent) :
        QMainWindow(parent), ui(new Ui::MainWindow) {
    ui->setupUi(this);
    resize(1920, 1080);

    m_threadpool = new QThreadPool(this);
    m_threadpool->setMaxThreadCount(1);


    connect(ui->btnAddFiles, &QPushButton::clicked,
            this, &MainWindow::slot_addAudioFiles);
    connect(ui->btnBrowse, &QPushButton::clicked,
            this, &MainWindow::slot_browseOutputDir);
    connect(ui->btnClearList, &QPushButton::clicked,
            this, &MainWindow::slot_clearAudioList);
    connect(ui->pushButtonAbout, &QPushButton::clicked,
            this, &MainWindow::slot_about);
    connect(ui->pushButtonStart, &QPushButton::clicked,
            this, &MainWindow::slot_start);
    connect(ui->actionAddFile, &QAction::triggered,
            this, &MainWindow::slot_addAudioFiles);
    connect(ui->actionAddFolder, &QAction::triggered,
            this, &MainWindow::slot_addFolder);
    connect(ui->actionSaveLogs, &QAction::triggered,
            this, &MainWindow::slot_saveLogs);
    connect(ui->actionAboutApp, &QAction::triggered,
            this, &MainWindow::slot_about);
    connect(ui->actionAboutQt, &QAction::triggered,
            this, &MainWindow::slot_aboutQt);
    connect(ui->actionQuit, &QAction::triggered,
            this, &QCoreApplication::quit, Qt::QueuedConnection);

    ui->progressBar->setMinimum(0);
    ui->progressBar->setMaximum(100);
    ui->progressBar->setValue(0);

    auto validator = new QRegularExpressionValidator(QRegularExpression("\\d+"));
    ui->lineEditThreshold->setValidator(new QDoubleValidator());
    ui->lineEditMinLen->setValidator(validator);
    ui->lineEditMinInterval->setValidator(validator);
    ui->lineEditHopSize->setValidator(validator);
    ui->lineEditMaxSilence->setValidator(validator);

    ui->cmbOutputWaveFormat->addItem("16-bit integer PCM",
        QVariant::fromValue(static_cast<int>(WF_INT16_PCM)));
    ui->cmbOutputWaveFormat->addItem("24-bit integer PCM",
        QVariant::fromValue(static_cast<int>(WF_INT24_PCM)));
    ui->cmbOutputWaveFormat->addItem("32-bit integer PCM",
        QVariant::fromValue(static_cast<int>(WF_INT32_PCM)));
    ui->cmbOutputWaveFormat->addItem("32-bit float",
        QVariant::fromValue(static_cast<int>(WF_FLOAT32)));

    m_workTotal = 0;
    m_workFinished = 0;
    m_processing = false;

    setWindowTitle(QApplication::applicationName());
    setAcceptDrops(true);

#ifdef Q_OS_WIN
    m_hwnd = (HWND)this->winId();
    ::CoInitialize(nullptr);

    HRESULT hr = ::CoCreateInstance(CLSID_TaskbarList, nullptr, CLSCTX_INPROC_SERVER, IID_ITaskbarList3, (void**)&m_pTaskbarList3);

    if (hr != S_OK) {
        m_pTaskbarList3 = nullptr;
    }
#endif
}

MainWindow::~MainWindow() {
#ifdef Q_OS_WIN
    if (m_pTaskbarList3) {
        m_pTaskbarList3->SetProgressState(m_hwnd, TBPF_NOPROGRESS);
        m_pTaskbarList3->Release();
        m_pTaskbarList3 = nullptr;
    }
    ::CoUninitialize();
#endif
    delete ui;
}

void MainWindow::slot_browseOutputDir() {
    QString path = QFileDialog::getExistingDirectory(this, "Browse Output Directory", ".");
    if (!path.isEmpty()) {
        ui->lineEditOutputDir->setText(QDir::toNativeSeparators(path));
    }
}

void MainWindow::slot_addAudioFiles() {
    if (m_processing) {
        warningProcessNotFinished();
        return;
    }

    QStringList paths = QFileDialog::getOpenFileNames(this, "Select Audio Files", ".", "Wave Files (*.wav)");
    for (const QString& path : paths) {
        addSingleAudioFile(path);
    }
}

void MainWindow::slot_addFolder() {
    if (m_processing) {
        warningProcessNotFinished();
        return;
    }
    QString path = QFileDialog::getExistingDirectory(this, "Add Folder", ".");
    QDir dir(path);
    if (!dir.exists()) {
        return;
    }
    QStringList files = dir.entryList({"*.wav"}, QDir::Files);
    for (const QString &name : files) {
        QString fullPath = path + QDir::separator() + name;
        addSingleAudioFile(fullPath);
    }
}

void MainWindow::slot_clearAudioList() {
    if (m_processing) {
        warningProcessNotFinished();
        return;
    }

    ui->listWidgetTaskList->clear();
}

void MainWindow::slot_about() {
    QString arch = QSysInfo::currentCpuArchitecture();
    QString aboutMsg = QString(
        "<p><b>About Audio Slicer</b></p>"
        "<p>"
            "Arch: %1<br>"
            "Qt Version: %2<br>"
            "Git branch: %3<br>"
            "Git commit: %4<br>"
            "Build date: %5<br>"
            "Toolchain: %6"
        "</p>"
        "<p><b>Copyright 2019-%7 OpenVPI.</b></p>")
            .arg(arch,
                 QT_VERSION_STR,
                 CHORUSKIT_GIT_BRANCH,
                 QString(CHORUSKIT_GIT_COMMIT_HASH).left(7), 
                 CHORUSKIT_BUILD_DATE_TIME,
                 CHORUSKIT_BUILD_COMPILER_ID,
                 CHORUSKIT_BUILD_YEAR);

    QMessageBox::information(this, "About", aboutMsg);
}

void MainWindow::slot_aboutQt() {
    QMessageBox::aboutQt(this);
}

void MainWindow::slot_start() {
    if (m_processing) {
        warningProcessNotFinished();
        return;
    }

    int item_count = ui->listWidgetTaskList->count();
    if (item_count == 0) {
        return;
    }

    m_workFinished = 0;
    m_workError = 0;
    m_workTotal = item_count;

    ui->progressBar->setMinimum(0);
    ui->progressBar->setMaximum(item_count);
    ui->progressBar->setValue(0);

    ui->txtLogs->clear();

#ifdef Q_OS_WIN
    // Show taskbar progress (Windows 7 and later)
    if (m_pTaskbarList3) {
        m_pTaskbarList3->SetProgressState(m_hwnd, TBPF_NORMAL);
        m_pTaskbarList3->SetProgressValue(m_hwnd, (ULONGLONG)0, (ULONGLONG)m_workTotal);
    }
#endif

    setProcessing(true);
    for (int i = 0; i < item_count; i++) {
        auto item = ui->listWidgetTaskList->item(i);
        auto path = item->data(Qt::ItemDataRole::UserRole + 1).toString();
        auto runnable = new WorkThread(
                path,
                ui->lineEditOutputDir->text(),
                ui->lineEditThreshold->text().toDouble(),
                ui->lineEditMinLen->text().toULongLong(),
                ui->lineEditMinInterval->text().toULongLong(),
                ui->lineEditHopSize->text().toULongLong(),
                ui->lineEditMaxSilence->text().toULongLong(),
                ui->cmbOutputWaveFormat->currentData().toInt());
        connect(runnable, &WorkThread::oneFinished,
                this, &MainWindow::slot_oneFinished);
        connect(runnable, &WorkThread::oneInfo,
                this, &MainWindow::slot_oneInfo);
        connect(runnable, &WorkThread::oneError,
                this, &MainWindow::slot_oneError);
        m_threadpool->start(runnable);
    }
}

void MainWindow::slot_saveLogs() {
    QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss");
    QString filename = QString("slicer-%1.log").arg(timestamp);
    QString path = QFileDialog::getSaveFileName(this, "Save Log File", filename, "Log File (*.log)");
    if (path.isEmpty()) {
        return;
    }
    QFile writeFile(path);
    if (!writeFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, "Error", "Can't write output log file!");
    } else {
        QTextStream textStream(&writeFile);
        QString txt = ui->txtLogs->toPlainText();
        textStream << txt;
    }
    writeFile.close();
}

void MainWindow::slot_oneFinished(const QString &filename) {
    m_workFinished++;
    ui->progressBar->setValue(m_workFinished);
    logMessage(QString("%1 finished.").arg(filename));
#ifdef Q_OS_WIN
    if (m_pTaskbarList3) {
        m_pTaskbarList3->SetProgressState((HWND)this->winId(), TBPF_NORMAL);
        m_pTaskbarList3->SetProgressValue((HWND)this->winId(), (ULONGLONG)m_workFinished, (ULONGLONG)m_workTotal);
    }
#endif

    if (m_workFinished == m_workTotal) {
        slot_threadFinished();
    }
}

void MainWindow::slot_oneInfo(const QString &infomsg) {
    logMessage(infomsg);
}

void MainWindow::slot_oneError(const QString& errmsg) {
    m_workFinished++;
    m_workError++;
    logMessage(errmsg);
    ui->progressBar->setValue(m_workFinished);
#ifdef Q_OS_WIN
    if (m_pTaskbarList3) {
        m_pTaskbarList3->SetProgressState((HWND)this->winId(), TBPF_NORMAL);
        m_pTaskbarList3->SetProgressValue((HWND)this->winId(), (ULONGLONG)m_workFinished, (ULONGLONG)m_workTotal);
    }
#endif
    // QMessageBox::critical(this, "Error", errmsg);

    if (m_workFinished == m_workTotal) {
        slot_threadFinished();
    }
}

void MainWindow::slot_threadFinished() {
    setProcessing(false);
    auto msg = QString("Slicing complete! Total: %3, Success: %1, Failed: %2")
                      .arg(m_workTotal - m_workError)
                      .arg(m_workError)
                      .arg(m_workTotal);
    logMessage(msg);
#ifdef Q_OS_WIN
    if (m_pTaskbarList3) {
        m_pTaskbarList3->SetProgressState((HWND)this->winId(), TBPF_NOPROGRESS);
    }
#endif
    QMessageBox::information(
            this, QApplication::applicationName(),
            msg);
    m_workFinished = 0;
    m_workError = 0;
    m_workTotal = 0;
}

void MainWindow::warningProcessNotFinished() {
    QMessageBox::warning(this, QApplication::applicationName(), "Please wait for slicing to complete!");
}

void MainWindow::setProcessing(bool processing) {
    bool enabled = !processing;
    ui->pushButtonStart->setText(processing ? "Slicing..." : "Start");
    ui->pushButtonStart->setEnabled(enabled);
    ui->btnAddFiles->setEnabled(enabled);
    ui->listWidgetTaskList->setEnabled(enabled);
    ui->btnClearList->setEnabled(enabled);
    ui->lineEditThreshold->setEnabled(enabled);
    ui->lineEditMinLen->setEnabled(enabled);
    ui->lineEditMinInterval->setEnabled(enabled);
    ui->lineEditHopSize->setEnabled(enabled);
    ui->lineEditMaxSilence->setEnabled(enabled);
    ui->lineEditOutputDir->setEnabled(enabled);
    ui->btnBrowse->setEnabled(enabled);
    m_processing = processing;
}

void MainWindow::logMessage(const QString &txt) {
    if (!txt.isEmpty()) {
        auto timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");
        ui->txtLogs->append(QString("[%1] %2").arg(timestamp, txt));
    }
}

void MainWindow::addSingleAudioFile(const QString &fullPath) {
    auto *item = new QListWidgetItem();
    item->setText(QFileInfo(fullPath).fileName());
    item->setData(Qt::ItemDataRole::UserRole + 1, fullPath);
    ui->listWidgetTaskList->addItem(item);
}

void MainWindow::closeEvent(QCloseEvent *event) {
    if (m_processing) {
        warningProcessNotFinished();
        event->ignore();
    }
}

void MainWindow::dragEnterEvent(QDragEnterEvent *event) {
    auto urls = event->mimeData()->urls();
    bool has_wav = false;
    for (const auto &url: urls) {
        if (!url.isLocalFile()) {
            continue;
        }
        auto path = url.toLocalFile();
        auto ext = QFileInfo(path).suffix();
        if (ext.compare("wav", Qt::CaseInsensitive) == 0) {
            has_wav = true;
            break;
        }
    }
    if (has_wav) {
        event->accept();
    }
}

void MainWindow::dropEvent(QDropEvent *event) {
    auto urls = event->mimeData()->urls();
    for (const auto &url: urls) {
        if (!url.isLocalFile()) {
            continue;
        }
        auto path = url.toLocalFile();
        auto ext = QFileInfo(path).suffix();
        if (ext.compare("wav", Qt::CaseInsensitive) != 0) {
            continue;
        }

        auto *item = new QListWidgetItem();
        item->setText(QFileInfo(path).fileName());
        item->setData(Qt::ItemDataRole::UserRole + 1, path);
        ui->listWidgetTaskList->addItem(item);
    }
}
