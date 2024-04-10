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
#include "ui/sidebar.h"
#include <cstdint>
#include <fmt/core.h>
#include <fmt/ranges.h>
#include <qaccessible_base.h>
#include <qalgorithms.h>
#include <qboxlayout.h>
#include <qbrush.h>
#include <qcoreevent.h>
#include <qdialog.h>
#include <qgesturerecognizer.h>
#include <qglobal.h>
#include <qgridlayout.h>
#include <qlcdnumber.h>
#include <qlist.h>
#include <qmainwindow.h>
#include <qmessagebox.h>
#include <qnamespace.h>
#include <qobject.h>
#include <qshortcut.h>
#include <qsizepolicy.h>
#include <qsplitter.h>
#include <qtextformat.h>
#include <qwidget.h>
#include <system_error>
#include <ui/viewframe.h>
#include <vector>
#include "BWindow.h"
#include "binaryninja-api/ui/linearview.h"
#include "ui/sidebaricons.h"
#include "ui/sidebarwidget.h"
#include "ui/splitter.h"
#include "ui/stackview.h"
#include "ui/tabwidget.h"
#include "ui/uicontext.h"
#include "ui/uitypes.h"
#include "ui/disassemblyview.h"
#include "ui/variablelist.h"

// TODO Fix the different crashes
// Crashing when moving the containers
SubView* SubView::m_instance = nullptr;
bool SubView::m_hide = false;
DockableTabCollection* collection = new DockableTabCollection();

std::set<QString> disabled_views;
std::set<QString> refresh_actions;
std::set<QString> protect_duplication;
std::set<DockableTabWidget*> tabs;
std::set<SplitPaneContainer*> containers;
// Hardcoded, cannot use GlobalArea as in the v3
std::set<QString> area_wigets = {"ScriptingConsole", "LogView"};

QObject* get_parent_from_type(QObject* base, std::string className){
    QObject* parent = base->parent();
    while(parent != nullptr){
        // qDebug() << parent;
        if(className.compare(parent->metaObject()->className()) == 0){
            return parent;
        }
        parent = parent->parent();
    }
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

SubViewBarStyle::SubViewBarStyle() : DockableTabStyle()
{}

ContainerFilter::ContainerFilter(QMainWindow* main) : QObject(){
    m_main = main;
}

bool ContainerFilter::eventFilter(QObject* watched, QEvent *event){
    if(event->type() == QEvent::Destroy){
    //     qDebug() << "destroyed called ";
    //     auto tabcontainer = (DockableTabWidget*)get_parent_from_type(watched, "DockableTabWidget");
    //     auto container = (SplitTabWidget*)get_parent_from_type(watched, "SplitTabWidget");
    //     auto widget = (QWidget*)watched;
    //     // qDebug() << "trying to close container, need to clean up the tabs first";
    //     event->accept();
    //     qDebug() << "before unregister " << tabcontainer->count();
    //     collection->unregisterContainer(tabcontainer);
    //     qDebug() << tabcontainer->indexOf(widget);
    //     auto index = tabcontainer->indexOf(widget);
    //     qDebug() << tabcontainer->widget(index);
    //     tabcontainer->removeTab(index);
    //     qDebug() << "after unregister " << tabcontainer->count();
    //     qDebug() << tabcontainer->widget(index);
        // return false;
    }
    // event->ignore();
    return true;
}

ClickFilter::ClickFilter(QMainWindow* main) : QObject(){
    m_main = main;
}


bool ClickFilter::eventFilter(QObject *watched, QEvent *event){
    if(event->type() == QEvent::MouseButtonRelease){
        auto linearview = static_cast<LinearView*>(watched);
        auto CFGWidgets = m_main->findChildren<DisassemblyContainer*>();
        if(CFGWidgets.size() != 0){
            for(auto widget : CFGWidgets){
                auto disass = widget->getDisassembly();
                // Those two are needed
                widget->setCurrentFunction(linearview->getCurrentFunction());
                disass->navigateToFunction(linearview->getCurrentFunction(), 0);
                widget->repaint();
            }
        }
        linearview->notifyRefresh();
        auto container = (SplitPaneWidget*)get_parent_from_type(watched, "SplitPaneWidget");
        auto vf = (ViewFrame*)get_parent_from_type(watched, "ViewFrame");
        if(container == nullptr){
            return false;
        }
        for(auto widget : container->findChildren<SidebarWidget*>()){
            widget->notifyViewChanged(vf);
            widget->notifyViewLocationChanged(vf->getCurrentViewInterface(), vf->getViewLocation());
        }
        auto handler = UIActionHandler::globalActions();
        for(auto action : refresh_actions){
            handler->executeAction(action);
        }
   }
    return false;
}


void SubViewBarStyle::paintTab(const QWidget* widget, QStylePainter& p, const DockableTabInfo& info, int idx, int count, int active, DockableTabInteractionState state, const QRect& rect) const{

// TODO Implement this
}

void SubView::hideSidebar(const UIActionContext& action){
    UIContext* context = action.context;
    QMainWindow* main = context->mainWindow();
    auto children = main->findChildren<SidebarIconsWidget*>();
    for(auto child : children){
        child->setVisible(m_hide);
    }
    auto children_widgets = main->findChildren<SidebarWidgetAndHeader*>();
;
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

void TabPane::endPane(){
    qDebug() << "called";
    this->removeEventFilter(m_filter);
    m_container->removeTab(m_widget);
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
    connect(this, &Pane::paneCloseRequested, this, &TabPane::endPane);
    auto main = (QMainWindow*)get_parent_from_type(container, "MainWindow");
    m_container = container;
    m_container->addTab(widget, name);
    m_widget = widget;
    m_filter = new ContainerFilter(main);
    // auto first = collection->containers().begin();
    // auto tabwidget = *first;
    // auto tabbar = tabwidget->tabBar();
    this->installEventFilter(m_filter);
    // auto style = new SubViewBarStyle();
    // tabbar->setTabStyle(style);
}

void TabPane::updateStatus(){
    updateWidgetStatus();
    return Pane::updateStatus();
}

bool TabPane::event(QEvent* event){
    return QWidget::event(event);
}

ViewFrame* TabPane::viewFrame() const{
    auto widget = this->widget();
    auto viewframe = widget->findChild<ViewFrame*>();
    return viewframe;
}


void SubView::init()
{
    m_instance = new SubView();
    UIContext::registerNotification(m_instance);
    UIAction::registerAction("Hide Sidebar");
    UIAction::registerAction("Go back");
    UIAction::registerAction("Go forward");
    auto actions = UIActionHandler::globalActions();
    actions->bindAction("Hide Sidebar", UIAction(SubView::hideSidebar));
    // actions->bindAction("Go back", UIAction(SubView::back));
    // actions->bindAction("Go forward", UIAction(SubView::forward));
    UIAction::registerAction("VarRefresh");
    UIAction::registerAction("StackRefresh");
    actions->bindAction("VarRefresh", UIAction(SubView::refresh_var));
    actions->bindAction("StackRefresh", UIAction(SubView::refresh_stack));
}

void SubView::refresh_var(const UIActionContext &context){
    auto ctx = context.context;
    QMainWindow* main = ctx->mainWindow();
    auto container = main->findChild<SplitPaneWidget*>();
    for(auto w : container->findChildren<VariableList*>()){
        w->refresh();
    }
}

void SubView::refresh_stack(const UIActionContext &context){
    auto ctx = context.context;
    QMainWindow* main = ctx->mainWindow();
    auto container = main->findChild<SplitPaneWidget*>();
    for(auto w : container->findChildren<StackView*>()){
        w->refresh();
    }
}

void SubView::back(const UIActionContext& context){
    auto ctx = context.context;
    auto vf = ctx->getCurrentViewFrame();
    vf->back();
}

void SubView::forward(const UIActionContext& context){
    auto ctx = context.context;
    auto vf = ctx->getCurrentViewFrame();
    vf->forward();
}

void SubView::addView(UIContext *context, QString viewtype){
    QMainWindow* main = context->mainWindow();
    ViewFrame* frame = context->getCurrentViewFrame();
    if(frame == nullptr){
        qDebug() << "frame is null";
        return;
    }
    // qDebug() << "frame " << frame << main->findChild<ViewFrame*>();
    BinaryViewRef bv = frame->getCurrentBinaryView();
    auto lv = main->findChild<LinearView*>();
    auto container = (SplitPaneContainer*)get_parent_from_type(frame, "SplitPaneContainer");
    if(container == nullptr){
        qDebug() << "parent is null";
    }
    auto sidebar = context->sidebar();
    auto types = sidebar->types();
    // for(auto c : collection->containers()){
    //     qDebug() << c;
    // }
    for(auto type: types){
        if(type->name().compare(viewtype) == 0){
        // Check if type is protected against multiple instances
        if(protect_duplication.find(type->name()) != protect_duplication.end()){
           qDebug() << "TODO Implement the duplication prevention";
        }
        QSplitter* view = new QSplitter();
        view->setOrientation(Qt::Orientation::Vertical);
        auto newview = type->createWidget(frame, bv);
        auto header = newview->headerWidget();
        // This needs to be called in this order,
        // trying to call setVisible afterwards makes everything segfault
        if(header != nullptr){
            header->setVisible(true);
            header->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
            view->addWidget(header);
        }
        view->addWidget(newview);
        auto splitw = new SplitTabWidget(collection);
        // QObject::connect(splitw, &SplitTabWidget::currentChanged, [=](QWidget* widget){
        //     qDebug() << "signal called";
        //         auto parent = (SplitPaneContainer*)get_parent_from_type(splitw, "SplitPaneContainer");
        //         // splitw->close();
            // auto container = splitw->findChild<DockableTabWidget*>();
            // if(container == nullptr){
            //     return;
            // }
            // // This will close everything when trying to move a tab while other tabs exists
            // if(container->currentWidget() == nullptr){
            //     qDebug() << "widget is null !";
            //     auto parent = (SplitPaneContainer*)get_parent_from_type(splitw, "SplitPaneContainer");
            //     // splitw->close();
            //     if(parent != nullptr){
            //         parent->closeCurrentPane();
            //     }
            // }
        // });
        TabPane* newpane = new TabPane(splitw, view, type->name());
        for(auto tab : splitw->findChildren<DockableTabWidget*>()){
            if(tabs.find(tab) == tabs.end()){
                QObject::connect(tab, &DockableTabWidget::tabCloseRequested, [=]() {
                    qDebug() << "close called";
                    qDebug() << tab->currentWidget();
                    if(tab->currentWidget() == nullptr){
                        tab->close();
                    }
                    // if(tab->count() == 0){
                    //     qDebug() << "empty !";
                    // }
                });
                QObject::connect(tab, &DockableTabWidget::tabRemovedForReparent, [=]() {
                    qDebug() << "moved called";
                    qDebug() << tab->currentWidget();
                    if(tab->currentWidget() == nullptr){
                        tab->close();
                    }
                    // if(tab->count() == 0){
                    //     qDebug() << "empty !";
                    // }
                });
                tabs.insert(tab);
            }
        }
        // if(containers.find(container) == containers.end()){
        //    container->installEventFilter(new ContainerFilter(main));
        //    containers.insert(container);
        // }
        container->open(newpane, Qt::Orientation::Horizontal);
        // We don't need to continue
        break;
        }
    }
}

void SubView::openCFGView(UIContext *context, ViewFrame *vf){
    BinaryViewRef bv = context->getCurrentViewFrame()->getCurrentBinaryView();
    QMainWindow* main = context->mainWindow();
    auto linearviews = main->findChildren<LinearView*>();
    LinearView* linearview;
    if(linearviews.size() != 0){
        linearview = linearviews[0];
    }
    auto flowGraph = new DisassemblyContainer((QWidget*)vf->parent(), bv, vf, linearview->getCurrentFunction());
    auto container = (SplitPaneContainer*)get_parent_from_type(vf, "SplitPaneContainer");
    TabPane* newpane = new TabPane(new SplitTabWidget(collection), flowGraph, "FlowGraph");
    container->open(newpane, Qt::Orientation::Horizontal);
}


void SubView::openLinearView(UIContext *context, ViewFrame* vf){
    BinaryViewRef bv = context->getCurrentViewFrame()->getCurrentBinaryView();
    QMainWindow* main = context->mainWindow();
    // TODO Find a way to have at least all the actions from normal linear view
    auto linearview = new LinearView(bv, vf);
    auto container = (SplitPaneContainer*)get_parent_from_type(vf, "SplitPaneContainer");
    TabPane* newpane = new TabPane(new SplitTabWidget(collection), linearview, "Linear View");
    container->open(newpane, Qt::Orientation::Horizontal);
}

void SubView::initView(UIContext *context, ViewFrame* vf){
    BinaryViewRef bv = context->getCurrentViewFrame()->getCurrentBinaryView();
    QMainWindow* main = context->mainWindow();
    auto linearviews = main->findChildren<LinearView*>();
    auto splitw = new SplitTabWidget(collection);
    int counter = 0;
    for(auto linear : linearviews){
        if(linear->getData() != bv){
            continue;
        }
        auto split_container = (SplitPaneContainer*)get_parent_from_type(linear, "SplitPaneContainer");
        auto splitter = (Splitter*)get_parent_from_type(linear, "Splitter");
        auto root_pane = (SplitPaneWidget*)get_parent_from_type(linear, "SplitPaneWidget");
        auto vp = (ViewPane*)get_parent_from_type(linear, "ViewPane");
        linear->installEventFilter(new ClickFilter(main));
        QString viewname = QString("LinearView %1").arg(QString::number(counter++));
        auto widget = new QSplitter();
        widget->setOrientation(Qt::Vertical);
        for(auto child : vp->findChildren<QWidget*>(Qt::FindDirectChildrenOnly)){
            widget->addWidget(child);
        }
        // QObject::connect(splitw, &SplitTabWidget::currentChanged, [=](){
        //     auto container = splitw->findChild<DockableTabWidget*>();
        //     if(container == nullptr){
        //         return;
        //     }
        //     // This will close everything when trying to move a tab while other tabs exists
        //     qDebug() << "new widget : " << container->currentWidget();
        //     if(container->count() == 0){
        //         qDebug() << "widget is null !";
        //         auto parent = (SplitPaneContainer*)get_parent_from_type(splitw, "SplitPaneContainer");
        //         if(parent != nullptr){
        //             parent->closeCurrentPane();
        //         }
        //     }
        // });
        TabPane* newpane = new TabPane(splitw, widget, viewname.toStdString().c_str());
        widget->addWidget(vp);
        vp->updateStatus();
        vp->setParent(widget);
        root_pane->open(newpane, Qt::Horizontal);
        split_container->layout()->removeWidget(vp);
        context->getCurrentActionHandler()->reparentWidget(linear);
        auto vpp = (SplitPaneContainer*)get_parent_from_type(vp, "SplitPaneContainer");
        vpp->hide();
    }
}

void SubView::OnAfterOpenFile(UIContext* context, FileContext* file, ViewFrame* frame){
    auto bv = context->getCurrentViewFrame()->getCurrentBinaryView();
    if(bv == nullptr){
        return;
    }
    auto scope = SettingsResourceScope;
    auto settings = BinaryNinja::Settings::Instance();
    settings->RegisterGroup("subviews", "Subview");
    if(!settings->Contains("subviews.enabled")){
        settings->RegisterSetting("subviews.enabled",R"({
        "title" : "Toggle the usage of subviews module",
        "type" : "number",
        "description" : "Toggle the usage of subview module",
        "ignore" : ["SettingsProjectScope", "SettingsUserScope"],
        "requiresRestart" : true
        })");
    }
    auto val = settings->Get<int64_t>("subviews.enabled", bv);
    if(val == 2){
        // TODO find the root cause for this segfault
        // UIContext::unregisterNotification(m_instance);
        return;
    }
    if(!settings->Contains("subviews.disabled_views")){
        settings->RegisterSetting("subviews.disabled_views", R"|({
        "title" : "Lists the widgets that should be disabled",
        "type" : "array",
        "elementType" : "string",
        "default" : ["Console", "Log", "Mini Graph", "Stack Trace", "Debugger Modules", "Find", "Other (Compatibility)"],
        "description" : "Blacklist widgets from the view submenu to prevent any crash",
        "ignore" : ["SettingsProjectScope", "SettingsUserScope"]
        })|");
    }
    if(!settings->Contains("subviews.refresh_actions")){
        settings->RegisterSetting("subviews.refresh_actions", R"|({
        "title" : "Refresh actions",
        "type" : "array",
        "elementType" : "string",
        "default" : ["VarRefresh", "StackRefresh"],
        "description" :"Add global actions that should be called when the linear view is updated (e.g. when clicking to another place). Useful for custom widgets that needs to be updated at this rate",
        "ignore" : ["SettingsProjectScope", "SettingsUserScope"]
        })|");
    }
    if(!settings->Contains("subviews.prevent_duplicates")){
        settings->RegisterSetting("subviews.prevent_duplicates", R"|({
        "title" : "Prevent duplicates",
        "type" : "array",
        "elementType" : "string",
        "default" : ["IPython"],
        "description" :"Prevent widgets in this list to have duplicates in the main view",
        "ignore" : ["SettingsProjectScope", "SettingsUserScope"]
        })|");
    }
    if(!settings->Contains("subviews.hide_sidebars")){
        settings->RegisterSetting("subviews.hide_sidebars",R"({
        "title" : "Hide sidebar on startup",
        "type" : "number",
        "description" : "Hide the sidebars on startup, better to use the module with it",
        "ignore" : ["SettingsProjectScope", "SettingsUserScope"],
        "requiresRestart" : true
        })");
    }
    auto disabled = settings->Get<std::vector<std::string>>("subviews.disabled_views", bv);
    for(auto d : disabled){
        disabled_views.insert(QString(d.c_str()));
    }
    auto refresh = settings->Get<std::vector<std::string>>("subviews.refresh_actions", bv);
    for(auto r : refresh){
        refresh_actions.insert(QString(r.c_str()));
    }
    auto duplicates = settings->Get<std::vector<std::string>>("subviews.prevent_duplicates", bv);
    for(auto d : duplicates){
        refresh_actions.insert(QString(d.c_str()));
    }
    auto hide = settings->Get<int64_t>("subviews.hide_sidebars", bv);
    if(hide == 2){
        qDebug() << "hiding sidebars...";
        QMainWindow* main = context->mainWindow();
        auto children = main->findChildren<SidebarIconsWidget*>();
        for(auto child : children){
            child->setVisible(m_hide);
        }
        auto children_widgets = main->findChildren<SidebarWidgetAndHeader*>();
        ;
        for(auto child : children_widgets){
            if(area_wigets.find(child->widget()->metaObject()->className())!= area_wigets.end()){
                continue;
            }
            child->widget()->setVisible(m_hide);
            child->header()->setVisible(m_hide);
        }
        m_hide = !m_hide;
        // auto handler = context->getCurrentActionHandler();
        // handler->executeAction("Hide Sidebar");
    }
    QMenuBar* menuBar = context->mainWindow()->menuBar();
    QMainWindow* main = context->mainWindow();
    QMenu* views = findMenu(menuBar->actions(), "View");
    auto actions = context->contentActionHandler();
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
        QString actionName = QString("Open ") + type->name();
        UIAction::registerAction(actionName);
        actions->bindAction(actionName, UIAction([=](){
            addView(context, type->name());
        }));
        QObject::connect(openSubView, &QAction::triggered, new QWidget(), [=](){
            addView(context, type->name());
        });
        subviews->addAction(openSubView);
    }
    QAction *openSubView = new QAction("Linear View");
    QObject::connect(openSubView, &QAction::triggered, new QWidget(), [=](){
        // TODO Create another function for this purpose
        openLinearView(context, context->getCurrentViewFrame());
    });
    QString actionName = QString("Open Linear View");
    UIAction::registerAction(actionName);
    actions->bindAction(actionName, UIAction([=](){
        openLinearView(context, context->getCurrentViewFrame());
    }));
    subviews->addAction(openSubView);
    views->addMenu(subviews);
    QAction *openCFG = new QAction("CFG View");
    QString actionNameCFG = QString("Open CFG View");
    UIAction::registerAction(actionNameCFG);
    actions->bindAction(actionNameCFG, UIAction([=](){
        openCFGView(context, context->getCurrentViewFrame());
    }));
    QObject::connect(openCFG, &QAction::triggered, new QWidget(), [=](){
        openCFGView(context, context->getCurrentViewFrame());
    });
    subviews->addAction(openCFG);
    views->addMenu(subviews);
    initView(context, frame);
}

void SubView::OnContextOpen(UIContext* context)
{
}

void SubView::OnContextClose(UIContext* context)
{
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
        for(auto linear : linearviews){
            // This could happen if the cfg action in still binded
            if(!linear->isVisible()){
                linear->setVisible(true);
            }
        }
    }
    else{
        if(!bvs.empty()){
            QMessageBox::critical(main, "Binary Ninja", "Could not find any instance of linear view, this instance is about to auto-destruct in a few secs");
            // This will probably always crash
            // for(auto pairs : bvs){
            //     auto bv = pairs.first;
            //     bv->SaveAutoSnapshot();
            // }
            auto handler = context->getCurrentActionHandler();
            handler->executeAction("Restart Binary Ninja");
        }
    }
}
