/* 强制设置支持状态 */
void DdcController::setForcedSupportStatus(bool supported)
{
    isForceSet = true;
    forcedSupport = supported;
    
    /* 清除当前显示器列表 */
    displays.clear();
    
    /* 添加一个支持DDC/CI的虚拟显示器 */
    DisplayInfo info;
    info.index = 0;
    info.supportsDdc = supported;
    info.currentBrightness = 50; /* 默认亮度为50% */
    
    displays.append(info);
    
    qDebug() << "已强制设置DDC/CI支持状态为:" << supported;
} 