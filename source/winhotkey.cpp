#include "winhotkey.h"
#include <QApplication>
#include <QDebug>
#include <QGuiApplication>

// 初始化静态成员
int WinHotkey::nextHotkeyId = 1;
QMap<int, WinHotkey*> WinHotkey::hotkeyInstances;

WinHotkey::WinHotkey(QObject *parent)
    : QObject(parent)
    , hotkeyId(0)
{
    // 安装事件过滤器
    qApp->installNativeEventFilter(this);
}

WinHotkey::~WinHotkey()
{
    // 取消注册热键并移除事件过滤器
    unregisterHotkey();
    qApp->removeNativeEventFilter(this);
}

bool WinHotkey::registerHotkey(const QKeySequence &keySequence)
{
    // 如果已经注册了热键，先取消注册
    if (hotkeyId) {
        unregisterHotkey();
    }
    
    // 如果传入的序列为空，直接返回
    if (keySequence.isEmpty()) {
        return false;
    }
    
    // 提取第一个按键组合
    QKeyCombination keyCombination = keySequence[0];
    int key = keyCombination.key();
    Qt::KeyboardModifiers modifiers = keyCombination.keyboardModifiers();
    
    // 转换为Windows API使用的代码
    UINT winKey = keyToWindowsKey(static_cast<Qt::Key>(key));
    UINT winMods = modifiersToWindowsModifiers(modifiers);
    
    // 如果按键无效，直接返回
    if (winKey == 0) {
        return false;
    }
    
    // 分配新的热键ID
    hotkeyId = nextHotkeyId++;
    
    // 注册热键
    bool success = RegisterHotKey(nullptr, hotkeyId, winMods, winKey);
    
    if (success) {
        // 保存按键序列
        currentKeySequence = keySequence;
        
        // 将此实例添加到静态映射表
        hotkeyInstances.insert(hotkeyId, this);
        
        qDebug() << "成功注册热键:" << keySequence.toString() << "ID:" << hotkeyId;
    } else {
        hotkeyId = 0;
        qDebug() << "注册热键失败:" << keySequence.toString() << "错误代码:" << GetLastError();
    }
    
    return success;
}

bool WinHotkey::unregisterHotkey()
{
    // 如果没有注册热键，直接返回
    if (!hotkeyId) {
        return true;
    }
    
    // 注销热键并从映射表移除
    bool success = UnregisterHotKey(nullptr, hotkeyId);
    hotkeyInstances.remove(hotkeyId);
    
    if (success) {
        qDebug() << "成功注销热键 ID:" << hotkeyId;
    } else {
        qDebug() << "注销热键失败 ID:" << hotkeyId << "错误代码:" << GetLastError();
    }
    
    // 重置状态
    hotkeyId = 0;
    currentKeySequence = QKeySequence();
    
    return success;
}

QKeySequence WinHotkey::keySequence() const
{
    return currentKeySequence;
}

bool WinHotkey::nativeEventFilter(const QByteArray &eventType, void *message, qintptr *result)
{
    Q_UNUSED(result);
    
    // 检查是否是Windows消息
    if (eventType == "windows_generic_MSG" || eventType == "windows_dispatcher_MSG") {
        MSG *msg = static_cast<MSG*>(message);
        
        // 检查是否是热键消息
        if (msg->message == WM_HOTKEY) {
            int id = static_cast<int>(msg->wParam);
            
            // 查找对应的热键实例
            WinHotkey *hotkey = hotkeyInstances.value(id, nullptr);
            
            // 如果找到实例，触发信号
            if (hotkey) {
                emit hotkey->activated();
                return true;
            }
        }
    }
    
    return false;
}

UINT WinHotkey::keyToWindowsKey(Qt::Key key)
{
    // 将Qt的按键转换为Windows虚拟键码
    switch (key) {
        case Qt::Key_Escape:    return VK_ESCAPE;
        case Qt::Key_Tab:
        case Qt::Key_Backtab:   return VK_TAB;
        case Qt::Key_Backspace: return VK_BACK;
        case Qt::Key_Return:
        case Qt::Key_Enter:     return VK_RETURN;
        case Qt::Key_Insert:    return VK_INSERT;
        case Qt::Key_Delete:    return VK_DELETE;
        case Qt::Key_Pause:     return VK_PAUSE;
        case Qt::Key_Print:     return VK_PRINT;
        case Qt::Key_Clear:     return VK_CLEAR;
        case Qt::Key_Home:      return VK_HOME;
        case Qt::Key_End:       return VK_END;
        case Qt::Key_Left:      return VK_LEFT;
        case Qt::Key_Up:        return VK_UP;
        case Qt::Key_Right:     return VK_RIGHT;
        case Qt::Key_Down:      return VK_DOWN;
        case Qt::Key_PageUp:    return VK_PRIOR;
        case Qt::Key_PageDown:  return VK_NEXT;
        case Qt::Key_CapsLock:  return VK_CAPITAL;
        case Qt::Key_NumLock:   return VK_NUMLOCK;
        case Qt::Key_ScrollLock: return VK_SCROLL;
        case Qt::Key_F1:        return VK_F1;
        case Qt::Key_F2:        return VK_F2;
        case Qt::Key_F3:        return VK_F3;
        case Qt::Key_F4:        return VK_F4;
        case Qt::Key_F5:        return VK_F5;
        case Qt::Key_F6:        return VK_F6;
        case Qt::Key_F7:        return VK_F7;
        case Qt::Key_F8:        return VK_F8;
        case Qt::Key_F9:        return VK_F9;
        case Qt::Key_F10:       return VK_F10;
        case Qt::Key_F11:       return VK_F11;
        case Qt::Key_F12:       return VK_F12;
        case Qt::Key_F13:       return VK_F13;
        case Qt::Key_F14:       return VK_F14;
        case Qt::Key_F15:       return VK_F15;
        case Qt::Key_F16:       return VK_F16;
        case Qt::Key_F17:       return VK_F17;
        case Qt::Key_F18:       return VK_F18;
        case Qt::Key_F19:       return VK_F19;
        case Qt::Key_F20:       return VK_F20;
        case Qt::Key_F21:       return VK_F21;
        case Qt::Key_F22:       return VK_F22;
        case Qt::Key_F23:       return VK_F23;
        case Qt::Key_F24:       return VK_F24;
        case Qt::Key_Space:     return VK_SPACE;
        case Qt::Key_Asterisk:  return VK_MULTIPLY;
        case Qt::Key_Plus:      return VK_ADD;
        case Qt::Key_Comma:     return VK_SEPARATOR;
        case Qt::Key_Minus:     return VK_SUBTRACT;
        case Qt::Key_Slash:     return VK_DIVIDE;
        case Qt::Key_0:         return 0x30;
        case Qt::Key_1:         return 0x31;
        case Qt::Key_2:         return 0x32;
        case Qt::Key_3:         return 0x33;
        case Qt::Key_4:         return 0x34;
        case Qt::Key_5:         return 0x35;
        case Qt::Key_6:         return 0x36;
        case Qt::Key_7:         return 0x37;
        case Qt::Key_8:         return 0x38;
        case Qt::Key_9:         return 0x39;
        case Qt::Key_A:         return 0x41;
        case Qt::Key_B:         return 0x42;
        case Qt::Key_C:         return 0x43;
        case Qt::Key_D:         return 0x44;
        case Qt::Key_E:         return 0x45;
        case Qt::Key_F:         return 0x46;
        case Qt::Key_G:         return 0x47;
        case Qt::Key_H:         return 0x48;
        case Qt::Key_I:         return 0x49;
        case Qt::Key_J:         return 0x4A;
        case Qt::Key_K:         return 0x4B;
        case Qt::Key_L:         return 0x4C;
        case Qt::Key_M:         return 0x4D;
        case Qt::Key_N:         return 0x4E;
        case Qt::Key_O:         return 0x4F;
        case Qt::Key_P:         return 0x50;
        case Qt::Key_Q:         return 0x51;
        case Qt::Key_R:         return 0x52;
        case Qt::Key_S:         return 0x53;
        case Qt::Key_T:         return 0x54;
        case Qt::Key_U:         return 0x55;
        case Qt::Key_V:         return 0x56;
        case Qt::Key_W:         return 0x57;
        case Qt::Key_X:         return 0x58;
        case Qt::Key_Y:         return 0x59;
        case Qt::Key_Z:         return 0x5A;
        default:                return 0;
    }
}

UINT WinHotkey::modifiersToWindowsModifiers(Qt::KeyboardModifiers modifiers)
{
    // 将Qt的修饰符转换为Windows修饰符
    UINT winMods = 0;
    
    if (modifiers & Qt::ShiftModifier) {
        winMods |= MOD_SHIFT;
    }
    
    if (modifiers & Qt::ControlModifier) {
        winMods |= MOD_CONTROL;
    }
    
    if (modifiers & Qt::AltModifier) {
        winMods |= MOD_ALT;
    }
    
    if (modifiers & Qt::MetaModifier) {
        winMods |= MOD_WIN;
    }
    
    return winMods;
}
