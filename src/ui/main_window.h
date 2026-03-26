#pragma once

#include "core/usb.h"
#include "core/vault.h"
#include "ui/widgets.h"
#include "ui/file_explorer.h"
#include <QMainWindow>
#include <QStackedWidget>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QThread>

namespace bunker::ui {

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

private slots:
    void refreshDrives();
    void beginEncrypt();
    void beginDecrypt();
    void submitPassword();
    void onProgress(int percent, const QString& status);
    void onFinished(bool ok, const QString& message);
    void backToDrive();
    void reset();

private:
    enum Page {
        Idle     = 0,
        Drive    = 1,
        Password = 2,
        Progress = 3,
        Complete = 4
    };

    void buildUi();
    void buildIdlePage();
    void buildDrivePage();
    void buildPasswordPage();
    void buildProgressPage();
    void buildCompletePage();
    void switchTo(Page page);

    QStackedWidget* pages = nullptr;

    // idle
    QLabel* idleIcon = nullptr;

    // drive
    DriveCard*    driveCard    = nullptr;
    FileExplorer* explorer     = nullptr;
    QWidget*      encryptedMsg = nullptr;
    QPushButton*  encryptBtn   = nullptr;
    QPushButton*  decryptBtn   = nullptr;

    // password
    QLabel*        pwTitle       = nullptr;
    QLineEdit*     pwInput       = nullptr;
    QLineEdit*     pwConfirm     = nullptr;
    QWidget*       confirmRow    = nullptr;
    StrengthMeter* strengthMeter = nullptr;
    QLabel*        pwError       = nullptr;
    QPushButton*   pwSubmit      = nullptr;

    // progress
    ProgressRing* ring       = nullptr;
    QLabel*       ringStatus = nullptr;

    // complete
    QLabel* doneIcon = nullptr;
    QLabel* doneText = nullptr;

    // state
    QList<DriveInfo> drives;
    int  activeDrive = -1;
    bool encrypting  = true;

    QThread*     workerThread = nullptr;
    VaultWorker* worker       = nullptr;
};

}
