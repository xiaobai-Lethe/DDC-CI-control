#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTranslator>
#include <QTimer>
#include <QMessageBox>
#include <QSettings>
#include <QShortcut>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QGraphicsEffect>
#include "ddc_controller.h"
#include "winhotkey.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class QLabel;
class QPropertyAnimation;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    /* 初始化完成后检测DDC/CI支持 */
    void checkDdcSupport();
    
    /* 亮度滑动条值变化 */
    void on_sliderBrightness_valueChanged(int value);
    
    /* 显示器选择框值变化 */
    void on_comboBoxMonitor_currentIndexChanged(int index);
    
    /* 切换到中文 */
    void on_actionChinese_triggered();
    
    /* 切换到英文 */
    void on_actionEnglish_triggered();
    
    /* 关于菜单项点击 */
    void on_actionAbout_triggered();
    
    /* 设置菜单项点击 */
    void on_actionSettings_triggered();
    
    /* 延时设置亮度，优化滑动流畅度 */
    void applyBrightnessChange();
    
    /* 滑动条释放事件，立即应用亮度设置 */
    void on_sliderBrightness_sliderReleased();
    
    /* 快捷键设置按钮点击 */
    void on_btnSetIncreaseKey_clicked();
    void on_btnSetDecreaseKey_clicked();
    
    /* 快捷键捕捉完成 */
    void finishKeyCaptureIncrease();
    void finishKeyCaptureDecrease();
    
    /* 开机自启动状态变化 */
    void on_checkBoxAutoStart_stateChanged(int state);
    
    /* 增加亮度的快捷键响应 */
    void increaseBrightness();
    
    /* 减少亮度的快捷键响应 */
    void decreaseBrightness();
    
    /* 开始Toast消息淡出动画 */
    void startToastFadeOut();

private:
    Ui::MainWindow *ui;
    
    /* DDC控制器 */
    DdcController *ddcController;
    
    /* 翻译器 */
    QTranslator translator;
    
    /* 配置设置对象 */
    QSettings *settings;
    
    /* 快速亮度变化步长 */
    static const int BRIGHTNESS_STEP = 5;
    
    /* 增加亮度全局热键 */
    WinHotkey *increaseHotkey;
    
    /* 减少亮度全局热键 */
    WinHotkey *decreaseHotkey;
    
    /* 当前正在捕获的快捷键类型（0=不捕获，1=增加亮度键，2=减少亮度键） */
    int currentKeyCapture;
    
    /* 当前语言是否为中文 */
    bool isChineseLanguage;
    
    /* 当前选中的显示器索引 */
    int currentMonitorIndex;
    
    /* 当前设置的亮度值 */
    int currentBrightnessValue;
    
    /* 上次显示快捷键亮度Toast的时间 */
    qint64 lastKeyBrightnessToastTime;
    
    /* 亮度设置延时定时器 */
    QTimer *brightnessTimer;
    
    /* 快捷键捕获计时器 */
    QTimer *keyCaptureTimer;
    
    /* 亮度更新节流标志 */
    bool brightnessUpdatePending;
    
    /* 最后一次亮度更新时间 */
    qint64 lastBrightnessUpdateTime;
    
    /* 亮度更新最小时间间隔(毫秒) */
    static const int BRIGHTNESS_UPDATE_INTERVAL = 30;
    
    /* 更新界面上显示的亮度值 */
    void updateBrightnessControls(int brightness);
    
    /* 设置亮度并更新控件状态 */
    void setBrightness(int value);
    
    /* 加载翻译 */
    void loadTranslation(const QString &language);
    
    /* 更新界面语言 */
    void switchLanguage();
    
    /* 填充显示器下拉框 */
    void populateMonitorComboBox();
    
    /* 保存配置到INI文件 */
    void saveSettings();
    
    /* 从INI文件加载配置 */
    void loadSettings();
    
    /* 设置开机自启动 */
    void setAutoStart(bool enable);
    
    /* 注册全局快捷键 */
    void registerHotkeys();
    
    /* 注销全局快捷键 */
    void unregisterHotkeys();
    
    /* 捕获快捷键 */
    void startKeyCapture(int captureType);
    
    /* 显示Toast样式的亮度提示 */
    void showBrightnessToast(int brightness);
    
    /* Toast提示标签 */
    QLabel *toastLabel;
    
    /* Toast动画 */
    QPropertyAnimation *toastAnimation;
    
    /* 系统托盘图标 */
    QSystemTrayIcon *trayIcon;
    
    /* 系统托盘菜单 */
    QMenu *trayMenu;
    
    /* 创建系统托盘 */
    void createTrayIcon();
    
    /* 处理系统托盘图标点击事件 */
    void onTrayIconActivated(QSystemTrayIcon::ActivationReason reason);
    
    /* 窗口关闭事件 */
    void closeEvent(QCloseEvent *event) override;
    
    void createToastAnimation(QGraphicsOpacityEffect *opacityEffect);
    void setFixedBrightness(int brightness);
};
#endif // MAINWINDOW_H
