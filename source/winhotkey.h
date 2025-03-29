#ifndef WINHOTKEY_H
#define WINHOTKEY_H

#include <QObject>
#include <QKeySequence>
#include <QAbstractNativeEventFilter>
#include <windows.h>

// 全局热键处理类
class WinHotkey : public QObject, public QAbstractNativeEventFilter
{
    Q_OBJECT

public:
    explicit WinHotkey(QObject *parent = nullptr);
    ~WinHotkey();

    // 注册热键
    bool registerHotkey(const QKeySequence &keySequence);
    
    // 取消注册热键
    bool unregisterHotkey();
    
    // 获取当前热键序列
    QKeySequence keySequence() const;
    
    // 本地事件过滤器，用于捕获热键事件
    bool nativeEventFilter(const QByteArray &eventType, void *message, qintptr *result) override;

signals:
    // 当热键被按下时触发信号
    void activated();

private:
    // 当前热键ID
    int hotkeyId;
    
    // 当前热键序列
    QKeySequence currentKeySequence;
    
    // 静态计数器，用于为热键生成唯一ID
    static int nextHotkeyId;
    
    // 静态映射表，用于查找热键ID对应的实例
    static QMap<int, WinHotkey*> hotkeyInstances;
    
    // 将Qt按键和修饰符转换为Windows API使用的代码
    static UINT keyToWindowsKey(Qt::Key key);
    static UINT modifiersToWindowsModifiers(Qt::KeyboardModifiers modifiers);
    static UINT qtKeyToWindowsKey(Qt::Key key);
};

#endif // WINHOTKEY_H 