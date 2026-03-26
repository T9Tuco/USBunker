#include "file_explorer.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QDesktopServices>
#include <QUrl>
#include <QFileInfo>
#include <QDir>
#include <QSet>

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

    // navigation bar
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

    col->addLayout(nav);

    // file system model
    icons = new BunkerIconProvider;
    model = new QFileSystemModel(this);
    model->setIconProvider(icons);
    model->setFilter(QDir::AllEntries | QDir::NoDotAndDotDot);
    model->setReadOnly(true);

    // tree view
    tree = new QTreeView;
    tree->setModel(model);
    tree->setRootIsDecorated(false);
    tree->setItemsExpandable(false);
    tree->setSelectionMode(QAbstractItemView::SingleSelection);
    tree->setSelectionBehavior(QAbstractItemView::SelectRows);
    tree->setSortingEnabled(true);
    tree->sortByColumn(0, Qt::AscendingOrder);
    tree->setAnimated(false);
    tree->setIndentation(0);
    tree->setUniformRowHeights(true);
    tree->setAlternatingRowColors(false);
    tree->setIconSize(QSize(20, 20));

    // column layout: Name gets most space, then Size, Type
    tree->header()->setStretchLastSection(false);
    tree->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    tree->hideColumn(3); // hide "Date Modified"

    connect(tree, &QTreeView::activated,
            this, &FileExplorer::onItemActivated);

    col->addWidget(tree, 1);
}

void FileExplorer::setRootPath(const QString& path) {
    basePath    = path;
    currentPath = path;
    QModelIndex idx = model->setRootPath(path);
    tree->setRootIndex(idx);
    updatePathLabel();
}

void FileExplorer::clear() {
    tree->setRootIndex(QModelIndex());
    pathLabel->setText("/");
    upBtn->setEnabled(false);
}

void FileExplorer::onItemActivated(const QModelIndex& index) {
    if (!index.isValid()) return;

    QString path = model->filePath(index);
    QFileInfo fi(path);

    if (fi.isDir()) {
        navigateTo(path);
    } else {
        // open with whatever the OS thinks is best
        QDesktopServices::openUrl(QUrl::fromLocalFile(path));
    }
}

void FileExplorer::goUp() {
    if (currentPath == basePath) return;

    QDir dir(currentPath);
    if (!dir.cdUp()) return;

    QString parent = dir.absolutePath();
    // don't escape the USB root
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

}
