#pragma once

#include <QWidget>
#include <QTreeView>
#include <QFileSystemModel>
#include <QLabel>
#include <QPushButton>
#include <QFileIconProvider>

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

private:
    void navigateTo(const QString& path);
    void updatePathLabel();

    QTreeView*          tree         = nullptr;
    QFileSystemModel*   model        = nullptr;
    BunkerIconProvider* icons        = nullptr;
    QLabel*             pathLabel    = nullptr;
    QPushButton*        upBtn        = nullptr;
    QString             basePath;
    QString             currentPath;
};

}
