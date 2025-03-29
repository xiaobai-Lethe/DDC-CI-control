#include "mainwindow.h"

#include <QApplication>
#include <QLocale>
#include <QTranslator>
#include <QLibraryInfo>
#include <QDebug>
#include <QSharedMemory>
#include <QMessageBox>

int main(int argc, char *argv[])
{
    /* 创建应用程序对象 */
    QApplication a(argc, argv);
    
    /* 设置应用程序信息 */
    QApplication::setApplicationName("DDC/CI control");
    QApplication::setApplicationDisplayName("DDC/CI control");
    QApplication::setApplicationVersion("1.0.0");
    QApplication::setOrganizationName("DDC/CI");
    
    /* 单实例检测 */
    QSharedMemory sharedMemory("DDC_CI_CONTROL_SINGLE_INSTANCE");
    if (!sharedMemory.create(1)) {
        QMessageBox::warning(nullptr, QObject::tr("警告"), 
                            QObject::tr("程序已经在运行中，不能同时运行多个实例！"));
        return 0;
    }
    
    /* 加载默认翻译 */
    QTranslator translator;
    const QStringList uiLanguages = QLocale::system().uiLanguages();
    
    qDebug() << "系统UI语言:" << uiLanguages;
    
    /* 尝试加载系统语言翻译 */
    bool translationLoaded = false;
    for (const QString &locale : uiLanguages) {
        const QString baseName = "display_" + QLocale(locale).name();
        if (translator.load(":/i18n/" + baseName)) {
            a.installTranslator(&translator);
            qDebug() << "已加载翻译:" << baseName;
            translationLoaded = true;
            break;
        }
    }
    
    /* 如果没有加载到翻译，默认使用中文 */
    if (!translationLoaded) {
        if (translator.load(":/i18n/display_zh_CN")) {
            a.installTranslator(&translator);
            qDebug() << "已加载默认中文翻译";
        } else {
            qDebug() << "无法加载默认中文翻译";
        }
    }
    
    /* 创建并显示主窗口 */
    MainWindow w;
    w.show();
    
    /* 进入应用程序事件循环 */
    return a.exec();
}
