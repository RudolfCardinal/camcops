/*
    Copyright (C) 2012-2020 Rudolf Cardinal (rudolf@pobox.com).

    This file is part of CamCOPS.

    CamCOPS is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    CamCOPS is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with CamCOPS. If not, see <http://www.gnu.org/licenses/>.
*/

// http://www.qtforum.org/article/18183/messagebox-with-qscrollbar.html
// ... modified a bit

#pragma once
#include <QMessageBox>
#include <QDialogButtonBox>


class ScrollMessageBox : public QDialog
{
    // A version of QMessageBox that scrolls its contents.
    // Use this by default, because if you run CamCOPS on small phone screens,
    // scrolling becomes vital.

    Q_OBJECT

    using StandardButton = QDialogButtonBox::StandardButton;

public:
    ScrollMessageBox(const QMessageBox::Icon& icon,
                     const QString& title,
                     const QString& text,
                     QWidget* parent = nullptr);
    void addButton(QAbstractButton* button, QDialogButtonBox::ButtonRole role);
    void addButton(QAbstractButton* button, QMessageBox::ButtonRole role);
    QPushButton* addButton(const QString& text, QDialogButtonBox::ButtonRole role);
    QPushButton* addButton(const QString& text, QMessageBox::ButtonRole role);
    void setDefaultButton(QPushButton* button);
    QAbstractButton* clickedButton() const;

signals:
    void buttonClicked(QAbstractButton* button);

protected:
    // QSize minimumSizeHint() const override; // NOT CALLED
    void setIcon(QMessageBox::Icon icon);
    QPixmap standardIcon(QMessageBox::Icon icon);
    static QDialogButtonBox::ButtonRole forceEnumMD(QMessageBox::ButtonRole role);
    static QMessageBox::ButtonRole forceEnumDM(QDialogButtonBox::ButtonRole role);

    QLabel* m_text_label;
    QLabel* m_icon_label;
    QDialogButtonBox* m_button_box;
    QAbstractButton* m_clicked_button;

private slots:
    void handleButtonClicked(QAbstractButton* button);

public:
    static StandardButton critical(
            QWidget* parent, const QString& title, const QString& text);
    static StandardButton information(
            QWidget* parent, const QString& title, const QString& text);
    static StandardButton question(
            QWidget* parent, const QString& title, const QString& text);
    static StandardButton warning(
            QWidget* parent, const QString& title, const QString& text);
    static StandardButton plain(
            QWidget* parent, const QString& title, const QString& text);
};
