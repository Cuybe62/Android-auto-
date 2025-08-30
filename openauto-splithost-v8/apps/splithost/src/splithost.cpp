#include <QtWidgets>
#include <QWindow>
#include <QProcess>
#include <QTimer>
#include <xcb/xcb.h>
#include <xcb/xproto.h>
#include <optional>
#include <cstring>
#include "hub/HubWindow.hpp"

static std::optional<xcb_atom_t> internAtom(xcb_connection_t* c, const char* name) {
    auto cookie = xcb_intern_atom(c, 0, std::strlen(name), name);
    xcb_intern_atom_reply_t* reply = xcb_intern_atom_reply(c, cookie, nullptr);
    if (!reply) return std::nullopt;
    xcb_atom_t atom = reply->atom;
    free(reply);
    return atom;
}
static QByteArray getWindowProperty(xcb_connection_t* c, xcb_window_t win, xcb_atom_t prop, xcb_atom_t type) {
    auto cookie = xcb_get_property(c, 0, win, prop, type, 0, 1024);
    xcb_get_property_reply_t* reply = xcb_get_property_reply(c, cookie, nullptr);
    if (!reply) return {};
    int len = xcb_get_property_value_length(reply);
    QByteArray out; if (len > 0) out = QByteArray((const char*)xcb_get_property_value(reply), len);
    free(reply); return out;
}
static QList<xcb_window_t> topLevelWindows(xcb_connection_t* c) {
    QList<xcb_window_t> out;
    const xcb_setup_t* setup = xcb_get_setup(c);
    xcb_screen_iterator_t it = xcb_setup_roots_iterator(setup);
    if (it.rem == 0) return out;
    xcb_screen_t* screen = it.data; xcb_window_t root = screen->root;
    auto treeCookie = xcb_query_tree(c, root);
    xcb_query_tree_reply_t* tree = xcb_query_tree_reply(c, treeCookie, nullptr);
    if (!tree) return out;
    int len = xcb_query_tree_children_length(tree);
    xcb_window_t* children = xcb_query_tree_children(tree);
    for (int i = 0; i < len; ++i) out.push_back(children[i]);
    free(tree); return out;
}
static std::optional<xcb_window_t> findWindowByPidOrTitle(xcb_connection_t* c, uint32_t pid, const QString& titleHint) {
    auto netWmPidOpt = internAtom(c, "_NET_WM_PID");
    auto netWmNameOpt = internAtom(c, "_NET_WM_NAME");
    auto utf8StrOpt = internAtom(c, "UTF8_STRING");
    auto wmNameOpt = internAtom(c, "WM_NAME");
    if (!netWmPidOpt) return std::nullopt;
    xcb_atom_t NET_WM_PID=*netWmPidOpt, NET_WM_NAME=netWmNameOpt.value_or(XCB_ATOM_NONE),
               UTF8=utf8StrOpt.value_or(XCB_ATOM_STRING), WM_NAME=wmNameOpt.value_or(XCB_ATOM_WM_NAME);
    for (auto w: topLevelWindows(c)) {
        QByteArray pidData = getWindowProperty(c, w, NET_WM_PID, XCB_ATOM_CARDINAL);
        if (pidData.size() >= 4) {
            uint32_t wpid = *reinterpret_cast<const uint32_t*>(pidData.constData());
            if (wpid == pid && pid != 0) return w;
        }
        if (!titleHint.isEmpty()) {
            QByteArray name = getWindowProperty(c, w, NET_WM_NAME, UTF8);
            if (name.isEmpty()) name = getWindowProperty(c, w, WM_NAME, XCB_ATOM_STRING);
            QString qname = QString::fromUtf8(name);
            if (qname.contains(titleHint, Qt::CaseInsensitive)) return w;
        }
    }
    return std::nullopt;
}

class CustomPanel : public QWidget {
    Q_OBJECT
public:
    explicit CustomPanel(QWidget* parent=nullptr): QWidget(parent) {
        auto* lay = new QVBoxLayout(this);
        auto* title = new QLabel("Interface personnalisée"); title->setAlignment(Qt::AlignCenter);
        title->setStyleSheet("font-size:22px;font-weight:600;");
        auto* grid = new QGridLayout();
        auto* micLbl = new QLabel("Micro USB détecté:"); micStatus = new QLabel("(inconnu)");
        auto* routeLbl = new QLabel("Sortie audio:"); audioOut = new QLabel("Jack 3.5mm");
        grid->addWidget(micLbl,0,0); grid->addWidget(micStatus,0,1);
        grid->addWidget(routeLbl,1,0); grid->addWidget(audioOut,1,1);
        auto* btnBar = new QHBoxLayout();
        auto* leftFull = new QPushButton("Plein écran: OpenAuto");
        auto* rightFull= new QPushButton("Plein écran: Interface");
        auto* splitBtn = new QPushButton("Vue partagée");
        auto* hubBtn   = new QPushButton("Ouvrir Hub");
        btnBar->addWidget(leftFull); btnBar->addWidget(rightFull);
        btnBar->addWidget(splitBtn); btnBar->addWidget(hubBtn);
        lay->addWidget(title); lay->addLayout(grid); lay->addStretch(); lay->addLayout(btnBar);
        connect(leftFull,&QPushButton::clicked,this,[this]{ emit requestMode(Mode::LeftFull); });
        connect(rightFull,&QPushButton::clicked,this,[this]{ emit requestMode(Mode::RightFull); });
        connect(splitBtn,&QPushButton::clicked,this,[this]{ emit requestMode(Mode::Split); });
        connect(hubBtn,&QPushButton::clicked,this,[this]{ emit openHubRequested(); });
    }
    enum class Mode { Split, LeftFull, RightFull };
signals: void requestMode(Mode); void openHubRequested();
public slots: void setMicStatus(const QString& s){ micStatus->setText(s);} void setAudioOut(const QString& s){ audioOut->setText(s);}
private: QLabel* micStatus{}; QLabel* audioOut{};
};

class HubWindow; // fwd

class SplitHost : public QMainWindow {
    Q_OBJECT
public:
    SplitHost(QString openautoPath, QString titleHint, QString hubWww, QString hubModules, QWidget* parent=nullptr)
    : QMainWindow(parent), openautoPath_(openautoPath), titleHint_(titleHint), hubWww_(hubWww), hubModules_(hubModules) {
        setWindowTitle("SplitHost — OpenAuto + UI"); resize(1280,720);
        splitter_=new QSplitter(this); splitter_->setHandleWidth(6); setCentralWidget(splitter_);
        leftPlaceholder_=new QWidget(); leftPlaceholder_->setStyleSheet("background:#111;color:#bbb");
        auto* l=new QVBoxLayout(leftPlaceholder_); auto* lbl=new QLabel("En attente d'OpenAuto…"); lbl->setAlignment(Qt::AlignCenter);
        l->addStretch(); l->addWidget(lbl); l->addStretch();
        panel_=new CustomPanel();
        splitter_->addWidget(leftPlaceholder_); splitter_->addWidget(panel_);
        splitter_->setStretchFactor(0,3); splitter_->setStretchFactor(1,2);
        connect(panel_,&CustomPanel::requestMode,this,&SplitHost::onModeRequested);
        connect(panel_,&CustomPanel::openHubRequested,this,[this]{ if(!hub_) hub_=new HubWindow(hubWww_,hubModules_,this); hub_->show(); hub_->raise(); hub_->activateWindow(); });
        if(!openautoPath_.isEmpty()){ child_.setProgram(openautoPath_); child_.setProcessChannelMode(QProcess::ForwardedChannels); child_.start(); }
        pollTimer_.setInterval(200); connect(&pollTimer_,&QTimer::timeout,this,&SplitHost::tryEmbedOnce); pollTimer_.start();
        QTimer::singleShot(1000,this,[this]{ panel_->setMicStatus("OK (USB)"); });
    }
    ~SplitHost(){ if(child_.state()!=QProcess::NotRunning){ child_.terminate(); if(!child_.waitForFinished(1500)) child_.kill(); } }
private slots:
    void onModeRequested(CustomPanel::Mode m){
        if(!embedded_) return;
        if(m==CustomPanel::Mode::Split){
            if(!embeddedContainer_->parent()) embeddedContainer_->setParent(splitter_);
            if(!panel_->parent()) panel_->setParent(splitter_);
            splitter_->insertWidget(0,embeddedContainer_); splitter_->insertWidget(1,panel_);
            splitter_->setSizes({800,480});
        } else if(m==CustomPanel::Mode::LeftFull){
            embeddedContainer_->setParent(this); setCentralWidget(embeddedContainer_);
        } else if(m==CustomPanel::Mode::RightFull){
            panel_->setParent(this); setCentralWidget(panel_);
        }
    }
    void tryEmbedOnce(){
        if(embedded_){ pollTimer_.stop(); return; }
        int scr=0; xcb_connection_t* c = xcb_connect(nullptr,&scr);
        if(xcb_connection_has_error(c)){ xcb_disconnect(c); return; }
        uint32_t pid=0; if(child_.state()!=QProcess::NotRunning) pid=(uint32_t)child_.processId();
        auto wopt = findWindowByPidOrTitle(c,pid,titleHint_);
        if(wopt.has_value()){
            xcb_window_t wid=*wopt; QWindow* foreign=QWindow::fromWinId((WId)wid);
            embeddedContainer_=QWidget::createWindowContainer(foreign); embeddedContainer_->setFocusPolicy(Qt::StrongFocus);
            splitter_->replaceWidget(0,embeddedContainer_); leftPlaceholder_->deleteLater(); embedded_=true; pollTimer_.stop();
        }
        xcb_disconnect(c);
    }
private:
    QString openautoPath_, titleHint_, hubWww_, hubModules_;
    QSplitter* splitter_{}; QWidget* leftPlaceholder_{}; CustomPanel* panel_{};
    QProcess child_; QTimer pollTimer_; bool embedded_=false; QWidget* embeddedContainer_{}; HubWindow* hub_=nullptr;
};

static void printHelpAndExit(const char* argv0){
    fprintf(stderr,
        "Usage: %s [--openauto /path/to/autoapp] [--title SUBSTR] [--hub-www DIR] [--hub-modules DIR]\\n\\n"
        "  --openauto PATH   Path to OpenAuto (autoapp). If omitted, attach to existing.\\n"
        "  --title SUBSTR    Window title substring match.\\n"
        "  --hub-www DIR     Path to local hub web assets (index.html).\\n"
        "  --hub-modules DIR Path to scripts producing JSON.\\n\\n", argv0);
    std::exit(1);
}

#include "hub/HubWindow.hpp"

int main(int argc, char** argv){
    QApplication app(argc, argv);
#if QT_VERSION >= QT_VERSION_CHECK(5,6,0)
    QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
#endif
    QString oaPath; QString titleHint="OpenAuto";
    QString hubWww = qEnvironmentVariable("HUB_WWW_DIR");
    QString hubModules = qEnvironmentVariable("HUB_MODULE_DIR");
    for(int i=1;i<argc;++i){ QString a=argv[i];
        if(a=="--openauto"&&i+1<argc) oaPath=argv[++i];
        else if(a=="--title"&&i+1<argc) titleHint=argv[++i];
        else if(a=="--hub-www"&&i+1<argc) hubWww=argv[++i];
        else if(a=="--hub-modules"&&i+1<argc) hubModules=argv[++i];
        else if(a=="-h"||a=="--help") printHelpAndExit(argv[0]);
    }
    SplitHost w(oaPath,titleHint,hubWww,hubModules); w.show(); return app.exec();
}

#include "moc_splithost.cpp"
