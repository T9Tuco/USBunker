#pragma once

namespace bunker::ui {

inline const char* STYLESHEET = R"qss(

QMainWindow, QWidget {
    background-color: #0d1117;
    color: #e6edf3;
    font-family: "Segoe UI", "SF Pro Display", "Cantarell", sans-serif;
    font-size: 14px;
}

QPushButton {
    background-color: #21262d;
    border: 1px solid #30363d;
    border-radius: 8px;
    padding: 10px 28px;
    color: #c9d1d9;
    font-size: 14px;
    font-weight: 500;
    min-height: 20px;
}

QPushButton:hover {
    background-color: #292e36;
    border-color: #00bcd4;
    color: #e6edf3;
}

QPushButton:pressed {
    background-color: #1a1f25;
}

QPushButton:disabled {
    background-color: #161b22;
    color: #484f58;
    border-color: #21262d;
}

QPushButton#primary {
    background-color: #00838f;
    border: 1px solid #00acc1;
    color: #ffffff;
    font-weight: 600;
}

QPushButton#primary:hover {
    background-color: #00acc1;
}

QPushButton#primary:pressed {
    background-color: #006978;
}

QPushButton#link {
    background: transparent;
    border: none;
    color: #58a6ff;
    padding: 4px 0;
    font-size: 13px;
}

QPushButton#link:hover {
    color: #79c0ff;
}

QLineEdit {
    background-color: #161b22;
    border: 2px solid #30363d;
    border-radius: 8px;
    padding: 10px 16px;
    color: #e6edf3;
    font-size: 14px;
    selection-background-color: #00838f;
}

QLineEdit:focus {
    border-color: #00bcd4;
}

QLabel {
    background: transparent;
}

QLabel#heading {
    font-size: 18px;
    font-weight: 600;
    color: #e6edf3;
}

QLabel#muted {
    color: #8b949e;
    font-size: 13px;
}

QLabel#error {
    color: #f85149;
    font-size: 14px;
}

QLabel#success {
    color: #3fb950;
    font-size: 14px;
}

QTreeView {
    background-color: #0d1117;
    border: 1px solid #21262d;
    border-radius: 8px;
    outline: none;
    font-size: 13px;
    color: #c9d1d9;
    selection-background-color: transparent;
}

QTreeView::item {
    padding: 5px 0;
    border: none;
}

QTreeView::item:selected {
    background-color: #1a3a4a;
    color: #e6edf3;
}

QTreeView::item:hover:!selected {
    background-color: #161b22;
}

QHeaderView::section {
    background-color: #161b22;
    color: #8b949e;
    border: none;
    border-bottom: 1px solid #21262d;
    border-right: 1px solid #161b22;
    padding: 7px 12px;
    font-size: 12px;
    font-weight: 600;
}

QScrollBar:vertical {
    background: transparent;
    width: 8px;
    border: none;
    margin: 0;
}

QScrollBar::handle:vertical {
    background: #30363d;
    border-radius: 4px;
    min-height: 28px;
}

QScrollBar::handle:vertical:hover {
    background: #484f58;
}

QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical,
QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical {
    background: none;
    height: 0;
    border: none;
}

QMenu {
    background-color: #161b22;
    border: 1px solid #30363d;
    border-radius: 8px;
    padding: 4px 0;
    color: #e6edf3;
    font-size: 13px;
}

QMenu::item {
    padding: 6px 24px;
}

QMenu::item:selected {
    background-color: #1a3a4a;
}

QMenu::separator {
    height: 1px;
    background: #21262d;
    margin: 4px 8px;
}

QInputDialog, QMessageBox {
    background-color: #0d1117;
    color: #e6edf3;
}

QScrollBar:horizontal {
    background: transparent;
    height: 8px;
    border: none;
    margin: 0;
}

QScrollBar::handle:horizontal {
    background: #30363d;
    border-radius: 4px;
    min-width: 28px;
}

QScrollBar::handle:horizontal:hover {
    background: #484f58;
}

QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal,
QScrollBar::add-page:horizontal, QScrollBar::sub-page:horizontal {
    background: none;
    width: 0;
    border: none;
}

)qss";

}
