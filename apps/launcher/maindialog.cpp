#include "maindialog.hpp"

#include <QCloseEvent>
#include <QDir>
#include <QMessageBox>
#include <QStringList>
#include <QTime>

#include <components/debug/debugging.hpp>
#include <components/debug/debuglog.hpp>
#include <components/files/configurationmanager.hpp>
#include <components/files/conversion.hpp>
#include <components/files/qtconfigpath.hpp>
#include <components/files/qtconversion.hpp>
#include <components/misc/helpviewer.hpp>
#include <components/misc/utf8qtextstream.hpp>
#include <components/settings/settings.hpp>
#include <components/version/version.hpp>

#include "datafilespage.hpp"
#include "graphicspage.hpp"
#include "importpage.hpp"
#include "settingspage.hpp"

using namespace Process;

void cfgError(const QString& title, const QString& msg)
{
    QMessageBox msgBox;
    msgBox.setWindowTitle(title);
    msgBox.setIcon(QMessageBox::Critical);
    msgBox.setStandardButtons(QMessageBox::Ok);
    msgBox.setText(msg);
    msgBox.exec();
}

Launcher::MainDialog::MainDialog(const Files::ConfigurationManager& configurationManager, QWidget* parent)
    : QMainWindow(parent)
    , mCfgMgr(configurationManager)
    , mGameSettings(mCfgMgr)
{
    setupUi(this);

    mGameInvoker = new ProcessInvoker();
    mWizardInvoker = new ProcessInvoker();

    connect(mWizardInvoker->getProcess(), &QProcess::started, this, &MainDialog::wizardStarted);

    connect(mWizardInvoker->getProcess(), qOverload<int, QProcess::ExitStatus>(&QProcess::finished), this,
        &MainDialog::wizardFinished);

    buttonBox->button(QDialogButtonBox::Close)->setText(tr("关闭"));
    buttonBox->button(QDialogButtonBox::Ok)->setText(tr(" 启动 OpenMW "));
    buttonBox->button(QDialogButtonBox::Help)->setText(tr("帮助"));

    // Order of buttons can be different on different setups,
    // so make sure that the Play button has a focus by default.
    buttonBox->button(QDialogButtonBox::Ok)->setFocus();

    connect(buttonBox, &QDialogButtonBox::rejected, this, &MainDialog::close);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &MainDialog::play);
    connect(buttonBox, &QDialogButtonBox::helpRequested, this, &MainDialog::help);

    // Remove what's this? button
    setWindowFlags(this->windowFlags() & ~Qt::WindowContextHelpButtonHint);

    createIcons();
}

Launcher::MainDialog::~MainDialog()
{
    delete mGameInvoker;
    delete mWizardInvoker;
}

void Launcher::MainDialog::createIcons()
{
    if (!QIcon::hasThemeIcon("document-new"))
        QIcon::setThemeName("tango");

    connect(dataAction, &QAction::triggered, this, &MainDialog::enableDataPage);
    connect(graphicsAction, &QAction::triggered, this, &MainDialog::enableGraphicsPage);
    connect(settingsAction, &QAction::triggered, this, &MainDialog::enableSettingsPage);
    connect(importAction, &QAction::triggered, this, &MainDialog::enableImportPage);
}

void Launcher::MainDialog::createPages()
{
    // Avoid creating the widgets twice
    if (pagesWidget->count() != 0)
        return;

    mDataFilesPage = new DataFilesPage(mCfgMgr, mGameSettings, mLauncherSettings, this);
    mGraphicsPage = new GraphicsPage(this);
    mImportPage = new ImportPage(mCfgMgr, mGameSettings, mLauncherSettings, this);
    mSettingsPage = new SettingsPage(mGameSettings, this);

    // Add the pages to the stacked widget
    pagesWidget->addWidget(mDataFilesPage);
    pagesWidget->addWidget(mGraphicsPage);
    pagesWidget->addWidget(mSettingsPage);
    pagesWidget->addWidget(mImportPage);

    // Select the first page
    dataAction->setChecked(true);

    // Using Qt::QueuedConnection because signal is emitted in a subthread and slot is in the main thread
    connect(mDataFilesPage, &DataFilesPage::signalLoadedCellsChanged, mSettingsPage,
        &SettingsPage::slotLoadedCellsChanged, Qt::QueuedConnection);
}

Launcher::FirstRunDialogResult Launcher::MainDialog::showFirstRunDialog()
{
    if (!setupLauncherSettings())
        return FirstRunDialogResultFailure;

    // Dialog wizard and setup will fail if the config directory does not already exist
    const auto& userConfigDir = mCfgMgr.getUserConfigPath();
    if (!exists(userConfigDir))
    {
        std::error_code ec;
        if (!create_directories(userConfigDir, ec))
        {
            cfgError(tr("创建 OpenMW 配置目录出错: 错误码 %0").arg(ec.value()),
                tr("<br><b>无法创建目录 %0</b><br><br>"
                   "%1<br>")
                    .arg(Files::pathToQString(userConfigDir))
                    .arg(QString(ec.message().c_str())));
            return FirstRunDialogResultFailure;
        }
    }

    if (mLauncherSettings.isFirstRun())
    {
        QMessageBox msgBox;
        msgBox.setWindowTitle(tr("首次运行"));
        msgBox.setIcon(QMessageBox::Question);
        msgBox.setStandardButtons(QMessageBox::NoButton);
        msgBox.setText(
            tr("<html><head/><body><p><b>欢迎使用 OpenMW！</b></p>"
               "<p>推荐运行安装向导。</p>"
               "<p>向导会让你选择已安装的晨风，"
               "或现安装晨风给 OpenMW 使用。</p></body></html>"));

        QAbstractButton* wizardButton
            = msgBox.addButton(tr("运行安装向导(&I)"), QMessageBox::AcceptRole); // ActionRole doesn't work?!
        QAbstractButton* skipButton = msgBox.addButton(tr("跳过"), QMessageBox::RejectRole);

        msgBox.exec();

        if (msgBox.clickedButton() == wizardButton)
        {
            if (mWizardInvoker->startProcess(QLatin1String("openmw-wizard"), false))
                return FirstRunDialogResultWizard;
        }
        else if (msgBox.clickedButton() == skipButton)
        {
            // Don't bother setting up absent game data.
            if (setup())
                return FirstRunDialogResultContinue;
        }
        return FirstRunDialogResultFailure;
    }

    if (!setup() || !setupGameData())
    {
        return FirstRunDialogResultFailure;
    }
    return FirstRunDialogResultContinue;
}

void Launcher::MainDialog::setVersionLabel()
{
    // Add version information to bottom of the window
    QString revision(QString::fromUtf8(Version::getCommitHash().data(), Version::getCommitHash().size()));
    QString tag(QString::fromUtf8(Version::getTagHash().data(), Version::getTagHash().size()));

    versionLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    if (!Version::getVersion().empty() && (revision.isEmpty() || revision == tag))
        versionLabel->setText(
            tr("OpenMW %1 发布版").arg(QString::fromUtf8(Version::getVersion().data(), Version::getVersion().size())));
    else
        versionLabel->setText(tr("OpenMW 开发版 (%1)").arg(revision.left(10)));

    // Add the compile date and time
    auto compileDate = QLocale(QLocale::C).toDate(QString(__DATE__).simplified(), QLatin1String("MMM d yyyy"));
    auto compileTime = QLocale(QLocale::C).toTime(QString(__TIME__).simplified(), QLatin1String("hh:mm:ss"));
    versionLabel->setToolTip(tr("编译于 %1 %2")
                                 .arg(QLocale::system().toString(compileDate, QLocale::LongFormat),
                                     QLocale::system().toString(compileTime, QLocale::ShortFormat)));
}

bool Launcher::MainDialog::setup()
{
    if (!setupGameSettings())
        return false;

    setVersionLabel();

    mLauncherSettings.setContentList(mGameSettings);

    if (!setupGraphicsSettings())
        return false;

    // Now create the pages as they need the settings
    createPages();

    // Call this so we can exit on SDL errors before mainwindow is shown
    if (!mGraphicsPage->loadSettings())
        return false;

    loadSettings();

    return true;
}

bool Launcher::MainDialog::reloadSettings()
{
    if (!setupLauncherSettings())
        return false;

    if (!setupGameSettings())
        return false;

    mLauncherSettings.setContentList(mGameSettings);

    if (!setupGraphicsSettings())
        return false;

    if (!mImportPage->loadSettings())
        return false;

    if (!mDataFilesPage->loadSettings())
        return false;

    if (!mGraphicsPage->loadSettings())
        return false;

    if (!mSettingsPage->loadSettings())
        return false;

    return true;
}

void Launcher::MainDialog::enableDataPage()
{
    pagesWidget->setCurrentIndex(0);
    mImportPage->resetProgressBar();
    dataAction->setChecked(true);
    graphicsAction->setChecked(false);
    importAction->setChecked(false);
    settingsAction->setChecked(false);
}

void Launcher::MainDialog::enableGraphicsPage()
{
    pagesWidget->setCurrentIndex(1);
    mImportPage->resetProgressBar();
    dataAction->setChecked(false);
    graphicsAction->setChecked(true);
    settingsAction->setChecked(false);
    importAction->setChecked(false);
}

void Launcher::MainDialog::enableSettingsPage()
{
    pagesWidget->setCurrentIndex(2);
    mImportPage->resetProgressBar();
    dataAction->setChecked(false);
    graphicsAction->setChecked(false);
    settingsAction->setChecked(true);
    importAction->setChecked(false);
}

void Launcher::MainDialog::enableImportPage()
{
    pagesWidget->setCurrentIndex(3);
    mImportPage->resetProgressBar();
    dataAction->setChecked(false);
    graphicsAction->setChecked(false);
    settingsAction->setChecked(false);
    importAction->setChecked(true);
}

bool Launcher::MainDialog::setupLauncherSettings()
{
    mLauncherSettings.clear();

    const QString path
        = Files::pathToQString(mCfgMgr.getUserConfigPath() / Config::LauncherSettings::sLauncherConfigFileName);

    if (!QFile::exists(path))
        return true;

    Log(Debug::Verbose) << "Loading config file: " << path.toUtf8().constData();

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        cfgError(tr("打开 OpenMW 配置文件出错"),
            tr("<br><b>无法读取 %0:</b><br><br>%1<br><br>"
               "请确认你有相关权限"
               "并再次尝试。<br>")
                .arg(file.fileName())
                .arg(file.errorString()));
        return false;
    }

    QTextStream stream(&file);
    Misc::ensureUtf8Encoding(stream);

    mLauncherSettings.readFile(stream);

    return true;
}

bool Launcher::MainDialog::setupGameSettings()
{
    mGameSettings.clear();

    QFile file;

    auto loadFile = [&](const QString& path, bool (Config::GameSettings::*reader)(QTextStream&, bool),
                        bool ignoreContent = false) -> std::optional<bool> {
        file.setFileName(path);
        if (file.exists())
        {
            if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
            {
                cfgError(tr("打开 OpenMW 配置文件出错"),
                    tr("<br><b>无法读取 %0</b><br><br>"
                       "请确认你有相关权限"
                       "并再次尝试。<br>")
                        .arg(file.fileName()));
                return {};
            }
            QTextStream stream(&file);
            Misc::ensureUtf8Encoding(stream);

            (mGameSettings.*reader)(stream, ignoreContent);
            file.close();
            return true;
        }
        return false;
    };

    // Load the user config file first, separately
    // So we can write it properly, uncontaminated
    if (!loadFile(Files::getUserConfigPathQString(mCfgMgr), &Config::GameSettings::readUserFile))
        return false;

    // Now the rest - priority: user > local > global
    if (auto result = loadFile(Files::getLocalConfigPathQString(mCfgMgr), &Config::GameSettings::readFile, true))
    {
        // Load global if local wasn't found
        if (!*result && !loadFile(Files::getGlobalConfigPathQString(mCfgMgr), &Config::GameSettings::readFile, true))
            return false;
    }
    else
        return false;
    if (!loadFile(Files::getUserConfigPathQString(mCfgMgr), &Config::GameSettings::readFile))
        return false;

    return true;
}

bool Launcher::MainDialog::setupGameData()
{
    QStringList dataDirs;

    // Check if the paths actually contain data files
    for (const QString& path3 : mGameSettings.getDataDirs())
    {
        QDir dir(path3);
        QStringList filters;
        filters << "*.esp"
                << "*.esm"
                << "*.omwgame"
                << "*.omwaddon";

        if (!dir.entryList(filters).isEmpty())
            dataDirs.append(path3);
    }

    if (dataDirs.isEmpty())
    {
        QMessageBox msgBox;
        msgBox.setWindowTitle(tr("晨风安装的检测出错"));
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.setStandardButtons(QMessageBox::NoButton);
        msgBox.setText(
            tr("<br><b>无法找到 Data Files 的位置</b><br><br>"
               "含有数据文件的目录无法找到。"));

        QAbstractButton* wizardButton = msgBox.addButton(tr("运行安装向导(&I)..."), QMessageBox::ActionRole);
        QAbstractButton* skipButton = msgBox.addButton(tr("跳过"), QMessageBox::RejectRole);

        Q_UNUSED(skipButton); // Suppress compiler unused warning

        msgBox.exec();

        if (msgBox.clickedButton() == wizardButton)
        {
            if (!mWizardInvoker->startProcess(QLatin1String("openmw-wizard"), false))
                return false;
        }
    }

    return true;
}

bool Launcher::MainDialog::setupGraphicsSettings()
{
    Settings::Manager::clear(); // Ensure to clear previous settings in case we had already loaded settings.
    try
    {
        Settings::Manager::load(mCfgMgr);
        return true;
    }
    catch (std::exception& e)
    {
        cfgError(tr("读取 OpenMW 配置文件出错"),
            tr("<br>问题可能由于 OpenMW 的安装不完整。<br>"
               "重新安装 OpenMW 可能解决这个问题。<br>")
                + e.what());
        return false;
    }
}

void Launcher::MainDialog::loadSettings()
{
    const auto& mainWindow = mLauncherSettings.getMainWindow();
    resize(mainWindow.mWidth, mainWindow.mHeight);
    move(mainWindow.mPosX, mainWindow.mPosY);
}

void Launcher::MainDialog::saveSettings()
{
    mLauncherSettings.setMainWindow(Config::LauncherSettings::MainWindow{
        .mWidth = width(),
        .mHeight = height(),
        .mPosX = pos().x(),
        .mPosY = pos().y(),
    });
    mLauncherSettings.resetFirstRun();
}

bool Launcher::MainDialog::writeSettings()
{
    // Now write all config files
    saveSettings();
    mDataFilesPage->saveSettings();
    mGraphicsPage->saveSettings();
    mImportPage->saveSettings();
    mSettingsPage->saveSettings();

    const auto& userPath = mCfgMgr.getUserConfigPath();

    if (!exists(userPath))
    {
        std::error_code ec;
        if (!create_directories(userPath, ec))
        {
            cfgError(tr("创建 OpenMW 配置目录出错: 错误码 %0").arg(ec.value()),
                tr("<br><b>无法创建目录 %0</b><br><br>"
                   "%1<br>")
                    .arg(Files::pathToQString(userPath))
                    .arg(QString(ec.message().c_str())));
            return false;
        }
    }

    // Game settings
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    QFile file(userPath / Files::openmwCfgFile);
#else
    QFile file(Files::getUserConfigPathQString(mCfgMgr));
#endif

    if (!file.open(QIODevice::ReadWrite | QIODevice::Text))
    {
        // File cannot be opened or created
        cfgError(tr("写入 OpenMW 配置文件出错"),
            tr("<br><b>无法创建或写入 %0</b><br><br>"
               "请确认你有相关权限"
               "并再次尝试。<br>")
                .arg(file.fileName()));
        return false;
    }

    mGameSettings.writeFileWithComments(file);
    file.close();

    // Graphics settings
    const auto settingsPath = mCfgMgr.getUserConfigPath() / "settings.cfg";
    try
    {
        Settings::Manager::saveUser(settingsPath);
    }
    catch (std::exception& e)
    {
        std::string msg = "<br><b>写入 settings.cfg 出错</b><br><br>" + Files::pathToUnicodeString(settingsPath)
            + "<br><br>" + e.what();
        cfgError(tr("写入用户设置文件出错"), tr(msg.c_str()));
        return false;
    }

    // Launcher settings
    file.setFileName(Files::pathToQString(userPath / Config::LauncherSettings::sLauncherConfigFileName));

    if (!file.open(QIODevice::ReadWrite | QIODevice::Text | QIODevice::Truncate))
    {
        // File cannot be opened or created
        cfgError(tr("写入启动器配置文件出错"),
            tr("<br><b>无法创建或写入 %0</b><br><br>"
               "请确认你有相关权限"
               "并再次尝试。<br>")
                .arg(file.fileName()));
        return false;
    }

    QTextStream stream(&file);
    stream.setDevice(&file);
    Misc::ensureUtf8Encoding(stream);

    mLauncherSettings.writeFile(stream);
    file.close();

    return true;
}

void Launcher::MainDialog::closeEvent(QCloseEvent* event)
{
    writeSettings();
    event->accept();
}

void Launcher::MainDialog::wizardStarted()
{
    hide();
}

void Launcher::MainDialog::wizardFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    if (exitCode != 0 || exitStatus == QProcess::CrashExit)
        return qApp->quit();

    // HACK: Ensure the pages are created, else segfault
    setup();

    if (setupGameData() && reloadSettings())
        show();
}

void Launcher::MainDialog::play()
{
    if (!writeSettings())
        return qApp->quit();

    if (!mGameSettings.hasMaster())
    {
        QMessageBox msgBox;
        msgBox.setWindowTitle(tr("没选择游戏文件"));
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.setText(
            tr("<br><b>你没选择游戏文件。</b><br><br>"
               "不选择游戏文件就无法启动 OpenMW。<br>"));
        msgBox.exec();
        return;
    }

    // Launch the game detached

    if (mGameInvoker->startProcess(QLatin1String("openmw"), true))
        return qApp->quit();
}

void Launcher::MainDialog::help()
{
    Misc::HelpViewer::openHelp("reference/index.html");
}
