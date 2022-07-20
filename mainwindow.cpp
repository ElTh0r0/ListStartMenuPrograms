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

#include "mainwindow.h"

#include <shlobj.h>

#include <QDebug>
#include <QDir>
#include <QDirIterator>
#include <QFileSystemModel>
#include <QImageWriter>
#include <QRegularExpression>
#include <QSettings>
#include <QStandardPaths>
#include <QTableWidget>

#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *pParent)
    : QMainWindow(pParent), m_pUi(new Ui::MainWindow) {
  m_pUi->setupUi(this);
  m_pTable = new QTableWidget(pParent);
  this->setCentralWidget(m_pTable);

  QStringList sListImgFileExt;
  for (const auto &ext : QImageWriter::supportedImageFormats()) {
    sListImgFileExt.append(ext);
  }
  this->getImageFileExtAssociates(sListImgFileExt);

  QStringList sListMenuFilter;
  sListMenuFilter << "Accessibility"
                  << "Administrative Tools"
                  << "Setup"
                  << "System Tools"
                  << "Uninstall"
                  << "Update"
                  << "Updater"
                  << "Windows PowerShell";
  const QString sMenuFilter("\\b(" + sListMenuFilter.join('|') + ")\\b");

  const QString sUserAppsFolder(
      QStandardPaths::standardLocations(QStandardPaths::ApplicationsLocation)
          .at(0));
  qDebug() << "User ApplicationsLocation:" << sUserAppsFolder;
  this->getShellLinks(sUserAppsFolder, sMenuFilter);

  const QString sAllUserAppsFolder(this->getAllUsersStartMenuPath());
  qDebug() << "All User Apps Location:   " << sAllUserAppsFolder;
  if (!sAllUserAppsFolder.isEmpty()) {
    this->getShellLinks(sAllUserAppsFolder, sMenuFilter);
  }

  // Show all found apps
  m_pTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
  m_pTable->setColumnCount(2);
  m_pTable->setColumnWidth(0, 200);
  m_pTable->setColumnWidth(1, 550);
  m_pTable->setHorizontalHeaderLabels(QStringList() << "Program"
                                                    << "Exe Filepath");

  for (const auto &app : qAsConst(m_GraphicsAppList)) {
    m_pTable->insertRow(m_pTable->rowCount());
    m_pTable->setItem(m_pTable->rowCount() - 1, 0,
                      new QTableWidgetItem(app.icon, app.name));
    m_pTable->setItem(m_pTable->rowCount() - 1, 1,
                      new QTableWidgetItem(app.filepath));
  }

  // Separator between Graphics and Misc apps
  m_pTable->insertRow(m_pTable->rowCount());
  m_pTable->setItem(
      m_pTable->rowCount() - 1, 0,
      new QTableWidgetItem("--------------------------------------"));
  m_pTable->setItem(
      m_pTable->rowCount() - 1, 1,
      new QTableWidgetItem("--------------------------------------"));

  for (const auto &app : qAsConst(m_MiscAppList)) {
    m_pTable->insertRow(m_pTable->rowCount());
    m_pTable->setItem(m_pTable->rowCount() - 1, 0,
                      new QTableWidgetItem(app.icon, app.name));
    m_pTable->setItem(m_pTable->rowCount() - 1, 1,
                      new QTableWidgetItem(app.filepath));
  }
}

MainWindow::~MainWindow() { delete m_pUi; }

auto MainWindow::getAllUsersStartMenuPath() -> QString {
  QString sRet("");
  WCHAR path[MAX_PATH];
  HRESULT hr = SHGetFolderPathW(NULL, CSIDL_COMMON_PROGRAMS, NULL, 0, path);

  if (SUCCEEDED(hr)) {
    sRet = QDir(QString::fromWCharArray(path)).absolutePath();
  } else {
    qWarning() << "Couldn't retrieve StartMenu path!";
  }

  return sRet;
}

void MainWindow::getImageFileExtAssociates(const QStringList &sListImgExt) {
  const QString sReg(
      "HKEY_CURRENT_USER\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\"
      "Explorer\\FileExts\\.%1\\OpenWithList");

  for (const auto &sExt : qAsConst(sListImgExt)) {
    QString sPath(sReg.arg(sExt));
    QSettings registry(sPath, QSettings::NativeFormat);
    for (const auto &key : registry.allKeys()) {
      if (1 == key.size()) {  // Keys for OpenWith apps are a, b, c, ...
        QString sVal = registry.value(key, "").toString();
        if (sVal.endsWith(".exe") && !m_GraphicsAppList.contains(sVal)) {
          App emptyApp;
          m_GraphicsAppList[sVal] = emptyApp;
        }
      }
    }
  }
}

void MainWindow::getShellLinks(const QString &sFolder,
                               const QString &sRegExFilter) {
  QRegularExpression regexfilter(sRegExFilter);

  // Go through all subfolders and *.lnk files
  QDirIterator it(sFolder, {"*.lnk"}, QDir::NoFilter,
                  QDirIterator::Subdirectories);
  while (it.hasNext()) {
    QFileInfo fiLnk(it.next());
    if (!regexfilter.match(fiLnk.absoluteFilePath()).hasMatch()) {
      QFileInfo fiSymlink(fiLnk.symLinkTarget());
      if (!fiSymlink.fileName().endsWith(".exe") ||
          fiSymlink.baseName().contains("unins") ||
          (m_GraphicsAppList.contains(fiSymlink.fileName()) &&
           !m_GraphicsAppList.value(fiSymlink.fileName()).filepath.isEmpty()) ||
          m_MiscAppList.contains(fiSymlink.fileName())) {
        continue;
      }
      // qDebug() << fiLnk.absoluteFilePath();
      // qDebug() << fiSymlink.absoluteFilePath();

      // Get icon from exe
      QFileSystemModel *model = new QFileSystemModel;
      model->setRootPath(fiSymlink.path());
      QIcon icon = model->fileIcon(model->index(fiSymlink.filePath()));

      App newApp;
      newApp.name = fiLnk.baseName();
      newApp.icon = icon;
      newApp.filepath = fiSymlink.absoluteFilePath();
      if (m_GraphicsAppList.contains(fiSymlink.fileName())) {
        m_GraphicsAppList.insert(fiSymlink.fileName(), newApp);
      } else {
        m_MiscAppList.insert(fiSymlink.fileName(), newApp);
      }
    }
  }
}
