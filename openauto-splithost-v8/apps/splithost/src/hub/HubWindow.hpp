#pragma once
#include <QWidget>
class QWebEngineView; class QWebChannel; class ModuleManager; class WebBridge;
class HubWindow : public QWidget {
    Q_OBJECT
public:
    explicit HubWindow(const QString& wwwDir, const QString& modulesDir, QWidget* parent=nullptr);
    ~HubWindow();
private:
    QWebEngineView* view_{}; QWebChannel* channel_{}; ModuleManager* modules_{}; WebBridge* bridge_{};
    QString wwwDir_; QString modulesDir_;
};
