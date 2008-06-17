/*
 * The information in this file is
 * Copyright(c) 2007 Ball Aerospace & Technologies Corporation
 * and is subject to the terms and conditions of the
 * GNU Lesser General Public License Version 2.1
 * The license text is available from   
 * http://www.gnu.org/licenses/lgpl.html
 */

#include <QtCore/QDir>
#include <QtGui/QLabel>
#include <QtGui/QLayout>
#include <QtGui/QMessageBox>

#include "ConfigurationSettings.h"
#include "AppVerify.h"
#include "Filename.h"
#include "FilePlugInDlg.h"
#include "ObjectResource.h"
#include "PlugInDescriptor.h"
#include "PlugInManagerServices.h"

#include <string>
#include <vector>
using namespace std;

std::map<std::string, QString> FilePlugInDlg::mLastPlugIns = std::map<std::string, QString>();

FilePlugInDlg::FilePlugInDlg(const std::vector<PlugInDescriptor*> &availablePlugins, 
                             const std::string plugInKey, QWidget* parent) :
   QFileDialog(parent), mPlugInKey(plugInKey)
{
   // Plug-in label
   mpPlugInLabel = new QLabel(this);
   setPlugInLabel("Plug-Ins");

   // Plug-in combo
   mpPlugInWidget = new QWidget(this);
   mpPlugInCombo = new QComboBox(mpPlugInWidget);
   mpPlugInCombo->setEditable(false);

   QHBoxLayout* pPlugInLayout = new QHBoxLayout(mpPlugInWidget);
   pPlugInLayout->setMargin(0);
   pPlugInLayout->setSpacing(5);
   pPlugInLayout->addWidget(mpPlugInCombo, 10);

   // Options button
   mpOptionsButton = new QPushButton("&Options...", this);

   // Layout
   QGridLayout* pGrid = dynamic_cast<QGridLayout*>(layout());
   if (pGrid != NULL)
   {
      pGrid->addWidget(mpPlugInLabel, 4, 0);
      pGrid->addWidget(mpPlugInWidget, 4, 1);
      pGrid->addWidget(mpOptionsButton, 4, 2);
      pGrid->setRowStretch(1, 10);
      pGrid->setColumnStretch(1, 10);
   }

   // Initializtion
   setWindowTitle("Open");
   setModal(true);
   setFileMode(QFileDialog::ExistingFile);
   enableOptions(false);

   for (std::vector<PlugInDescriptor*>::const_iterator iter = availablePlugins.begin();
      iter != availablePlugins.end(); ++iter)
   {
      PlugInDescriptor *pDescriptor = *iter;
      LOG_IF(pDescriptor == NULL, continue);

      QString strPlugIn = QString::fromStdString(pDescriptor->getName());
      QStringList filterList;

      // Add the file filters to the list
      string filters = pDescriptor->getFileExtensions();
      if (filters.empty() == false)
      {
         QString strFilters = QString::fromStdString(filters);
         filterList = strFilters.split(";;", QString::SkipEmptyParts);
      }

      filterList.append("All Files (*)");
      mPlugInFilters.insert(strPlugIn, filterList);

      // Add the plug-in to the combo
      mpPlugInCombo->addItem(strPlugIn);
   }

   // Set the path bookmarks
   QStringList bookmarks;
   vector<Filename*> pathBookmarks = Service<ConfigurationSettings>()->getSettingPathBookmarks();
   for(vector<Filename*>::iterator pathBookmark = pathBookmarks.begin(); pathBookmark != pathBookmarks.end(); ++pathBookmark)
   {
      Filename *pPath = *pathBookmark;
      if(pPath != NULL && pPath->isDirectory())
      {
         bookmarks << QString::fromStdString(pPath->getFullPathAndName());
      }
   }
   setHistory(bookmarks);

   // Set the initial directory
   string directory;
   const Filename* pWorkingDir = NULL;
   Service<ConfigurationSettings> pSettings;
   pWorkingDir = pSettings->getSetting(mPlugInKey).getPointerToValue<Filename>();
   if (pWorkingDir == NULL)
   {
      pWorkingDir = pSettings->getSettingImportExportPath();
   }
   if (pWorkingDir != NULL)
   {
      directory = pWorkingDir->getFullPathAndName();
   }

   if(!directory.empty())
   {
      setDirectory(QString::fromStdString(directory));
   }

   // Set the initial plug-in
   mpPlugInCombo->setCurrentIndex(0);
   setSelectedPlugIn(mLastPlugIns[mPlugInKey]);
   updateFileFilters(mpPlugInCombo->currentText());

   // Connections
   connect(mpPlugInCombo, SIGNAL(activated(const QString&)), this, SLOT(updateFileFilters(const QString&)));
   connect(mpPlugInCombo, SIGNAL(activated(const QString&)), this, SIGNAL(plugInSelected(const QString&)));
   connect(mpOptionsButton, SIGNAL(clicked()), this, SIGNAL(optionsClicked()));
}

FilePlugInDlg::~FilePlugInDlg()
{
}

void FilePlugInDlg::setSelectedPlugIn(const QString& strPlugIn)
{
   QString strCurrentPlugIn = getSelectedPlugIn();
   if ((strPlugIn.isEmpty() == true) || (strCurrentPlugIn == strPlugIn))
   {
      return;
   }

   int idx = mpPlugInCombo->findText(strPlugIn);
   if(idx != -1)
   {
      mpPlugInCombo->setCurrentIndex(idx);
      updateFileFilters(strPlugIn);
      emit plugInSelected(strPlugIn);
   }
}

QString FilePlugInDlg::getSelectedPlugIn() const
{
   return mpPlugInCombo->currentText();
}

void FilePlugInDlg::enableOptions(bool bEnable)
{
   mpOptionsButton->setEnabled(bEnable);
}

QWidget* FilePlugInDlg::getPlugInWidget() const
{
   return mpPlugInWidget;
}

bool FilePlugInDlg::isDefaultExtension(const QString& strExtension) const
{
   if (strExtension.isEmpty() == true)
   {
      return false;
   }

   QString strFilters = selectedFilter();

   int iPosition = strFilters.indexOf(".");
   while (iPosition != -1)
   {
      int iLength = strFilters.length();
      strFilters = strFilters.right(iLength - iPosition - 1);

      string filters = strFilters.toStdString();

      QString strCurrentExtension = strFilters;

      int iSemicolon = -1;
      iSemicolon = strFilters.indexOf(";");
      if (iSemicolon != -1)
      {
         strCurrentExtension = strFilters.left(iSemicolon);
      }
      else
      {
         int iParen = -1;
         iParen = strFilters.indexOf(")");
         if (iParen != -1)
         {
            strCurrentExtension = strFilters.left(iParen);
         }
      }

      strCurrentExtension = strCurrentExtension.trimmed();

      if (strCurrentExtension == strExtension)
      {
         return true;
      }

      iPosition = strFilters.indexOf(".");
   }

   return false;
}

void FilePlugInDlg::updateFileFilters(const QString& strPlugIn)
{
   QString strFilter;
   if (strPlugIn.isEmpty() == false)
   {
      QMap<QString, QStringList>::iterator iter = mPlugInFilters.find(strPlugIn);
      if (iter != mPlugInFilters.end())
      {
         QStringList filterList = iter.value();
         if (filterList.empty() == false)
         {
            strFilter = filterList.front();
            setFilters(filterList);
         }
      }
   }

   if (strFilter.isEmpty() == true)
   {
      setFilter("All Files (*)");
   }
}

void FilePlugInDlg::accept()
{
   QStringList selected = selectedFiles();
   if(!selected.isEmpty() && !QFileInfo(selected.front()).isDir())
   {
      // Set the current plug-in to the selected plug-in
      mLastPlugIns[mPlugInKey] = getSelectedPlugIn();

      QDir dir = directory();

      QString strDirectory = dir.absolutePath();
      if (strDirectory.isEmpty() == false)
      {
         FactoryResource<Filename> pImportDir;
         pImportDir->setFullPathAndName(strDirectory.toStdString());
         Service<ConfigurationSettings> pSettings;
         pSettings->setSessionSetting(mPlugInKey, *pImportDir.get());
      }
   }

   QFileDialog::accept();
}

std::vector<PlugInDescriptor*> FilePlugInDlg::getPlugInNames(const std::string &plugInType, std::string subtype)
{
   std::vector<PlugInDescriptor*> selectedDescriptors;
   std::vector<PlugInDescriptor*> descriptors = Service<PlugInManagerServices>()->getPlugInDescriptors(plugInType);
   for (std::vector<PlugInDescriptor*>::const_iterator iter = descriptors.begin();
      iter != descriptors.end(); ++iter)
   {
      PlugInDescriptor *pDescriptor = *iter;
      LOG_IF(pDescriptor == NULL, continue);

      if (pDescriptor->getSubtype() == subtype || subtype.empty())
      {
         selectedDescriptors.push_back(pDescriptor);
      }
   }
   return selectedDescriptors;
}

bool FilePlugInDlg::isDefaultPlugIn() const
{
   return (getSelectedPlugIn() != mLastPlugIns[mPlugInKey]);
}

void FilePlugInDlg::setPlugInLabel(const QString &label)
{
   mpPlugInLabel->setText(label + ":  ");
}