#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "settingsdialog.h"
#include <QDebug>
#include <QDateTime>
#include <QSettings>
#include <QCoreApplication>
#include <QDir>
#include <QLabel>
#include <QPropertyAnimation>
#include <QGraphicsOpacityEffect>
#include <QGuiApplication>
#include <QScreen>
#include <QCloseEvent>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QPushButton>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , ddcController(nullptr)
    , translator()
    , settings(nullptr)
    , increaseHotkey(nullptr)
    , decreaseHotkey(nullptr)
    , currentKeyCapture(0)
    , isChineseLanguage(true)
    , currentMonitorIndex(0)
    , currentBrightnessValue(50)
    , lastKeyBrightnessToastTime(0)
    , brightnessTimer(nullptr)
    , keyCaptureTimer(nullptr)
    , brightnessUpdatePending(false)
    , lastBrightnessUpdateTime(0)
    , toastLabel(nullptr)
    , toastAnimation(nullptr)
    , trayIcon(nullptr)
    , trayMenu(nullptr)
{
    ui->setupUi(this);
    
    /* 获取DDC控制器单例 */
    ddcController = DdcController::getInstance();
    ddcController->setOptimizeMode(true);  // 启用优化模式以提高响应速度
    
    /* 状态初始化 */
    currentMonitorIndex = 0;
    currentBrightnessValue = 50;
    brightnessUpdatePending = false;
    lastBrightnessUpdateTime = 0;
    
    /* 创建并设置亮度设置延时定时器 */
    brightnessTimer = new QTimer(this);
    brightnessTimer->setSingleShot(true);
    connect(brightnessTimer, &QTimer::timeout, this, &MainWindow::applyBrightnessChange);
    
    /* 创建配置设置对象 */
    settings = new QSettings(QCoreApplication::organizationName(), QCoreApplication::applicationName(), this);
    
    /* 创建全局热键对象 */
    increaseHotkey = new WinHotkey(this);
    decreaseHotkey = new WinHotkey(this);
    
    /* 连接热键信号 */
    connect(increaseHotkey, &WinHotkey::activated, this, &MainWindow::increaseBrightness);
    connect(decreaseHotkey, &WinHotkey::activated, this, &MainWindow::decreaseBrightness);
    
    /* 连接亮度控制信号槽 */
    connect(ui->sliderBrightness, &QSlider::valueChanged, this, &MainWindow::on_sliderBrightness_valueChanged);
    connect(ui->sliderBrightness, &QSlider::sliderReleased, this, &MainWindow::on_sliderBrightness_sliderReleased);
    
    /* 连接设置按钮信号槽 */

    
    /* 使用延时确保界面完全加载后再检测DDC/CI支持 */
    QTimer::singleShot(500, this, &MainWindow::checkDdcSupport);
    
    /* 设置窗口标题 */
    setWindowTitle(tr("DDC/CI control"));
    
    /* 设置状态栏消息 */
    ui->statusbar->showMessage(tr("正在初始化..."));
    
    /* 从INI文件加载配置 */
    loadSettings();
    
    /* 创建系统托盘图标 */
    createTrayIcon();
    
    /* 设置窗口标志，使最小化时不显示在任务栏 */
    setWindowFlags(windowFlags() | Qt::Tool);
}

MainWindow::~MainWindow()
{
    delete ui;
}

/* 检测显示器是否支持DDC/CI */
void MainWindow::checkDdcSupport()
{
    qDebug() << "检测显示器DDC/CI支持";
    
    /* 更新状态标签 */
    ui->labelStatus->setText(tr("正在检测显示器DDC/CI支持..."));
    
    /* 刷新UI确保状态更新可见 */
    QApplication::processEvents();
    
    /* 检测DDC/CI支持 */
    bool supported = ddcController->detectDdcSupport();
    
    if (!supported) {
        /* 显示器不支持DDC/CI */
        ui->labelStatus->setText(tr("显示器不支持DDC/CI"));
        ui->statusbar->showMessage(tr("显示器不支持DDC/CI"));
        
        /* 显示错误消息 */
        QMessageBox::warning(this, tr("显示器不支持DDC/CI"), 
                             tr("您的显示器不支持DDC/CI功能，无法调整亮度。"));
        
        /* 禁用亮度控制 */
        ui->sliderBrightness->setEnabled(false);
        ui->comboBoxMonitor->setEnabled(false);
        ui->progressBarBrightness->setEnabled(false);
    } else {
        /* 显示器支持DDC/CI */
        ui->labelStatus->setText(tr("已检测到支持DDC/CI的显示器"));
        ui->statusbar->showMessage(tr("已检测到支持DDC/CI的显示器"));
        
        /* 启用亮度控制 */
        ui->sliderBrightness->setEnabled(true);
        ui->progressBarBrightness->setEnabled(true);
        
        /* 如果有多个显示器，启用显示器选择 */
        if (ddcController->getSupportedDisplayCount() > 1) {
            ui->comboBoxMonitor->setEnabled(true);
        }
        
        /* 填充显示器下拉框 */
        populateMonitorComboBox();
        
        /* 获取并显示当前亮度 */
        int brightness = ddcController->getCurrentBrightness(currentMonitorIndex);
        if (brightness >= 0) {
            currentBrightnessValue = brightness;
            updateBrightnessControls(brightness);
        }
    }
}

/* 填充显示器下拉框 */
void MainWindow::populateMonitorComboBox()
{
    ui->comboBoxMonitor->clear();
    
    int supportedDisplayCount = ddcController->getSupportedDisplayCount();
    for (int i = 0; i < supportedDisplayCount; i++) {
        ui->comboBoxMonitor->addItem(tr("显示器 %1").arg(i + 1));
    }
}

/* 更新界面上显示的亮度值 */
void MainWindow::updateBrightnessControls(int brightness)
{
    /* 断开信号连接，防止循环触发 */
    ui->sliderBrightness->blockSignals(true);
    
    /* 更新控件值 */
    ui->sliderBrightness->setValue(brightness);
    ui->progressBarBrightness->setValue(brightness);
    ui->labelBrightnessPercentage->setText(QString("%1%").arg(brightness));
    
    /* 恢复信号连接 */
    ui->sliderBrightness->blockSignals(false);
}

/* 亮度滑动条值变化 - 平滑过渡优化版 */
void MainWindow::on_sliderBrightness_valueChanged(int value)
{
    /* 更新进度条和百分比标签 */
    ui->progressBarBrightness->setValue(value);
    ui->labelBrightnessPercentage->setText(QString("%1%").arg(value));
    
    /* 用于平滑过渡的静态变量 */
    static int lastValue = -1;  // 记录上一次的值，用于判断方向
    static int lastShownValue = -1;  // 记录上一次显示的值，用于平滑过渡
    static bool isInTransition = false;  // 是否正在过渡中
    static qint64 lastToastShowTime = 0; // 上次显示Toast的时间
    static qint64 lastBrightnessApplyTime = 0; // 上次应用亮度的时间
    
    /* 存储当前值 */
    currentBrightnessValue = value;
    
    /* 获取当前时间 */
    qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
    
    /* 检测是否是快速大范围滑动 */
    if (lastValue != -1) {
        int diff = abs(value - lastValue);
        if (diff > 3) {
            isInTransition = true;
        }
    }
    
    /* 更新UI滑块位置，但不触发额外信号 */
    updateBrightnessControls(value);
    
    /* 减少亮度更新频率 - 滑动过程中只在间隔时间后更新亮度 */
    bool shouldApplyBrightness = false;
    
    /* 如果是鼠标释放或间隔足够长，应用亮度更改 */
    if (!ui->sliderBrightness->isSliderDown() || 
        (currentTime - lastBrightnessApplyTime) > 80) {  // 每80ms最多更新一次亮度
        shouldApplyBrightness = true;
        lastBrightnessApplyTime = currentTime;
        
        /* 使用短延时设置亮度，允许聚合多个快速变化 */
        brightnessTimer->start(10);
    }
    
    /* 减少Toast显示频率 - 滑动过程中限制Toast显示 */
    if ((currentTime - lastToastShowTime) > 100) {  // 每100ms最多显示一次Toast
        if (isInTransition) {
            /* 确定滑动方向 */
            int direction = (value > lastShownValue) ? 1 : -1;
            
            /* 根据方向平滑更新显示值 */
            if (lastShownValue == -1) {
                lastShownValue = value;
            } else {
                /* 每次最多变化5个单位，加快过渡速度 */
                lastShownValue = lastShownValue + direction * 5;
                
                /* 确保显示值不超过目标值边界 */
                if (direction > 0) {
                    lastShownValue = qMin(lastShownValue, value);
                } else {
                    lastShownValue = qMax(lastShownValue, value);
                }
            }
            
            /* 只在滑动暂停或到达目标时显示Toast */
            if (!ui->sliderBrightness->isSliderDown() || lastShownValue == value) {
                showBrightnessToast(lastShownValue);
                lastToastShowTime = currentTime;
                
                /* 检查是否完成过渡 */
                if (lastShownValue == value) {
                    isInTransition = false;
                }
            }
        } else {
            /* 非过渡状态 - 直接显示目标值，但减少频率 */
            if (!ui->sliderBrightness->isSliderDown()) {
                showBrightnessToast(value);
                lastToastShowTime = currentTime;
            }
        }
    }
    
    lastValue = value;
    
    /* 当滑动停止时重置平滑过渡变量 */
    if (!ui->sliderBrightness->isSliderDown()) {
        /* 在滑动结束时再显示一次最终值 */
        if (shouldApplyBrightness) {
            applyBrightnessChange();
        }
        showBrightnessToast(value);
        
        /* 重置过渡状态变量 */
        lastShownValue = -1;
        lastValue = -1;
        isInTransition = false;
    }
}

/* 显示器选择框值变化 */
void MainWindow::on_comboBoxMonitor_currentIndexChanged(int index)
{
    if (index < 0) {
        return;
    }
    
    currentMonitorIndex = index;
    qDebug() << "已选择显示器" << currentMonitorIndex;
    
    /* 获取并显示当前亮度 */
    int brightness = ddcController->getCurrentBrightness(currentMonitorIndex);
    if (brightness >= 0) {
        currentBrightnessValue = brightness;
        updateBrightnessControls(brightness);
    }
}

/* 应用亮度设置 - 极速响应版本 */
void MainWindow::applyBrightnessChange()
{
    brightnessUpdatePending = false;
    lastBrightnessUpdateTime = QDateTime::currentMSecsSinceEpoch();
    
    /* 禁用界面更新以减少延迟 */
    setUpdatesEnabled(false);
    
    /* 设置显示器亮度 - 立即执行 */
    if (ddcController->setBrightness(currentMonitorIndex, currentBrightnessValue)) {
        qDebug() << "已将显示器" << currentMonitorIndex << "亮度设置为" << currentBrightnessValue;
        ui->statusbar->showMessage(tr("亮度已设置: %1%").arg(currentBrightnessValue));
        
        /* 不再检查滑动条状态，按照滑动条上的Toast显示逻辑来处理 */
        // 在on_sliderBrightness_valueChanged中处理滑动时的Toast显示
    } else {
        qDebug() << "设置显示器" << currentMonitorIndex << "亮度失败";
        ui->statusbar->showMessage(tr("设置亮度失败"));
    }
    
    /* 重新启用界面更新 */
    setUpdatesEnabled(true);
}

/* 滑动条释放立即应用亮度 */
void MainWindow::on_sliderBrightness_sliderReleased()
{
    /* 立即应用当前亮度值并停止计时器 */
    brightnessTimer->stop();
    applyBrightnessChange();
    
    /* 在滑动结束时显示toast */
    showBrightnessToast(currentBrightnessValue);
}

/* 设置亮度并更新控件状态 - 保留旧版实现供其他地方调用 */
void MainWindow::setBrightness(int value)
{
    currentBrightnessValue = value;
    if (ddcController->setBrightness(currentMonitorIndex, value)) {
        qDebug() << "已将显示器" << currentMonitorIndex << "亮度设置为" << value;
        ui->statusbar->showMessage(tr("亮度已设置: %1%").arg(value));
        showBrightnessToast(value);
    } else {
        qDebug() << "设置显示器" << currentMonitorIndex << "亮度失败";
        ui->statusbar->showMessage(tr("设置亮度失败"));
    }
}

/* 切换到中文 */
void MainWindow::on_actionChinese_triggered()
{
    if (!isChineseLanguage) {
        isChineseLanguage = true;
        switchLanguage();
    }
}

/* 切换到英文 */
void MainWindow::on_actionEnglish_triggered()
{
    if (isChineseLanguage) {
        isChineseLanguage = false;
        switchLanguage();
    }
}

/* 加载翻译 */
void MainWindow::loadTranslation(const QString &language)
{
    /* 卸载当前翻译 */
    qApp->removeTranslator(&translator);
    
    /* 加载新翻译 */
    if (translator.load(":/i18n/display_" + language)) {
        qApp->installTranslator(&translator);
        qDebug() << "已加载" << language << "翻译";
    } else {
        qDebug() << "加载" << language << "翻译失败";
    }
}

/* 更新界面语言 */
void MainWindow::switchLanguage()
{
    if (isChineseLanguage) {
        /* 切换到中文 */
        loadTranslation("zh_CN");
    } else {
        /* 切换到英文 */
        loadTranslation("en_US");
    }
    
    /* 更新界面文本 */
    ui->retranslateUi(this);
}

/* 关于菜单项点击 */
void MainWindow::on_actionAbout_triggered()
{
    QMessageBox::about(this, tr("关于 DDC/CI"), 
                      tr("DDC/CI 显示器亮度控制器\n\n此应用程序允许您通过DDC/CI协议控制支持的显示器亮度。\n\n版本：1.0.0"));
}

/* 设置菜单项点击 */
void MainWindow::on_actionSettings_triggered()
{
    // 创建并显示设置对话框
    SettingsDialog settingsDialog(this);
    
    // 加载当前设置
    QKeySequence increaseKey, decreaseKey;
    bool autoStart = false;
    
    if (settings) {
        increaseKey = QKeySequence(settings->value("HotKeys/Increase", "").toString());
        decreaseKey = QKeySequence(settings->value("HotKeys/Decrease", "").toString());
        autoStart = settings->value("General/AutoStart", false).toBool();
    }
    
    // 设置对话框初始值
    settingsDialog.setIncreaseKeySequence(increaseKey);
    settingsDialog.setDecreaseKeySequence(decreaseKey);
    settingsDialog.setAutoStart(autoStart);
    
    // 显示对话框并处理结果
    if (settingsDialog.exec() == QDialog::Accepted) {
        // 保存新设置
        if (settings) {
            settings->setValue("HotKeys/Increase", settingsDialog.getIncreaseKeySequence().toString());
            settings->setValue("HotKeys/Decrease", settingsDialog.getDecreaseKeySequence().toString());
            settings->setValue("General/AutoStart", settingsDialog.getAutoStart());
            settings->sync();
        }
        
        // 注册新的热键
        if (increaseHotkey) {
            increaseHotkey->registerHotkey(settingsDialog.getIncreaseKeySequence());
        }
        
        if (decreaseHotkey) {
            decreaseHotkey->registerHotkey(settingsDialog.getDecreaseKeySequence());
        }
        
        // 设置开机自启动
        setAutoStart(settingsDialog.getAutoStart());
    }
}

/* 快捷键设置按钮点击 */
void MainWindow::on_btnSetIncreaseKey_clicked()
{
    // 启动增加亮度快捷键捕获
    startKeyCapture(1);
}

void MainWindow::on_btnSetDecreaseKey_clicked()
{
    // 启动减少亮度快捷键捕获
    startKeyCapture(2);
}

/* 快捷键捕捉完成 */
void MainWindow::finishKeyCaptureIncrease()
{
    // 从设置对话框获取新的快捷键
    QKeySequence keySequence = QKeySequence(settings->value("HotKeys/Increase", "").toString());
    
    // 注册新的热键
    if (increaseHotkey) {
        increaseHotkey->registerHotkey(keySequence);
    }
    
    // 重置捕获状态
    currentKeyCapture = 0;
}

void MainWindow::finishKeyCaptureDecrease()
{
    // 从设置对话框获取新的快捷键
    QKeySequence keySequence = QKeySequence(settings->value("HotKeys/Decrease", "").toString());
    
    // 注册新的热键
    if (decreaseHotkey) {
        decreaseHotkey->registerHotkey(keySequence);
    }
    
    // 重置捕获状态
    currentKeyCapture = 0;
}

/* 开机自启动状态变化 */
void MainWindow::on_checkBoxAutoStart_stateChanged(int state)
{
    // 设置开机自启动
    setAutoStart(state == Qt::Checked);
    
    // 保存设置
    if (settings) {
        settings->setValue("General/AutoStart", state == Qt::Checked);
        settings->sync();
    }
}

/* 增加亮度 */
void MainWindow::increaseBrightness()
{
    // 确保显示器支持DDC/CI
    if (!ddcController->getSupportedDisplayCount()) {
        return;
    }
    
    // 限制快捷键响应频率 - 降低间隔时间以加快响应速度
    static qint64 lastKeyActionTime = 0;
    qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
    
    // 将响应间隔从150ms降到80ms，让按键感觉更灵敏
    if (currentTime - lastKeyActionTime < 80) {
        return;
    }
    lastKeyActionTime = currentTime;
    
    /* 获取当前亮度 */
    int currentBrightness = ui->sliderBrightness->value();
    
    /* 计算新亮度，使用略大的步长以加快变化速度 */
    // 检测是否持续按住按键 - 如果短时间内多次调用，增加步长
    static int consecutivePress = 0;
    static qint64 lastPressTime = 0;
    
    // 判断是否是快速连续按键
    if (currentTime - lastPressTime < 400) {
        // 连续按键次数增加，最大值为5
        consecutivePress = qMin(consecutivePress + 1, 5);
    } else {
        // 重置连续按键计数
        consecutivePress = 0;
    }
    lastPressTime = currentTime;
    
    // 动态步长：基础步长为BRIGHTNESS_STEP，连续按键时逐渐增加
    int dynamicStep = BRIGHTNESS_STEP + (consecutivePress * 2);
    
    // 计算新亮度
    int newBrightness = currentBrightness + dynamicStep;
    if (newBrightness > 100) {
        newBrightness = 100;
    }
    
    /* 如果亮度实际变化，则应用新设置 */
    if (newBrightness != currentBrightness) {
        /* 设置新值并更新UI */
        currentBrightnessValue = newBrightness;
        
        /* 直接设置亮度，跳过缓冲延迟 */
        if (ddcController->setBrightness(currentMonitorIndex, newBrightness)) {
            qDebug() << "已将显示器" << currentMonitorIndex << "亮度设置为" << newBrightness;
            
            /* 更新UI控件 */
            updateBrightnessControls(newBrightness);
            
            /* 更新状态栏 */
            ui->statusbar->showMessage(tr("亮度已设置: %1%").arg(newBrightness));
            
            /* 显示Toast提示 */
            showBrightnessToast(newBrightness);
        }
    }
}

/* 减少亮度 */
void MainWindow::decreaseBrightness()
{
    // 确保显示器支持DDC/CI
    if (!ddcController->getSupportedDisplayCount()) {
        return;
    }
    
    // 限制快捷键响应频率 - 降低间隔时间以加快响应速度
    static qint64 lastKeyActionTime = 0;
    qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
    
    // 将响应间隔从150ms降到80ms，让按键感觉更灵敏
    if (currentTime - lastKeyActionTime < 80) {
        return;
    }
    lastKeyActionTime = currentTime;
    
    /* 获取当前亮度 */
    int currentBrightness = ui->sliderBrightness->value();
    
    /* 计算新亮度，使用略大的步长以加快变化速度 */
    // 检测是否持续按住按键 - 如果短时间内多次调用，增加步长
    static int consecutivePress = 0;
    static qint64 lastPressTime = 0;
    
    // 判断是否是快速连续按键
    if (currentTime - lastPressTime < 400) {
        // 连续按键次数增加，最大值为5
        consecutivePress = qMin(consecutivePress + 1, 5);
    } else {
        // 重置连续按键计数
        consecutivePress = 0;
    }
    lastPressTime = currentTime;
    
    // 动态步长：基础步长为BRIGHTNESS_STEP，连续按键时逐渐增加
    int dynamicStep = BRIGHTNESS_STEP + (consecutivePress * 2);
    
    // 计算新亮度
    int newBrightness = currentBrightness - dynamicStep;
    if (newBrightness < 0) {
        newBrightness = 0;
    }
    
    /* 如果亮度实际变化，则应用新设置 */
    if (newBrightness != currentBrightness) {
        /* 设置新值并更新UI */
        currentBrightnessValue = newBrightness;
        
        /* 直接设置亮度，跳过缓冲延迟 */
        if (ddcController->setBrightness(currentMonitorIndex, newBrightness)) {
            qDebug() << "已将显示器" << currentMonitorIndex << "亮度设置为" << newBrightness;
            
            /* 更新UI控件 */
            updateBrightnessControls(newBrightness);
            
            /* 更新状态栏 */
            ui->statusbar->showMessage(tr("亮度已设置: %1%").arg(newBrightness));
            
            /* 显示Toast提示 */
            showBrightnessToast(newBrightness);
        }
    }
}

/* 设置开机自启动 */
void MainWindow::setAutoStart(bool enable)
{
    /* 创建/删除注册表项实现开机自启动（Windows平台） */
    QSettings bootSettings("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run", QSettings::NativeFormat);
    
    if (enable) {
        /* 获取应用程序的可执行文件路径 */
        QString appPath = QCoreApplication::applicationFilePath();
        /* 将路径中的正斜杠转换为反斜杠 */
        appPath = QDir::toNativeSeparators(appPath);
        /* 添加到注册表启动项 */
        bootSettings.setValue("DDC_CI_Display", appPath);
    } else {
        /* 从注册表启动项移除 */
        bootSettings.remove("DDC_CI_Display");
    }
}

/* 捕获快捷键 */
void MainWindow::startKeyCapture(int captureType)
{
    /* 设置当前捕获类型 */
    currentKeyCapture = captureType;
    
    if (!keyCaptureTimer) {
        /* 创建捕获超时计时器 */
        keyCaptureTimer = new QTimer(this);
        keyCaptureTimer->setSingleShot(true);
        connect(keyCaptureTimer, &QTimer::timeout, [this]() {
            /* 捕获超时，恢复正常状态 */
            currentKeyCapture = 0;
            ui->statusbar->showMessage(tr("快捷键捕获已取消"), 3000);
        });
    }
    
    /* 10秒后自动取消捕获 */
    keyCaptureTimer->start(10000);
    
    /* 显示提示信息 */
    QString message;
    if (captureType == 1) {
        message = tr("请按键设置增加亮度快捷键...");
    } else if (captureType == 2) {
        message = tr("请按键设置减少亮度快捷键...");
    }
    
    ui->statusbar->showMessage(message);
}

/* 从INI文件加载配置 */
void MainWindow::loadSettings()
{
    if (!settings) {
        return;
    }
    
    // 加载热键设置
    QKeySequence increaseKey = QKeySequence(settings->value("HotKeys/Increase", "").toString());
    QKeySequence decreaseKey = QKeySequence(settings->value("HotKeys/Decrease", "").toString());
    
    // 注册热键
    if (!increaseKey.isEmpty() && increaseHotkey) {
        increaseHotkey->registerHotkey(increaseKey);
        qDebug() << "已加载并注册增加亮度快捷键:" << increaseKey.toString();
    }
    
    if (!decreaseKey.isEmpty() && decreaseHotkey) {
        decreaseHotkey->registerHotkey(decreaseKey);
        qDebug() << "已加载并注册减少亮度快捷键:" << decreaseKey.toString();
    }
    
    // 加载开机自启动设置
    bool autoStart = settings->value("General/AutoStart", false).toBool();
    
    // 确保开机自启动设置和实际状态一致
    setAutoStart(autoStart);
    
    qDebug() << "已加载配置设置";
}

/* 显示Toast样式的亮度提示 */
void MainWindow::showBrightnessToast(int brightness)
{
    // 静态变量记录上次显示的时间
    static qint64 lastToastStartTime = 0;
    qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
    
    // 创建或重用Toast标签
    if (!toastLabel) {
        toastLabel = new QLabel(nullptr); // 设置为无父窗口，使其成为独立窗口
        toastLabel->setAlignment(Qt::AlignCenter);
        toastLabel->setStyleSheet(
            "QLabel {"
            "  background-color: rgba(40, 40, 40, 180);"
            "  color: white;"
            "  border-radius: 20px;"
            "  padding: 30px;"
            "  font-size: 24px;"
            "  font-weight: bold;"
            "}"
        );
        // 设置为无边框窗口，并保持在最顶层
        toastLabel->setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);
        toastLabel->setAttribute(Qt::WA_TranslucentBackground);
        toastLabel->setAttribute(Qt::WA_ShowWithoutActivating);
        toastLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
        
        // 一次性创建不透明度效果，避免重复创建
        QGraphicsOpacityEffect *opacityEffect = new QGraphicsOpacityEffect(toastLabel);
        toastLabel->setGraphicsEffect(opacityEffect);
        opacityEffect->setOpacity(1.0);
    }
    
    // 创建进度条样式显示
    QString progressBar;
    int progressWidth = 30; // 进度条总字符数
    int filledChars = (progressWidth * brightness) / 100;
    
    progressBar = "[";
    for (int i = 0; i < progressWidth; i++) {
        progressBar += (i < filledChars) ? "■" : "□";
    }
    progressBar += "]";
    
    // 设置标签文本
    toastLabel->setText(tr("亮度: %1%\n%2").arg(brightness).arg(progressBar));
    
    // 调整标签大小以适应文本
    toastLabel->adjustSize();
    
    // 获取主屏幕
    QScreen *screen = QGuiApplication::primaryScreen();
    QRect screenGeometry = screen->geometry();
    
    // 计算在屏幕中心位置
    int x = (screenGeometry.width() - toastLabel->width()) / 2;
    
    // 在屏幕下方显示，距离底部留出一定空间
    int bottomMargin = 100; // 距离底部的距离（像素）
    int y = screenGeometry.height() - toastLabel->height() - bottomMargin;
    
    toastLabel->move(x, y);
    
    // 如果距离上次显示时间太短，仅更新内容不重新创建动画
    if (currentTime - lastToastStartTime < 250) {
        // 确保不透明度是1.0（完全不透明）
        if (QGraphicsOpacityEffect *effect = qobject_cast<QGraphicsOpacityEffect*>(toastLabel->graphicsEffect())) {
            effect->setOpacity(1.0);
        }
        toastLabel->show();
        return;
    }
    
    // 停止之前的动画（如果有）
    if (toastAnimation && toastAnimation->state() == QAbstractAnimation::Running) {
        // 断开所有连接防止动画完成时回调被触发
        disconnect(toastAnimation, nullptr, nullptr, nullptr);
        toastAnimation->stop();
        delete toastAnimation; // 直接删除而不是deleteLater，确保立即释放
        toastAnimation = nullptr;
    }
    
    // 重置不透明度为1.0
    if (QGraphicsOpacityEffect *effect = qobject_cast<QGraphicsOpacityEffect*>(toastLabel->graphicsEffect())) {
        effect->setOpacity(1.0);
    } else {
        // 如果没有效果，创建一个新的
        QGraphicsOpacityEffect *newEffect = new QGraphicsOpacityEffect(toastLabel);
        toastLabel->setGraphicsEffect(newEffect);
        newEffect->setOpacity(1.0);
    }
    
    // 显示标签
    toastLabel->show();
    
    // 记录显示时间
    lastToastStartTime = currentTime;
    
    // 创建淡出动画，但使用较短的延迟
    QTimer::singleShot(400, this, &MainWindow::startToastFadeOut);
}

/* 开始Toast消息淡出 */
void MainWindow::startToastFadeOut()
{
    // 确保Toast标签存在且可见
    if (!toastLabel || !toastLabel->isVisible()) {
        return;
    }

    // 获取不透明度效果
    QGraphicsOpacityEffect *opacityEffect = qobject_cast<QGraphicsOpacityEffect*>(toastLabel->graphicsEffect());
    if (!opacityEffect) {
        // 如果没有效果，不执行淡出
        return;
    }

    // 创建淡出动画
    if (toastAnimation) {
        disconnect(toastAnimation, nullptr, nullptr, nullptr);
        delete toastAnimation;
    }
    
    toastAnimation = new QPropertyAnimation(opacityEffect, "opacity");
    toastAnimation->setDuration(1000);  // 减少持续时间，让消息更快消失
    toastAnimation->setStartValue(1.0);
    toastAnimation->setEndValue(0.0);
    toastAnimation->setEasingCurve(QEasingCurve::OutCubic);
    
    // 连接动画完成信号
    connect(toastAnimation, &QPropertyAnimation::finished, 
            [this]() {
        if (toastLabel) {
            toastLabel->hide();
        }
        if (toastAnimation) {
            delete toastAnimation;
            toastAnimation = nullptr;
        }
    });
    
    // 立即开始动画
    toastAnimation->start();
}

/* 创建Toast淡出动画 - 保留但不再使用的旧版本函数 */
void MainWindow::createToastAnimation(QGraphicsOpacityEffect *opacityEffect)
{
    // 为兼容性保留此函数，但实际使用startToastFadeOut
    QTimer::singleShot(400, this, &MainWindow::startToastFadeOut);
}

/* 创建系统托盘 */
void MainWindow::createTrayIcon()
{
    /* 创建托盘菜单 */
    trayMenu = new QMenu(this);
    
    /* 添加菜单项 */
    QAction *showHideAction = trayMenu->addAction(tr("显示/隐藏窗口"));
    connect(showHideAction, &QAction::triggered, [this]() {
        if (isVisible()) {
            hide();
        } else {
            show();
            activateWindow();
        }
    });
    
    trayMenu->addSeparator();
    
    QAction *settingsAction = trayMenu->addAction(tr("设置"));
    connect(settingsAction, &QAction::triggered, this, &MainWindow::on_actionSettings_triggered);
    
    trayMenu->addSeparator();
    
    QAction *quitAction = trayMenu->addAction(tr("退出"));
    connect(quitAction, &QAction::triggered, qApp, &QCoreApplication::quit);
    
    /* 创建系统托盘图标 */
    trayIcon = new QSystemTrayIcon(this);
    
    /* 设置图标 - 尝试加载自定义图标，如果失败则使用默认应用图标 */
    QIcon icon(":/icons/loco.ico");
    if (icon.isNull()) {
        icon = QIcon(":/icons/app.ico");
    }
    if (icon.isNull()) {
        icon = QApplication::windowIcon();
    }
    
    trayIcon->setIcon(icon);
    trayIcon->setToolTip(tr("DDC/CI 亮度控制"));
    trayIcon->setContextMenu(trayMenu);
    
    /* 连接托盘图标点击信号 */
    connect(trayIcon, &QSystemTrayIcon::activated, this, &MainWindow::onTrayIconActivated);
    
    /* 显示系统托盘图标 */
    trayIcon->show();
}

/* 处理系统托盘图标点击事件 */
void MainWindow::onTrayIconActivated(QSystemTrayIcon::ActivationReason reason)
{
    switch (reason) {
        case QSystemTrayIcon::Trigger:
        case QSystemTrayIcon::DoubleClick:
            // 单击或双击托盘图标，切换窗口显示状态
            if (isVisible()) {
                hide();
            } else {
                show();
                activateWindow(); // 激活窗口，使其获得焦点
            }
            break;
        default:
            break;
    }
}

/* 窗口关闭事件处理 */
void MainWindow::closeEvent(QCloseEvent *event)
{
    if (trayIcon && trayIcon->isVisible()) {
        // 如果系统托盘可见，点击关闭按钮只是隐藏窗口
        hide();
        event->ignore(); // 忽略关闭事件
        
        // 显示提示信息
        if (QSystemTrayIcon::supportsMessages()) {
            trayIcon->showMessage(tr("DDC/CI 亮度控制"),
                                 tr("应用程序将继续在系统托盘中运行。"),
                                 QSystemTrayIcon::Information,
                                 3000); // 显示3秒
        }
    } else {
        // 否则正常关闭
        QMainWindow::closeEvent(event);
    }
}
