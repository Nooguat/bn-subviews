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
#include <cstdint>
#include <fmt/core.h>
#include <fmt/ranges.h>
#include <ios>
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
#include <stack>
#include <ui/viewframe.h>
#include <vector>
#include "DockableSidebar.h"
#include "BWindow.h"
#include "binaryninja-api/ui/linearview.h"
#include "ui/sidebaricons.h"
#include "ui/sidebarwidget.h"
#include "ui/splitter.h"
#include "ui/stackview.h"
#include "ui/tabwidget.h"
#include "ui/typeview.h"
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
// Hardcoded, cannot use GlobalArea as in the v3
std::set<QString> area_wigets = {"ScriptingConsole", "LogView"};
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

ClickFilter::ClickFilter(QMainWindow* main) : QObject(){
    m_main = main;
}

ContainerFilter::ContainerFilter(QMainWindow* main) : QObject(){
    m_main = main;
}

ViewFilter::ViewFilter(QMainWindow* main) : QObject(){
    m_main = main;
}

bool ViewFilter::eventFilter(QObject* watched, QEvent*event){
    qDebug() << watched->metaObject()->className() << event;
    return true;
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
        auto main = get_parent_from_type(watched, "MainWindow");
        if(main == nullptr){
            event->ignore();
            return false;
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

// TODO Fix the segfault at the end of program exec, we need to clear up the filter
bool ContainerFilter::eventFilter(QObject* watched, QEvent* event){
    if(event->type() == QEvent::Leave || event->type() == QEvent::ChildAdded || event->type() == QEvent::ChildPolished){
        qDebug() << event->type();
        auto main = get_parent_from_type(watched, "MainWindow");
        if(main == nullptr){
            event->ignore();
            return false;
        }
        for(auto child : main->findChildren<SplitPaneContainer*>()){
            qDebug() << child << child->children();
            if(child->findChild<SplitPaneContainer*>() != nullptr || child->findChild<LinearView*>() != nullptr){
                qDebug() << "continue";
                continue;
            }
            if(child->findChild<ViewPane*>() != nullptr ){
                child->setVisible(false);
            }
            else if(child->findChild<TabPane*>() != nullptr){
               auto pane = child->findChild<TabPane*>();
               auto widget = child->findChild<SidebarWidget*>();
               if(widget == nullptr){
                    qDebug() << "closing empty tabpane" << pane->children();
                    pane->closePane();
                    child->close();
               }
            }
            else{
                qDebug() << child << "should close now" << child->children();
                child->closeCurrentPane();
            }
        qDebug() << "------------------------";
        }
    }
    event->ignore();
    return true;
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

TabPane::TabPane(SplitTabWidget* container, QWidget* widget, QString name, PaneHeader* default_header) : Pane(container){
    if(default_header == nullptr){
        auto header = new WidgetPaneHeader("");
        // header->installEventFilter(new ContainerFilter());
        Pane::init(header);
    }
    else{
        // default_header->installEventFilter(new ContainerFilter());
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
    qDebug() << "frame " << frame << main->findChild<ViewFrame*>();
    BinaryViewRef bv = frame->getCurrentBinaryView();
    auto lv = main->findChild<LinearView*>();
    auto container = (SplitPaneContainer*)get_parent_from_type(frame, "SplitPaneContainer");
    if(container == nullptr){
        qDebug() << "parent is null";
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
        // trying to call setVisible afterwards makes everything segfault
        if(header != nullptr){
            header->setVisible(true);
            header->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
            view->addWidget(header);
        }
        view->addWidget(newview);
        auto splitw = new SplitTabWidget(collection);
        TabPane* newpane = new TabPane(splitw, view, type->name());
        context->getCurrentActionHandler()->updateActionBindings("refresh");
        QObject::connect(splitw, &SplitTabWidget::currentChanged, [=](){
            auto container = splitw->findChild<DockableTabWidget*>();
            if(container == nullptr){
                return;
            }
            // This will close everything when trying to move a tab while other tabs exists
            // qDebug() << "new widget : " << container->currentWidget();
            if(container->count() == 0){
                qDebug() << "widget is null !";
                auto parent = (SplitPaneContainer*)get_parent_from_type(splitw, "SplitPaneContainer");
                // splitw->close();
                if(parent != nullptr){
                    parent->closeCurrentPane();
                }
            }
        });
        container->open(newpane, Qt::Orientation::Horizontal);
        auto sidebars = main->findChildren<Sidebar*>();
        for(auto sb : sidebars){
            for(auto child : sb->findChildren<SidebarWidget*>()){
                if(child->metaObject()->className() == viewtype){
                    child->closing();
                }
            }
        }
        // newpane->parent()->installEventFilter(new ContainerFilter(main));
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
        QObject::connect(splitw, &SplitTabWidget::currentChanged, [=](){
            auto container = splitw->findChild<DockableTabWidget*>();
            if(container == nullptr){
                return;
            }
            // This will close everything when trying to move a tab while other tabs exists
            qDebug() << "new widget : " << container->currentWidget();
            if(container->count() == 0){
                qDebug() << "widget is null !";
                auto parent = (SplitPaneContainer*)get_parent_from_type(splitw, "SplitPaneContainer");
                if(parent != nullptr){
                    parent->closeCurrentPane();
                }
                // splitw->close();
            }
        });
        qDebug() << widget->children();
        TabPane* newpane = new TabPane(splitw, widget, viewname.toStdString().c_str());
        // vp->installEventFilter(new ViewFilter(main));
        widget->addWidget(vp);
        vp->updateStatus();
        vp->setParent(widget);
        root_pane->open(newpane, Qt::Horizontal);
        split_container->layout()->removeWidget(vp);
        // for(auto child : vp->children()){
        //     delete child;
        // }
        context->getCurrentActionHandler()->reparentWidget(linear);
        // Clear out the viewpane
        auto vpp = (SplitPaneContainer*)vp->parent();
        vpp->hide();
        // vpp->closeCurrentPane();
        // newpane->parent()->installEventFilter(new ContainerFilter(main));
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
        // This crashes, find why
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
        "title" : "Lists the actions to call upon linear view updates",
        "type" : "array",
        "elementType" : "string",
        "default" : ["VarRefresh", "StackRefresh"],
        "description" :"Add global actions that should be called when the linear view is updated (e.g. when clicking to another place). Useful for custom widgets that needs to be updated at this rate",
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
    auto hide = settings->Get<int64_t>("subviews.hide_sidebars", bv);
    qDebug() << "hide value" << hide;
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
    // Add open cfg view
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
    openLinearView(context, frame);
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
