#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>
#include <QKeySequence>
#include <QKeyEvent>

namespace Ui {
class SettingsDialog;
}

class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget *parent = nullptr);
    ~SettingsDialog();
    
    /* 设置自动启动状态 */
    void setAutoStart(bool autoStart);
    
    /* 获取自动启动状态 */
    bool getAutoStart() const;
    
    /* 设置增加亮度快捷键 */
    void setIncreaseKeySequence(const QKeySequence &keySequence);
    
    /* 获取增加亮度快捷键 */
    QKeySequence getIncreaseKeySequence() const;
    
    /* 设置减少亮度快捷键 */
    void setDecreaseKeySequence(const QKeySequence &keySequence);
    
    /* 获取减少亮度快捷键 */
    QKeySequence getDecreaseKeySequence() const;
    
    /* 开始捕获快捷键 */
    void startCaptureIncrease();
    void startCaptureDecrease();
    
    /* 停止捕获快捷键 */
    void stopCapture();
    
protected:
    /* 按键事件处理 */
    void keyPressEvent(QKeyEvent *event) override;
    
private slots:
    /* 设置增加亮度快捷键按钮点击 */
    void on_btnSetIncreaseKey_clicked();
    
    /* 设置减少亮度快捷键按钮点击 */
    void on_btnSetDecreaseKey_clicked();
    
signals:
    /* 增加亮度快捷键捕获完成信号 */
    void increaseCaptureFinished();
    
    /* 减少亮度快捷键捕获完成信号 */
    void decreaseCaptureFinished();

private:
    Ui::SettingsDialog *ui;
    
    /* 当前正在捕获的快捷键类型（0=不捕获，1=增加亮度键，2=减少亮度键） */
    int currentCapture;
    
    /* 已设置的增加亮度快捷键 */
    QKeySequence increaseKeySequence;
    
    /* 已设置的减少亮度快捷键 */
    QKeySequence decreaseKeySequence;
    
    /* 更新快捷键显示 */
    void updateKeyDisplays();
};

#endif // SETTINGSDIALOG_H 