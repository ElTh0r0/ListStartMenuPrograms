/**
 * Copyright (C) 2022 Thorsten Roth
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef MAINWINDOW_H_
#define MAINWINDOW_H_

#include <QMainWindow>
#include <QMap>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class QTableWidget;

struct App {
  QString name;
  QIcon icon;
  QString filepath;
};

class MainWindow : public QMainWindow {
  Q_OBJECT

 public:
  MainWindow(QWidget *pParent = nullptr);
  ~MainWindow();

 private:
  auto getAllUsersStartMenuPath() -> QString;
  void getImageFileExtAssociates(const QStringList &sListImgExt);
  void getShellLinks(const QString &sFolder, const QString &sRegExFilter);

  Ui::MainWindow *m_pUi;
  QTableWidget *m_pTable;
  QMap<QString, App> m_GraphicsAppList;
  QMap<QString, App> m_MiscAppList;
};

#endif  // MAINWINDOW_H_
