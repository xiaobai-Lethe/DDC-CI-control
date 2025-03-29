#include "settingsdialog.h"
#include "ui_settingsdialog.h"
#include <QDebug>

SettingsDialog::SettingsDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SettingsDialog),
    currentCapture(0)
{
    ui->setupUi(this);
    
    // 初始隐藏捕获提示
    ui->labelCapture->setVisible(false);
    
    // 更新按键显示
    updateKeyDisplays();
}

SettingsDialog::~SettingsDialog()
{
    delete ui;
}

void SettingsDialog::setAutoStart(bool autoStart)
{
    ui->checkBoxAutoStart->setChecked(autoStart);
}

bool SettingsDialog::getAutoStart() const
{
    return ui->checkBoxAutoStart->isChecked();
}

void SettingsDialog::setIncreaseKeySequence(const QKeySequence &keySequence)
{
    increaseKeySequence = keySequence;
    updateKeyDisplays();
}

QKeySequence SettingsDialog::getIncreaseKeySequence() const
{
    return increaseKeySequence;
}

void SettingsDialog::setDecreaseKeySequence(const QKeySequence &keySequence)
{
    decreaseKeySequence = keySequence;
    updateKeyDisplays();
}

QKeySequence SettingsDialog::getDecreaseKeySequence() const
{
    return decreaseKeySequence;
}

void SettingsDialog::updateKeyDisplays()
{
    // 显示亮度增加快捷键
    if (increaseKeySequence.isEmpty()) {
        ui->lineEditIncreaseKey->clear();
    } else {
        ui->lineEditIncreaseKey->setText(increaseKeySequence.toString(QKeySequence::NativeText));
    }
    
    // 显示亮度减少快捷键
    if (decreaseKeySequence.isEmpty()) {
        ui->lineEditDecreaseKey->clear();
    } else {
        ui->lineEditDecreaseKey->setText(decreaseKeySequence.toString(QKeySequence::NativeText));
    }
}

void SettingsDialog::startCaptureIncrease()
{
    // 设置为增加亮度快捷键捕获模式
    currentCapture = 1;
    
    // 显示捕获提示
    ui->labelCapture->setText(tr("请按键设置增加亮度快捷键..."));
    ui->labelCapture->setVisible(true);
    
    // 禁用按钮避免重复点击
    ui->btnSetIncreaseKey->setEnabled(false);
    ui->btnSetDecreaseKey->setEnabled(false);
    ui->buttonBox->setEnabled(false);
    
    // 设置焦点以捕获按键
    setFocus();
}

void SettingsDialog::startCaptureDecrease()
{
    // 设置为减少亮度快捷键捕获模式
    currentCapture = 2;
    
    // 显示捕获提示
    ui->labelCapture->setText(tr("请按键设置减少亮度快捷键..."));
    ui->labelCapture->setVisible(true);
    
    // 禁用按钮避免重复点击
    ui->btnSetIncreaseKey->setEnabled(false);
    ui->btnSetDecreaseKey->setEnabled(false);
    ui->buttonBox->setEnabled(false);
    
    // 设置焦点以捕获按键
    setFocus();
}

void SettingsDialog::stopCapture()
{
    // 重置捕获模式
    currentCapture = 0;
    
    // 隐藏捕获提示
    ui->labelCapture->setVisible(false);
    
    // 恢复按钮状态
    ui->btnSetIncreaseKey->setEnabled(true);
    ui->btnSetDecreaseKey->setEnabled(true);
    ui->buttonBox->setEnabled(true);
}

void SettingsDialog::keyPressEvent(QKeyEvent *event)
{
    // 如果不在捕获模式，执行默认处理
    if (currentCapture == 0) {
        QDialog::keyPressEvent(event);
        return;
    }
    
    // 忽略仅修饰键的情况
    if (event->key() == Qt::Key_Control || 
        event->key() == Qt::Key_Shift || 
        event->key() == Qt::Key_Alt || 
        event->key() == Qt::Key_Meta) {
        return;
    }
    
    // 如果按了Escape键，取消捕获
    if (event->key() == Qt::Key_Escape) {
        stopCapture();
        return;
    }
    
    // 创建按键序列
    Qt::KeyboardModifiers modifiers = event->modifiers();
    Qt::Key key = static_cast<Qt::Key>(event->key());
    QKeySequence keySequence(modifiers | key);
    
    // 根据当前捕获模式设置对应的快捷键
    if (currentCapture == 1) {
        increaseKeySequence = keySequence;
        stopCapture();
        updateKeyDisplays();
        emit increaseCaptureFinished();
    } else if (currentCapture == 2) {
        decreaseKeySequence = keySequence;
        stopCapture();
        updateKeyDisplays();
        emit decreaseCaptureFinished();
    }
    
    // 阻止事件继续传播
    event->accept();
}

void SettingsDialog::on_btnSetIncreaseKey_clicked()
{
    startCaptureIncrease();
}

void SettingsDialog::on_btnSetDecreaseKey_clicked()
{
    startCaptureDecrease();
} 