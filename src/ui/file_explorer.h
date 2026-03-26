#pragma once

#include <QWidget>
#include <QTreeView>
#include <QFileSystemModel>
#include <QLabel>
#include <QPushButton>
#include <QFileIconProvider>
#include <QMenu>
#include <QClipboard>

namespace bunker::ui {

class BunkerIconProvider : public QFileIconProvider {
public:
    QIcon icon(const QFileInfo& info) const override;
    QIcon icon(IconType type) const override;
    QString type(const QFileInfo& info) const override;
};

class FileExplorer : public QWidget {
    Q_OBJECT

public:
    explicit FileExplorer(QWidget* parent = nullptr);
    void setRootPath(const QString& path);
    void clear();

private slots:
    void onItemActivated(const QModelIndex& index);
    void goUp();
    void showContextMenu(const QPoint& pos);

    void openSelected();
    void renameSelected();
    void deleteSelected();
    void createFolder();
    void copySelected();
    void cutSelected();
    void pasteItems();

private:
    void navigateTo(const QString& path);
    void updatePathLabel();
    void updateToolbar();
    bool copyRecursive(const QString& src, const QString& dst);
    QStringList selectedPaths();

    QTreeView*          tree         = nullptr;
    QFileSystemModel*   model        = nullptr;
    BunkerIconProvider* icons        = nullptr;
    QLabel*             pathLabel    = nullptr;
    QPushButton*        upBtn        = nullptr;
    QPushButton*        newFolderBtn = nullptr;
    QPushButton*        pasteBtn     = nullptr;
    QPushButton*        deleteBtn    = nullptr;
    QString             basePath;
    QString             currentPath;

    QStringList         clipboard;
    bool                clipboardIsCut = false;
};

}
