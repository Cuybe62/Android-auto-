#include "HubWindow.hpp"
#include "ModuleManager.hpp"
#include "WebBridge.hpp"
#include <QVBoxLayout>
#include <QWebEngineView>
#include <QWebChannel>
#include <QFileInfo>
#include <QUrl>
#include <QDir>
HubWindow::HubWindow(const QString& wwwDir, const QString& modulesDir, QWidget* parent)
    : QWidget(parent), wwwDir_(wwwDir), modulesDir_(modulesDir) {
    setWindowTitle("SplitHost — Hub"); resize(1000, 700);
    auto* lay = new QVBoxLayout(this);
    auto* view = new QWebEngineView(this);
    auto* channel = new QWebChannel(this);
    auto* bridge = new WebBridge(this);
    channel->registerObject(QStringLiteral("hub"), bridge);
    view->page()->setWebChannel(channel);
    auto* mgr = new ModuleManager(modulesDir_, this);
    bridge->setModules(mgr->modules());
    connect(mgr, &ModuleManager::moduleUpdated, bridge, &WebBridge::forwardModuleUpdate);
    connect(mgr, &ModuleManager::moduleError, bridge, &WebBridge::forwardModuleError);
    lay->addWidget(view);
    QFileInfo idx(QDir(wwwDir_).filePath("index.html"));
    if (idx.exists()) view->setUrl(QUrl::fromLocalFile(idx.absoluteFilePath()));
    else view->setHtml("<h2>Hub — aucun index.html</h2><p>Build le web (scripts/build_hub_web.sh).</p>");
}
HubWindow::~HubWindow() {}
