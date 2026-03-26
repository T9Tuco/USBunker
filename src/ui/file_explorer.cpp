#include "file_explorer.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QDesktopServices>
#include <QUrl>
#include <QFileInfo>
#include <QDir>
#include <QSet>
#include <QKeyEvent>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QMessageBox>
#include <QInputDialog>
#include <QShortcut>
#include <QApplication>
#include <QProcess>

namespace bunker::ui {

// ---------------------------------------------------------------------------
// BunkerIconProvider -- color-coded file type icons
// ---------------------------------------------------------------------------

QIcon BunkerIconProvider::icon(const QFileInfo& info) const {
    if (info.isDir())
        return QIcon(":/icons/folder.svg");

    static const QSet<QString> images   = {"jpg","jpeg","png","gif","bmp","svg",
                                           "webp","ico","tiff","tif","raw","heic"};
    static const QSet<QString> video    = {"mp4","avi","mkv","mov","wmv","flv",
                                           "webm","m4v","mpg","mpeg","ts"};
    static const QSet<QString> audio    = {"mp3","wav","flac","ogg","aac","m4a",
                                           "wma","opus","alac","aiff"};
    static const QSet<QString> docs     = {"pdf","doc","docx","xls","xlsx","ppt",
                                           "pptx","txt","rtf","odt","ods","odp",
                                           "csv","md","tex","epub"};
    static const QSet<QString> archives = {"zip","rar","7z","tar","gz","bz2",
                                           "xz","zst","cab","iso","dmg"};
    static const QSet<QString> code     = {"c","cpp","h","hpp","py","js","ts",
                                           "java","go","rs","rb","php","html",
                                           "css","json","xml","yaml","yml",
                                           "sh","bat","ps1","sql","swift","kt"};

    QString ext = info.suffix().toLower();

    if (images.contains(ext))   return QIcon(":/icons/image.svg");
    if (video.contains(ext))    return QIcon(":/icons/video.svg");
    if (audio.contains(ext))    return QIcon(":/icons/audio.svg");
    if (docs.contains(ext))     return QIcon(":/icons/document.svg");
    if (archives.contains(ext)) return QIcon(":/icons/archive.svg");
    if (code.contains(ext))     return QIcon(":/icons/code.svg");

    return QIcon(":/icons/file.svg");
}

QIcon BunkerIconProvider::icon(IconType type) const {
    switch (type) {
    case Folder:  return QIcon(":/icons/folder.svg");
    case File:    return QIcon(":/icons/file.svg");
    case Desktop: return QIcon(":/icons/folder.svg");
    default:      return QFileIconProvider::icon(type);
    }
}

QString BunkerIconProvider::type(const QFileInfo& info) const {
    if (info.isDir()) return "Folder";

    static const QHash<QString, QString> labels = {
        {"jpg","Image"},{"jpeg","Image"},{"png","Image"},{"gif","Image"},
        {"bmp","Image"},{"svg","Image"},{"webp","Image"},{"tiff","Image"},
        {"mp4","Video"},{"avi","Video"},{"mkv","Video"},{"mov","Video"},
        {"wmv","Video"},{"webm","Video"},{"flv","Video"},
        {"mp3","Audio"},{"wav","Audio"},{"flac","Audio"},{"ogg","Audio"},
        {"aac","Audio"},{"m4a","Audio"},{"opus","Audio"},
        {"pdf","Document"},{"doc","Document"},{"docx","Document"},
        {"xls","Spreadsheet"},{"xlsx","Spreadsheet"},{"csv","Spreadsheet"},
        {"ppt","Presentation"},{"pptx","Presentation"},
        {"txt","Text"},{"md","Markdown"},{"rtf","Rich Text"},
        {"zip","Archive"},{"rar","Archive"},{"7z","Archive"},
        {"tar","Archive"},{"gz","Archive"},{"iso","Disk Image"},
        {"cpp","C++ Source"},{"c","C Source"},{"h","Header"},
        {"py","Python"},{"js","JavaScript"},{"ts","TypeScript"},
        {"java","Java"},{"go","Go"},{"rs","Rust"},{"html","HTML"},
        {"css","Stylesheet"},{"json","JSON"},{"xml","XML"},
        {"sh","Shell Script"},{"bat","Batch File"},
    };

    QString ext = info.suffix().toLower();
    auto it = labels.find(ext);
    if (it != labels.end()) return *it;

    if (ext.isEmpty()) return "File";
    return ext.toUpper() + " File";
}

// ---------------------------------------------------------------------------
// FileExplorer
// ---------------------------------------------------------------------------

FileExplorer::FileExplorer(QWidget* parent) : QWidget(parent) {
    auto* col = new QVBoxLayout(this);
    col->setContentsMargins(0, 0, 0, 0);
    col->setSpacing(6);

    // navigation + toolbar
    auto* nav = new QHBoxLayout;
    nav->setSpacing(8);

    upBtn = new QPushButton("Back");
    upBtn->setObjectName("link");
    upBtn->setFixedWidth(48);
    upBtn->setEnabled(false);
    connect(upBtn, &QPushButton::clicked, this, &FileExplorer::goUp);
    nav->addWidget(upBtn);

    pathLabel = new QLabel("/");
    pathLabel->setObjectName("muted");
    pathLabel->setStyleSheet("font-family: monospace; font-size: 13px;");
    nav->addWidget(pathLabel, 1);

    newFolderBtn = new QPushButton("New Folder");
    newFolderBtn->setFixedHeight(28);
    newFolderBtn->setStyleSheet(
        "padding: 4px 12px; font-size: 12px; border-radius: 6px;");
    connect(newFolderBtn, &QPushButton::clicked,
            this, &FileExplorer::createFolder);
    nav->addWidget(newFolderBtn);

    pasteBtn = new QPushButton("Paste");
    pasteBtn->setFixedHeight(28);
    pasteBtn->setStyleSheet(
        "padding: 4px 12px; font-size: 12px; border-radius: 6px;");
    pasteBtn->setEnabled(false);
    connect(pasteBtn, &QPushButton::clicked, this, &FileExplorer::pasteItems);
    nav->addWidget(pasteBtn);

    deleteBtn = new QPushButton("Delete");
    deleteBtn->setFixedHeight(28);
    deleteBtn->setStyleSheet(
        "padding: 4px 12px; font-size: 12px; border-radius: 6px;"
        "color: #f85149;");
    deleteBtn->setEnabled(false);
    connect(deleteBtn, &QPushButton::clicked,
            this, &FileExplorer::deleteSelected);
    nav->addWidget(deleteBtn);

    col->addLayout(nav);

    // file system model -- editable for rename
    icons = new BunkerIconProvider;
    model = new QFileSystemModel(this);
    model->setIconProvider(icons);
    model->setFilter(QDir::AllEntries | QDir::NoDotAndDotDot);
    model->setReadOnly(false);

    // tree view
    tree = new QTreeView;
    tree->setModel(model);
    tree->setRootIsDecorated(false);
    tree->setItemsExpandable(false);
    tree->setSelectionMode(QAbstractItemView::ExtendedSelection);
    tree->setSelectionBehavior(QAbstractItemView::SelectRows);
    tree->setSortingEnabled(true);
    tree->sortByColumn(0, Qt::AscendingOrder);
    tree->setAnimated(false);
    tree->setIndentation(0);
    tree->setUniformRowHeights(true);
    tree->setAlternatingRowColors(false);
    tree->setIconSize(QSize(20, 20));
    tree->setEditTriggers(QAbstractItemView::EditKeyPressed);
    tree->setContextMenuPolicy(Qt::CustomContextMenu);

    // drag & drop -- accept files from outside
    tree->setDragEnabled(false);
    tree->setAcceptDrops(true);
    tree->setDropIndicatorShown(true);
    tree->viewport()->setAcceptDrops(true);

    // column layout
    tree->header()->setStretchLastSection(false);
    tree->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    tree->hideColumn(3);

    connect(tree, &QTreeView::activated,
            this, &FileExplorer::onItemActivated);
    connect(tree, &QTreeView::customContextMenuRequested,
            this, &FileExplorer::showContextMenu);

    // update toolbar state when selection changes
    connect(tree->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, [this]() { updateToolbar(); });

    // keyboard shortcuts
    auto* delKey = new QShortcut(QKeySequence::Delete, tree);
    connect(delKey, &QShortcut::activated, this, &FileExplorer::deleteSelected);

    auto* renameKey = new QShortcut(Qt::Key_F2, tree);
    connect(renameKey, &QShortcut::activated,
            this, &FileExplorer::renameSelected);

    auto* copyKey = new QShortcut(QKeySequence::Copy, tree);
    connect(copyKey, &QShortcut::activated, this, &FileExplorer::copySelected);

    auto* cutKey = new QShortcut(QKeySequence::Cut, tree);
    connect(cutKey, &QShortcut::activated, this, &FileExplorer::cutSelected);

    auto* pasteKey = new QShortcut(QKeySequence::Paste, tree);
    connect(pasteKey, &QShortcut::activated, this, &FileExplorer::pasteItems);

    col->addWidget(tree, 1);
}

void FileExplorer::setRootPath(const QString& path) {
    basePath    = path;
    currentPath = path;
    QModelIndex idx = model->setRootPath(path);
    tree->setRootIndex(idx);
    updatePathLabel();
    updateToolbar();
}

void FileExplorer::clear() {
    tree->setRootIndex(QModelIndex());
    pathLabel->setText("/");
    upBtn->setEnabled(false);
    clipboard.clear();
    updateToolbar();
}

// ---------------------------------------------------------------------------
// Navigation
// ---------------------------------------------------------------------------

void FileExplorer::onItemActivated(const QModelIndex& index) {
    if (!index.isValid()) return;

    QString path = model->filePath(index);
    QFileInfo fi(path);

    if (fi.isDir()) {
        navigateTo(path);
    } else {
        QDesktopServices::openUrl(QUrl::fromLocalFile(path));
    }
}

void FileExplorer::goUp() {
    if (currentPath == basePath) return;

    QDir dir(currentPath);
    if (!dir.cdUp()) return;

    QString parent = dir.absolutePath();
    if (!parent.startsWith(basePath))
        parent = basePath;

    navigateTo(parent);
}

void FileExplorer::navigateTo(const QString& path) {
    currentPath = path;
    tree->setRootIndex(model->index(path));
    updatePathLabel();
}

void FileExplorer::updatePathLabel() {
    QDir base(basePath);
    QString rel = base.relativeFilePath(currentPath);
    if (rel == "." || rel.isEmpty())
        pathLabel->setText("/");
    else
        pathLabel->setText("/" + rel);

    upBtn->setEnabled(currentPath != basePath);
}

void FileExplorer::updateToolbar() {
    bool hasSel = tree->selectionModel()
                  && !tree->selectionModel()->selectedRows().isEmpty();
    deleteBtn->setEnabled(hasSel);
    pasteBtn->setEnabled(!clipboard.isEmpty());
}

// ---------------------------------------------------------------------------
// Context menu
// ---------------------------------------------------------------------------

void FileExplorer::showContextMenu(const QPoint& pos) {
    QMenu menu(this);
    menu.setStyleSheet(
        "QMenu {"
        "  background-color: #161b22;"
        "  border: 1px solid #30363d;"
        "  border-radius: 8px;"
        "  padding: 4px 0;"
        "  color: #e6edf3;"
        "  font-size: 13px;"
        "}"
        "QMenu::item {"
        "  padding: 6px 24px;"
        "}"
        "QMenu::item:selected {"
        "  background-color: #1a3a4a;"
        "}"
        "QMenu::separator {"
        "  height: 1px;"
        "  background: #21262d;"
        "  margin: 4px 8px;"
        "}");

    auto sel = tree->selectionModel()->selectedRows();
    bool hasSel = !sel.isEmpty();
    bool singleSel = sel.size() == 1;

    if (singleSel) {
        QString path = model->filePath(sel.first());
        QFileInfo fi(path);
        if (fi.isDir()) {
            menu.addAction("Open Folder", this, &FileExplorer::openSelected);
        } else {
            menu.addAction("Open", this, &FileExplorer::openSelected);
        }
        menu.addSeparator();
    }

    if (hasSel) {
        menu.addAction("Copy\tCtrl+C", this, &FileExplorer::copySelected);
        menu.addAction("Cut\tCtrl+X", this, &FileExplorer::cutSelected);
    }

    if (!clipboard.isEmpty()) {
        menu.addAction("Paste\tCtrl+V", this, &FileExplorer::pasteItems);
    }

    menu.addSeparator();

    if (singleSel) {
        menu.addAction("Rename\tF2", this, &FileExplorer::renameSelected);
    }

    menu.addAction("New Folder", this, &FileExplorer::createFolder);

    if (hasSel) {
        menu.addSeparator();
        auto* delAction = menu.addAction("Delete\tDel",
                                         this, &FileExplorer::deleteSelected);
        delAction->setIcon(QIcon());
        // a gentle red hint
        QFont f = delAction->font();
        delAction->setFont(f);
    }

    menu.exec(tree->viewport()->mapToGlobal(pos));
}

// ---------------------------------------------------------------------------
// File operations
// ---------------------------------------------------------------------------

QStringList FileExplorer::selectedPaths() {
    QStringList paths;
    auto rows = tree->selectionModel()->selectedRows();
    for (auto& idx : rows) {
        paths << model->filePath(idx);
    }
    return paths;
}

void FileExplorer::openSelected() {
    auto sel = tree->selectionModel()->selectedRows();
    if (sel.isEmpty()) return;

    QString path = model->filePath(sel.first());
    QFileInfo fi(path);

    if (fi.isDir()) {
        navigateTo(path);
    } else {
        QDesktopServices::openUrl(QUrl::fromLocalFile(path));
    }
}

void FileExplorer::renameSelected() {
    auto sel = tree->selectionModel()->selectedRows();
    if (sel.size() != 1) return;
    tree->edit(sel.first());
}

void FileExplorer::deleteSelected() {
    QStringList paths = selectedPaths();
    if (paths.isEmpty()) return;

    QString msg;
    if (paths.size() == 1) {
        QFileInfo fi(paths.first());
        msg = QString("Delete \"%1\"?").arg(fi.fileName());
    } else {
        msg = QString("Delete %1 items?").arg(paths.size());
    }

    auto answer = QMessageBox::question(
        this, "Confirm Delete", msg,
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);

    if (answer != QMessageBox::Yes) return;

    for (auto& path : paths) {
        QFileInfo fi(path);
        if (fi.isSymLink()) continue;

        if (fi.isDir()) {
            QDir dir(path);
            dir.removeRecursively();
        } else if (fi.isFile()) {
            QFile::remove(path);
        }
    }
}

void FileExplorer::createFolder() {
    bool ok = false;
    QString name = QInputDialog::getText(
        this, "New Folder", "Folder name:",
        QLineEdit::Normal, "New Folder", &ok);

    if (!ok || name.trimmed().isEmpty()) return;
    name = name.trimmed();

    // no path separators allowed
    if (name.contains('/') || name.contains('\\')) {
        QMessageBox::warning(this, "Invalid Name",
            "Folder name cannot contain path separators.");
        return;
    }

    QDir dir(currentPath);
    if (dir.exists(name)) {
        QMessageBox::warning(this, "Already Exists",
            "A file or folder with that name already exists.");
        return;
    }

    dir.mkdir(name);
}

void FileExplorer::copySelected() {
    clipboard = selectedPaths();
    clipboardIsCut = false;
    updateToolbar();
}

void FileExplorer::cutSelected() {
    clipboard = selectedPaths();
    clipboardIsCut = true;
    updateToolbar();
}

void FileExplorer::pasteItems() {
    if (clipboard.isEmpty()) return;

    QDir dest(currentPath);
    int errors = 0;

    for (auto& srcPath : clipboard) {
        QFileInfo srcInfo(srcPath);
        QString destPath = dest.filePath(srcInfo.fileName());

        // avoid overwriting silently -- find a unique name
        if (QFileInfo::exists(destPath)) {
            QString base = srcInfo.completeBaseName();
            QString ext  = srcInfo.suffix();
            int n = 1;
            do {
                if (ext.isEmpty())
                    destPath = dest.filePath(
                        QString("%1 (%2)").arg(base).arg(n));
                else
                    destPath = dest.filePath(
                        QString("%1 (%2).%3").arg(base).arg(n).arg(ext));
                n++;
            } while (QFileInfo::exists(destPath));
        }

        bool ok = false;
        if (srcInfo.isDir()) {
            ok = copyRecursive(srcPath, destPath);
            if (ok && clipboardIsCut) {
                QDir(srcPath).removeRecursively();
            }
        } else {
            ok = QFile::copy(srcPath, destPath);
            if (ok && clipboardIsCut) {
                QFile::remove(srcPath);
            }
        }

        if (!ok) errors++;
    }

    if (clipboardIsCut) clipboard.clear();
    updateToolbar();

    if (errors > 0) {
        QMessageBox::warning(this, "Paste Error",
            QString("Failed to paste %1 item(s).").arg(errors));
    }
}

bool FileExplorer::copyRecursive(const QString& src, const QString& dst) {
    QDir srcDir(src);
    QDir dstDir(dst);

    if (!dstDir.mkpath("."))
        return false;

    // copy files
    for (auto& name : srcDir.entryList(QDir::Files | QDir::Hidden)) {
        if (!QFile::copy(srcDir.filePath(name), dstDir.filePath(name)))
            return false;
    }

    // recurse into subdirectories
    for (auto& name : srcDir.entryList(
             QDir::Dirs | QDir::NoDotAndDotDot | QDir::Hidden)) {
        if (!copyRecursive(srcDir.filePath(name), dstDir.filePath(name)))
            return false;
    }

    return true;
}

}
