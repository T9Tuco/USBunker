#include "main_window.h"
#include "style.h"

#include <QApplication>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>
#include <QSequentialAnimationGroup>
#include <QRegularExpression>
#include <QIcon>
#include <QStyle>

namespace bunker::ui {

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    setWindowTitle("USBunker");
    setMinimumSize(740, 560);
    resize(820, 620);
    setStyleSheet(STYLESHEET);
    buildUi();
    refreshDrives();
}

MainWindow::~MainWindow() {
    if (workerThread && workerThread->isRunning()) {
        workerThread->quit();
        workerThread->wait(3000);
    }
}

// ---------------------------------------------------------------------------
//  UI construction
// ---------------------------------------------------------------------------

void MainWindow::buildUi() {
    auto* root = new QWidget;
    setCentralWidget(root);
    auto* col = new QVBoxLayout(root);
    col->setContentsMargins(0, 0, 0, 0);
    col->setSpacing(0);

    // top bar
    auto* bar = new QWidget;
    bar->setFixedHeight(48);
    bar->setStyleSheet("background:#161b22;");
    auto* barRow = new QHBoxLayout(bar);
    barRow->setContentsMargins(20, 0, 20, 0);

    auto* logo = new QLabel;
    logo->setPixmap(QIcon(":/icons/shield.svg").pixmap(22, 22));
    barRow->addWidget(logo);

    auto* brand = new QLabel("USBunker");
    brand->setStyleSheet(
        "font-size:15px; font-weight:700; color:#e6edf3; padding-left:8px;");
    barRow->addWidget(brand);
    barRow->addStretch();

    auto* ver = new QLabel("v1.0");
    ver->setStyleSheet("font-size:11px; color:#484f58;");
    barRow->addWidget(ver);

    col->addWidget(bar);

    auto* line = new QFrame;
    line->setFrameShape(QFrame::HLine);
    line->setFixedHeight(1);
    line->setStyleSheet("background:#21262d; border:none;");
    col->addWidget(line);

    pages = new QStackedWidget;
    col->addWidget(pages, 1);

    buildIdlePage();
    buildDrivePage();
    buildPasswordPage();
    buildProgressPage();
    buildCompletePage();
}

// -- Idle ------------------------------------------------------------------

void MainWindow::buildIdlePage() {
    auto* page = new QWidget;
    auto* lay  = new QVBoxLayout(page);
    lay->setAlignment(Qt::AlignCenter);
    lay->setSpacing(14);

    idleIcon = new QLabel;
    idleIcon->setPixmap(QIcon(":/icons/shield.svg").pixmap(80, 80));
    idleIcon->setAlignment(Qt::AlignCenter);
    lay->addWidget(idleIcon);

    // breathing glow
    auto* opacity = new QGraphicsOpacityEffect(idleIcon);
    idleIcon->setGraphicsEffect(opacity);
    auto* seq = new QSequentialAnimationGroup(this);
    auto* fo = new QPropertyAnimation(opacity, "opacity");
    fo->setDuration(1400); fo->setStartValue(1.0); fo->setEndValue(0.3);
    fo->setEasingCurve(QEasingCurve::InOutSine);
    auto* fi = new QPropertyAnimation(opacity, "opacity");
    fi->setDuration(1400); fi->setStartValue(0.3); fi->setEndValue(1.0);
    fi->setEasingCurve(QEasingCurve::InOutSine);
    seq->addAnimation(fo);
    seq->addAnimation(fi);
    seq->setLoopCount(-1);
    seq->start();

    auto* h = new QLabel("No USB drive detected");
    h->setObjectName("heading");
    h->setAlignment(Qt::AlignCenter);
    lay->addWidget(h);

    auto* sub = new QLabel("Insert a drive, then hit Scan");
    sub->setObjectName("muted");
    sub->setAlignment(Qt::AlignCenter);
    lay->addWidget(sub);

    lay->addSpacing(14);

    auto* btn = new QPushButton("Scan for Drives");
    btn->setFixedWidth(180);
    connect(btn, &QPushButton::clicked, this, &MainWindow::refreshDrives);
    lay->addWidget(btn, 0, Qt::AlignCenter);

    pages->addWidget(page);
}

// -- Drive + Explorer ------------------------------------------------------

void MainWindow::buildDrivePage() {
    auto* page = new QWidget;
    auto* lay  = new QVBoxLayout(page);
    lay->setContentsMargins(20, 16, 20, 16);
    lay->setSpacing(12);

    driveCard = new DriveCard;
    lay->addWidget(driveCard);

    // file explorer -- visible when drive is not encrypted
    explorer = new FileExplorer;
    lay->addWidget(explorer, 1);

    // encrypted placeholder -- visible when vault is present
    encryptedMsg = new QWidget;
    auto* msgLay = new QVBoxLayout(encryptedMsg);
    msgLay->setAlignment(Qt::AlignCenter);
    msgLay->setSpacing(12);

    auto* lockPic = new QLabel;
    lockPic->setPixmap(QIcon(":/icons/lock.svg").pixmap(56, 56));
    lockPic->setAlignment(Qt::AlignCenter);
    msgLay->addWidget(lockPic);

    auto* lockTitle = new QLabel("This drive is encrypted");
    lockTitle->setObjectName("heading");
    lockTitle->setAlignment(Qt::AlignCenter);
    msgLay->addWidget(lockTitle);

    auto* lockSub = new QLabel("Enter your password to unlock and restore files");
    lockSub->setObjectName("muted");
    lockSub->setAlignment(Qt::AlignCenter);
    msgLay->addWidget(lockSub);

    lay->addWidget(encryptedMsg, 1);

    // bottom button row
    auto* row = new QHBoxLayout;
    row->setSpacing(12);

    encryptBtn = new QPushButton("Encrypt Drive");
    encryptBtn->setObjectName("primary");
    encryptBtn->setIcon(QIcon(":/icons/lock.svg"));
    encryptBtn->setMinimumWidth(160);
    connect(encryptBtn, &QPushButton::clicked, this, &MainWindow::beginEncrypt);
    row->addWidget(encryptBtn);

    decryptBtn = new QPushButton("Decrypt Drive");
    decryptBtn->setObjectName("primary");
    decryptBtn->setIcon(QIcon(":/icons/unlock.svg"));
    decryptBtn->setMinimumWidth(160);
    connect(decryptBtn, &QPushButton::clicked, this, &MainWindow::beginDecrypt);
    row->addWidget(decryptBtn);

    row->addStretch();

    auto* rescan = new QPushButton("Rescan");
    connect(rescan, &QPushButton::clicked, this, &MainWindow::refreshDrives);
    row->addWidget(rescan);

    lay->addLayout(row);

    pages->addWidget(page);
}

// -- Password --------------------------------------------------------------

void MainWindow::buildPasswordPage() {
    auto* page = new QWidget;
    auto* lay  = new QVBoxLayout(page);
    lay->setContentsMargins(100, 36, 100, 36);
    lay->setSpacing(14);

    auto* back = new QPushButton("Back");
    back->setObjectName("link");
    back->setFixedWidth(50);
    connect(back, &QPushButton::clicked, this, &MainWindow::backToDrive);
    lay->addWidget(back, 0, Qt::AlignLeft);

    pwTitle = new QLabel;
    pwTitle->setObjectName("heading");
    lay->addWidget(pwTitle);

    lay->addSpacing(6);

    pwInput = new QLineEdit;
    pwInput->setEchoMode(QLineEdit::Password);
    pwInput->setPlaceholderText("Password");
    pwInput->setMinimumHeight(44);
    lay->addWidget(pwInput);

    strengthMeter = new StrengthMeter;
    lay->addWidget(strengthMeter);

    confirmRow = new QWidget;
    auto* cLay = new QVBoxLayout(confirmRow);
    cLay->setContentsMargins(0, 0, 0, 0);
    pwConfirm = new QLineEdit;
    pwConfirm->setEchoMode(QLineEdit::Password);
    pwConfirm->setPlaceholderText("Confirm password");
    pwConfirm->setMinimumHeight(44);
    cLay->addWidget(pwConfirm);
    lay->addWidget(confirmRow);

    auto* show = new QPushButton("Show password");
    show->setObjectName("link");
    show->setCheckable(true);
    connect(show, &QPushButton::toggled, this, [this](bool on) {
        auto mode = on ? QLineEdit::Normal : QLineEdit::Password;
        pwInput->setEchoMode(mode);
        pwConfirm->setEchoMode(mode);
    });
    lay->addWidget(show, 0, Qt::AlignLeft);

    pwError = new QLabel;
    pwError->setObjectName("error");
    pwError->setVisible(false);
    lay->addWidget(pwError);

    lay->addSpacing(8);

    pwSubmit = new QPushButton;
    pwSubmit->setObjectName("primary");
    pwSubmit->setMinimumHeight(44);
    pwSubmit->setMinimumWidth(200);
    connect(pwSubmit, &QPushButton::clicked,
            this, &MainWindow::submitPassword);
    lay->addWidget(pwSubmit, 0, Qt::AlignCenter);

    lay->addStretch();
    pages->addWidget(page);

    // live strength feedback
    connect(pwInput, &QLineEdit::textChanged, this, [this](const QString& t) {
        int s = 0;
        if (t.length() >= 8)  s++;
        if (t.length() >= 12) s++;
        if (t.contains(QRegularExpression("[A-Z]"))
            && t.contains(QRegularExpression("[a-z]"))) s++;
        if (t.contains(QRegularExpression("\\d"))) s++;
        if (t.contains(QRegularExpression("[^A-Za-z0-9]"))) s++;
        strengthMeter->setScore(qMin(s, 4));
    });

    connect(pwInput,   &QLineEdit::returnPressed,
            this, &MainWindow::submitPassword);
    connect(pwConfirm, &QLineEdit::returnPressed,
            this, &MainWindow::submitPassword);
}

// -- Progress --------------------------------------------------------------

void MainWindow::buildProgressPage() {
    auto* page = new QWidget;
    auto* lay  = new QVBoxLayout(page);
    lay->setAlignment(Qt::AlignCenter);
    lay->setSpacing(24);

    ring = new ProgressRing;
    lay->addWidget(ring, 0, Qt::AlignCenter);

    ringStatus = new QLabel("Starting...");
    ringStatus->setObjectName("muted");
    ringStatus->setAlignment(Qt::AlignCenter);
    ringStatus->setWordWrap(true);
    lay->addWidget(ringStatus);

    pages->addWidget(page);
}

// -- Complete --------------------------------------------------------------

void MainWindow::buildCompletePage() {
    auto* page = new QWidget;
    auto* lay  = new QVBoxLayout(page);
    lay->setAlignment(Qt::AlignCenter);
    lay->setSpacing(16);

    doneIcon = new QLabel;
    doneIcon->setAlignment(Qt::AlignCenter);
    lay->addWidget(doneIcon);

    doneText = new QLabel;
    doneText->setAlignment(Qt::AlignCenter);
    doneText->setWordWrap(true);
    QFont f = doneText->font();
    f.setPixelSize(16);
    doneText->setFont(f);
    lay->addWidget(doneText);

    lay->addSpacing(20);

    auto* done = new QPushButton("Done");
    done->setFixedWidth(140);
    connect(done, &QPushButton::clicked, this, &MainWindow::reset);
    lay->addWidget(done, 0, Qt::AlignCenter);

    pages->addWidget(page);
}

// ---------------------------------------------------------------------------
//  Page transitions
// ---------------------------------------------------------------------------

void MainWindow::switchTo(Page target) {
    int idx = static_cast<int>(target);
    if (pages->currentIndex() == idx) return;

    QWidget* old  = pages->currentWidget();
    QWidget* next = pages->widget(idx);

    auto* fadeOutFx = new QGraphicsOpacityEffect(old);
    old->setGraphicsEffect(fadeOutFx);
    auto* animOut = new QPropertyAnimation(fadeOutFx, "opacity");
    animOut->setDuration(110);
    animOut->setStartValue(1.0);
    animOut->setEndValue(0.0);
    animOut->setEasingCurve(QEasingCurve::OutQuad);

    connect(animOut, &QPropertyAnimation::finished, this, [=]() {
        pages->setCurrentIndex(idx);
        old->setGraphicsEffect(nullptr);

        auto* fadeInFx = new QGraphicsOpacityEffect(next);
        next->setGraphicsEffect(fadeInFx);
        auto* animIn = new QPropertyAnimation(fadeInFx, "opacity");
        animIn->setDuration(160);
        animIn->setStartValue(0.0);
        animIn->setEndValue(1.0);
        animIn->setEasingCurve(QEasingCurve::InQuad);

        connect(animIn, &QPropertyAnimation::finished, this, [next]() {
            next->setGraphicsEffect(nullptr);
        });
        animIn->start(QAbstractAnimation::DeleteWhenStopped);
    });

    animOut->start(QAbstractAnimation::DeleteWhenStopped);
}

// ---------------------------------------------------------------------------
//  Slots
// ---------------------------------------------------------------------------

void MainWindow::refreshDrives() {
    drives = detectRemovableDrives();

    if (drives.isEmpty()) {
        activeDrive = -1;
        explorer->clear();
        switchTo(Idle);
        return;
    }

    activeDrive = 0;
    const auto& d = drives[0];
    driveCard->setDriveInfo(d);

    bool locked = d.hasVault;
    encryptBtn->setVisible(!locked);
    decryptBtn->setVisible(locked);
    explorer->setVisible(!locked);
    encryptedMsg->setVisible(locked);

    if (!locked)
        explorer->setRootPath(d.path);
    else
        explorer->clear();

    switchTo(Drive);
}

void MainWindow::beginEncrypt() {
    encrypting = true;
    pwTitle->setText("Set a password for encryption");
    pwSubmit->setText("Encrypt Drive");
    confirmRow->setVisible(true);
    strengthMeter->setVisible(true);
    pwInput->clear();
    pwConfirm->clear();
    pwError->setVisible(false);
    strengthMeter->setScore(0);
    switchTo(Password);
    pwInput->setFocus();
}

void MainWindow::beginDecrypt() {
    encrypting = false;
    pwTitle->setText("Enter your password");
    pwSubmit->setText("Decrypt Drive");
    confirmRow->setVisible(false);
    strengthMeter->setVisible(false);
    pwInput->clear();
    pwError->setVisible(false);
    switchTo(Password);
    pwInput->setFocus();
}

void MainWindow::submitPassword() {
    pwError->setVisible(false);
    QString pw = pwInput->text();

    if (pw.isEmpty()) {
        pwError->setText("Please enter a password.");
        pwError->setVisible(true);
        return;
    }

    if (encrypting) {
        if (pw.length() < 8) {
            pwError->setText("Password must be at least 8 characters.");
            pwError->setVisible(true);
            return;
        }
        if (pw != pwConfirm->text()) {
            pwError->setText("Passwords do not match.");
            pwError->setVisible(true);
            return;
        }
    }

    // grab password and immediately wipe the input fields
    pwInput->clear();
    pwConfirm->clear();

    switchTo(Progress);
    ring->setValue(0);
    ringStatus->setText("Starting...");

    workerThread = new QThread;
    worker       = new VaultWorker;
    worker->moveToThread(workerThread);

    connect(worker, &VaultWorker::progress,
            this, &MainWindow::onProgress, Qt::QueuedConnection);
    connect(worker, &VaultWorker::finished,
            this, &MainWindow::onFinished, Qt::QueuedConnection);
    connect(workerThread, &QThread::finished,
            worker, &QObject::deleteLater);

    QString path = drives[activeDrive].path;

    if (encrypting) {
        connect(workerThread, &QThread::started, worker,
                [w = worker, path, pw]() { w->encrypt(path, pw); });
    } else {
        connect(workerThread, &QThread::started, worker,
                [w = worker, path, pw]() { w->decrypt(path, pw); });
    }

    workerThread->start();
}

void MainWindow::onProgress(int percent, const QString& status) {
    ring->setValue(percent);
    ringStatus->setText(status);
}

void MainWindow::onFinished(bool ok, const QString& message) {
    if (workerThread) {
        workerThread->quit();
        workerThread->wait();
        workerThread->deleteLater();
        workerThread = nullptr;
        worker       = nullptr;
    }

    if (ok) {
        doneIcon->setPixmap(QIcon(":/icons/check.svg").pixmap(72, 72));
        doneText->setObjectName("success");
    } else {
        doneIcon->setPixmap(QIcon(":/icons/shield.svg").pixmap(72, 72));
        doneText->setObjectName("error");
    }
    doneText->setText(message);
    doneText->style()->unpolish(doneText);
    doneText->style()->polish(doneText);

    switchTo(Complete);
}

void MainWindow::backToDrive() {
    switchTo(Drive);
}

void MainWindow::reset() {
    refreshDrives();
}

}
