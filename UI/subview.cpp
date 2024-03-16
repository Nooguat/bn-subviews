#include "subview.h"
#include <QLayout>
#include <QApplication>
#include <QList>
#include <QObject>
#include <QMainWindow>
#include <QMenu>
#include "binaryninjaapi.h"
#include "binaryninjacore.h"
#include "ui/action.h"
#include "ui/linearview.h"
#include "ui/pane.h"
#include "ui/render.h"
#include "ui/scriptingconsole.h"
#include "ui/sidebar.h"
#include <fmt/core.h>
#include <fmt/ranges.h>
#include <ios>
#include <qalgorithms.h>
#include <qboxlayout.h>
#include <qbrush.h>
#include <qdialog.h>
#include <qgesturerecognizer.h>
#include <qglobal.h>
#include <qgridlayout.h>
#include <qlcdnumber.h>
#include <qlist.h>
#include <qmainwindow.h>
#include <qmessagebox.h>
#include <qnamespace.h>
#include <qsizepolicy.h>
#include <qsplitter.h>
#include <stack>
#include <ui/viewframe.h>
#include <vector>
#include "DockableSidebar.h"
#include "BWindow.h"
#include "binaryninja-api/ui/linearview.h"
#include "ui/sidebaricons.h"
#include "ui/stackview.h"
#include "ui/tabwidget.h"
#include "ui/typeview.h"
#include "ui/uicontext.h"
#include "ui/uitypes.h"
#include "ui/disassemblyview.h"

// TODO Fix the different crashes
// TODO Add an option to enable/disable the module
SubView* SubView::m_instance = nullptr;
bool SubView::m_hide = false;
DockableTabCollection* collection = new DockableTabCollection();
auto disabled = BinaryNinja::Metadata(false);
auto enabled = BinaryNinja::Metadata(true);

// TODO Expose those as settings in BN
std::set<QString> disabled_views = {"Console", "Log", "Other (Compatibility)", "Mini Graph", "Stack Trace", "Debugger Modules", "Find"};
// Need to hardcode those names this since there are not part of v3 GlobalArea anymore
std::set<QString> area_wigets = {"ScriptingConsole", "LogView"};
std::set<QString> refreshable_views = {"StackView", "LinearView", "VariableList"};
QLayout* findParentLayout(QWidget* w, QLayout* topLevelLayout)
{
    for (QObject* qo: topLevelLayout->children())
    {
        QLayout* layout = qobject_cast<QLayout*>(qo);
        if (layout != nullptr)
        {
            if (layout->indexOf(w) > -1)
                return layout;
            else if (!layout->children().isEmpty())
            {
                layout = findParentLayout(w, layout);
                if (layout != nullptr)
                    return layout;
            }
        }
    }
    return nullptr;
}

QLayout* findParentLayout(QWidget* w)
{
    if (w->parentWidget() != nullptr)
        if (w->parentWidget()->layout() != nullptr)
            return findParentLayout(w, w->parentWidget()->layout());
    return nullptr;
}


QMenu* findMenu(QList<QAction*> actions, QString name){
    foreach (QAction *action, actions){
        if(action->menu()){
            if(QString::compare(name, action->text()) == 0){
                return action->menu();
            }
            findMenu(action->menu()->actions(), name);
        }
    }
    return nullptr;
}

QObject* get_parent_from_type(QObject* base, std::string className){
    QObject* parent = base;
    while(parent->parent() != nullptr){
        if(className.compare(parent->metaObject()->className()) == 0){
            qDebug() << "found parent !";
            return parent;
        }
        qDebug() << parent->metaObject()->className();
        parent = parent->parent();
    }
    return nullptr;
}

SubViewBarStyle::SubViewBarStyle() : DockableTabStyle()
{}

void SubViewBarStyle::paintTab(const QWidget* widget, QStylePainter& p, const DockableTabInfo& info, int idx, int count, int active, DockableTabInteractionState state, const QRect& rect) const{

// TODO Implement this
}

void SubView::reloadModule(const UIActionContext& action){
    UIContext* context = action.context;
    BinaryViewRef bv = context->getAvailableBinaryViews()[0].first;
}
void SubView::hideSidebar(const UIActionContext& action){
    UIContext* context = action.context;
    QMainWindow* main = context->mainWindow();
    auto children = main->findChildren<SidebarIconsWidget*>();
    for(auto child : children){
        child->setVisible(m_hide);
    }
    auto children_widgets = main->findChildren<SidebarWidgetAndHeader*>();

    for(auto child : children_widgets){
        if(area_wigets.find(child->widget()->metaObject()->className())!= area_wigets.end()){
            continue;
        }
        child->widget()->setVisible(m_hide);
        child->header()->setVisible(m_hide);
    }
    m_hide = !m_hide;
    return;
}

TabPane::TabPane(SplitTabWidget* container, QWidget* widget, QString name, PaneHeader* default_header) : Pane(container){
    if(default_header == nullptr){
        auto header = new WidgetPaneHeader("");
        Pane::init(header);
    }
    else{
        Pane::init(default_header);
    }
    container->addTab(widget, name);
    container->update();
    m_container = container;
    auto first = collection->containers().begin();
    auto tabwidget = *first;
    auto tabbar = tabwidget->tabBar();
    // auto style = new SubViewBarStyle();
    // tabbar->setTabStyle(style);

}

void TabPane::updateStatus(){
    updateWidgetStatus();
    return Pane::updateStatus();
}

void SubView::init()
{

    m_instance = new SubView();
    // TODO Update disabled views here from settings
    UIContext::registerNotification(m_instance);
    UIAction::registerAction("Hide Sidebar");
    UIAction::registerAction("Toggle Subviews");
    auto actions = UIActionHandler::globalActions();
    actions->bindAction("Hide Sidebar", UIAction(SubView::hideSidebar));
    actions->bindAction("Toggle Subviews", UIAction(SubView::reloadModule));
}



void SubView::addView(UIContext *context, QString viewtype){
    QMainWindow* main = context->mainWindow();
    ViewFrame* frame = context->getCurrentViewFrame();
    BinaryViewRef bv = context->getAvailableBinaryViews()[0].first;
    if(frame == nullptr){
        qDebug() << "frame is null";
        return;
    }
    auto vp = frame->parent();
    if(vp == nullptr){
        qDebug() << "vp is null";
        return;
    }
    SplitPaneContainer* container = static_cast<SplitPaneContainer*>(vp->parent());
    if(container == nullptr){
        qDebug() << "container is null";
        return;
    }
    auto sidebar = context->sidebar();
    auto types = sidebar->types();
    for(auto type: types){
        if(type->name().compare(viewtype) == 0){
        QSplitter* view = new QSplitter();
        view->setOrientation(Qt::Orientation::Vertical);
        auto newview = type->createWidget(frame, bv);
        auto header = newview->headerWidget();
        // This needs to be called in this order,
        // trying to call setVisible afterwards makes everything
        // segfault
        if(header != nullptr){
            header->setVisible(true);
            view->addWidget(header);
        }
        view->addWidget(newview);
        TabPane* newpane = new TabPane(new SplitTabWidget(collection), view, type->name());
        container->open(newpane, Qt::Orientation::Horizontal);
        }
    }

}

void SubView::openCFGView(UIContext *context, ViewFrame *vf){
    BinaryViewRef bv = context->getAvailableBinaryViews()[0].first;
    auto vp = vf->parent();
    QMainWindow* main = context->mainWindow();
    auto linearviews = main->findChildren<LinearView*>();
    LinearView* linearview;
    if(linearviews.size() != 0){
        qDebug() << "found linear views";
        linearview = linearviews[0];
    }
    auto flowGraph = new DisassemblyContainer((QWidget*)vf->parent(), bv, vf, linearview->getCurrentFunction());
    SplitPaneContainer* container = static_cast<SplitPaneContainer*>(vp->parent());
    SplitPaneContainer* split_container = static_cast<SplitPaneContainer*>(vf->parent()->parent());
    TabPane* newpane = new TabPane(new SplitTabWidget(collection), flowGraph, "FlowGraph");
    container->open(newpane, Qt::Orientation::Horizontal);
}

void SubView::openLinearView(UIContext *context, ViewFrame* vf){
    BinaryViewRef bv = context->getAvailableBinaryViews()[0].first;
    SplitPaneContainer* split_container = static_cast<SplitPaneContainer*>(vf->parent()->parent());
    QMainWindow* main = context->mainWindow();
    auto linearviews = main->findChildren<LinearView*>();
    auto container = new SplitTabWidget(collection);
    int counter = 0;
    for(auto linear : linearviews){
        QString viewname = QString("LinearView %1").arg(QString::number(counter++));
        auto header_status = linear->getStatusBarWidget();
        auto header_options = linear->getHeaderOptionsWidget();
        auto header_subtypes = linear->getHeaderSubtypeWidget();
        QSplitter* widget = new QSplitter();
        QSplitter* header = new QSplitter();
        header->addWidget(header_options);
        header->addWidget(header_status);

        widget->setOrientation(Qt::Orientation::Vertical);
        widget->addWidget(header);
        widget->addWidget(linear);
        TabPane* newpane = new TabPane(container, widget, viewname.toStdString().c_str());
        // header_status->setParent(newpane);
        // header_options->setParent(newpane);
        // header_subtypes->setParent(newpane);
        split_container->open(newpane, Qt::Orientation::Horizontal);
        // Needed to remove all references to previous splitpanecontainer
        QWidget* vp = (QWidget*)vf->parent();
        QWidget* vpp = (QWidget*)vp->parent();
        for(auto child : split_container->children()){
            qDebug() << child->metaObject()->className();
        }
        auto controller = (QWidget*)split_container->parent();
        controller->update();
        controller->updateGeometry();
        split_container->layout()->update();
        vp->close();
        vpp->close();
        // linear->close();
    }
}

void SubView::OnAfterOpenFile(UIContext* context, FileContext* file, ViewFrame* frame){
    auto bvs = context->getAvailableBinaryViews();
    if(!bvs.empty()){
        auto bv = bvs[0].first;
        auto scope = SettingsResourceScope;
        auto settings = BinaryNinja::Settings::Instance();
        settings->RegisterGroup("subviews", "Subview");
        settings->RegisterSetting("subviews.enabled",R"({
        "title" : "Toggle the usage of subviews module",
        "type" : "number",
        "description" : "Toggle the usage of subview module",
        "ignore" : ["SettingsProjectScope", "SettingsUserScope"],
        "requiresRestart" : true
        })");
        // TODO Add the disabled views array setting
        // settings->RegisterSetting("subviews.diabled_views", R"({

        // })");
        auto val = settings->Get<int64_t>("subviews.enabled", bv);
        if(val == 2){
            // This crashes, find why
            // UIContext::unregisterNotification(m_instance);
            return;
        }
        // settings->Set("subviews.enabled", 10, bv, scope);
    }
    QMenuBar* menuBar = context->mainWindow()->menuBar();
    QMainWindow* main = context->mainWindow();
    QMenu* views = findMenu(menuBar->actions(), "View");
    if(views == nullptr){
        return;
    }
    QMenu* subviews = new QMenu("Subviews");
    auto sidebar = context->sidebar();
    auto types = sidebar->types();
    for(auto type: types){
        if(disabled_views.find(type->name()) != disabled_views.end()){
            continue;
        }
        QAction *openSubView = new QAction(type->name());
        QObject::connect(openSubView, &QAction::triggered, new QWidget(), [=](){
        QString actionName = QString("Open ") + type->name();
        UIAction::registerAction(actionName);
        auto actions = UIActionHandler::globalActions();
        actions->bindAction(actionName, UIAction(SubView::hideSidebar));
        addView(context, type->name());
        });
        subviews->addAction(openSubView);
    }
    // Add open linear view
    QAction *openSubView = new QAction("Linear View");
    QObject::connect(openSubView, &QAction::triggered, new QWidget(), [=](){
        openLinearView(context, context->getCurrentViewFrame());
    });
    QString actionName = QString("Open Linear View");
    UIAction::registerAction(actionName);
    subviews->addAction(openSubView);
    views->addMenu(subviews);
    // Add open cfg view
    QAction *openCFG = new QAction("CFG View");
    QObject::connect(openCFG, &QAction::triggered, new QWidget(), [=](){
        openCFGView(context, context->getCurrentViewFrame());
    });
    QString actionNameCFG = QString("Open CFG");
    UIAction::registerAction(actionNameCFG);
    subviews->addAction(openCFG);
    views->addMenu(subviews);
    openLinearView(context, frame);
}

void SubView::OnContextOpen(UIContext* context)
{
}

void SubView::OnContextClose(UIContext* context)
{
    // TODO Clean up all views
}

void SubView::OnViewChange(UIContext *context, ViewFrame *frame, const QString &type)
{
    auto bvs = context->getAvailableBinaryViews();
    QMainWindow* main = context->mainWindow();
    // TODO Get the last focused linearview -> implement this in the onClick
    // event for LinearView
    auto linearviews = main->findChildren<LinearView*>();
    LinearView* linearview;
    if(linearviews.size() != 0){
        linearview = linearviews[0];
    }
    auto subviews = main->findChildren<TabPane*>();
    for(auto subview : subviews){
        auto tab = subview->findChild<SplitTabWidget*>();
        auto widgets = subview->findChild<DockableTabWidget*>();
        if(widgets->count() == 0){
            QWidget* parent = static_cast<QWidget*>(subview->parent());
            parent->close();
        }
    }

    // Working, need to find a more global way to work
    // Override onclick event for linearview
    auto CFGWidgets = main->findChildren<DisassemblyContainer*>();
    if(CFGWidgets.size() != 0){
        for(auto widget : CFGWidgets){
            auto disass = widget->getDisassembly();
            // Those two are needed
            widget->setCurrentFunction(linearview->getCurrentFunction());
            disass->navigateToFunction(linearview->getCurrentFunction(), 0);
            widget->repaint();
        }
    }
}

