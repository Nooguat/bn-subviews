// Created by vr1s on 5/8/23.
//

#include "Notifications.h"
#include <QLayout>
#include <QApplication>
#include <QList>
#include <QObject>
#include <QMainWindow>
#include <QMenu>
#include "ui/linearview.h"
#include "ui/pane.h"
#include "ui/render.h"
#include "ui/sidebar.h"
#include <qalgorithms.h>
#include <qboxlayout.h>
#include <qbrush.h>
#include <qdialog.h>
#include <qglobal.h>
#include <qgridlayout.h>
#include <qlist.h>
#include <qmainwindow.h>
#include <qnamespace.h>
#include <qsplitter.h>
#include <stack>
#include <ui/viewframe.h>
#include "DockableSidebar.h"
#include "BWindow.h"
//#include "SharedCache/dscpicker.h"
#include "binaryninja-api/ui/linearview.h"
#include "ui/stackview.h"
#include "ui/typeview.h"
#include "ui/uicontext.h"
#include "ui/uitypes.h"
#include <regex>
//#include "Actions/MultiShortcut.h"
// #include "Actions/Actions.h"
//#include "UI/Theme/ThemeEditor.h"

Notifications* Notifications::m_instance = nullptr;

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

void Notifications::init()
{
    m_instance = new Notifications;
    UIContext::registerNotification(m_instance);
}

QMenu* findMenu(QList<QAction*> actions, QString name){
    foreach (QAction *action, actions){
        if(action->menu()){
            if(QString::compare(name, action->text()) == 0){
                return action->menu();
                printf("menu : %s\n", action->text().toStdString().c_str());
            }
            findMenu(action->menu()->actions(), name);
        }
    }
    return nullptr;
}

// QWidget* Notifications::findWidget(){
//     for (auto &widget : QApplication::allWidgets()) {
//         if (widget && widget->metaObject() && std::string(widget->metaObject()->className()) == "Sidebar") {
//             auto bar = static_cast<Sidebar*>(widget);
//             //auto viewframe = new ViewFrame(context->mainWindow()->centralWidget(), nullptr, "", false);
//             //viewframe->setVisible(true);
//             //bar->setContainer(nullptr);
//             auto layout = findParentLayout(widget);
//             auto junk = new QWidget(context->mainWindow()->centralWidget());
//             junk->setVisible(false);
//             junk->setFixedSize(0, 0);
//             bar->container()->setParent(junk);
//             bar->container()->setVisible(false);
//             break;
//         }
//     }
//     return nullptr;
// }

void PocWindow::testWindow(){
    return ;
}

QObject* get_parent_from_type(QObject* base, std::string className){
    QObject* parent = base->parent();
    while(parent != nullptr){
        if(className.compare(parent->metaObject()->className()) == 0){
            return parent;
        }
        parent = parent->parent();
    }
    return nullptr;
}

void Notifications::GetChildren(UIContext *context){
    QMainWindow* main = context->mainWindow();
    ViewFrame* frame = context->getCurrentViewFrame();
    BinaryViewRef bv = context->getAvailableBinaryViews()[0].first;
    if(frame == nullptr){
        qDebug() << "frame is null";
        return;
    }
    SplitPaneContainer* container = static_cast<SplitPaneContainer*>(frame->parent()->parent());
    if(container == nullptr){
        qDebug() << "container is null";
        return;
    }
    StackViewSidebarWidget* stackview = new StackViewSidebarWidget(frame, bv);
    WidgetPane* newpane = new WidgetPane(stackview, "a title");
    stackview->refresh();
    container->open(newpane, Qt::Orientation::Horizontal);

}

void Notifications::OnContextOpen(UIContext* context)
{

	QMenuBar* menuBar = context->mainWindow()->menuBar();
    QMenu* views = findMenu(menuBar->actions(), "View");
    if(views == nullptr){
        return;
    }
    QAction *pocTypes = new QAction("Types");
    QObject::connect(pocTypes, &QAction::triggered, new QWidget(), [=](){
        GetChildren(context);
    });
    views->addAction(pocTypes);
    // for (auto &widget : QApplication::allWidgets()) {
    //     if (widget && widget->metaObject() && std::regex_match(std::string(widget->metaObject()->className()) , std::regex(".*Sidebar.*"))) {qDebug() << widget->metaObject()->className();
    //         delete widget;
    //         widget = NULL;
    //     }
    // }
    /*
    context->mainWindow()->setStyleSheet(context->mainWindow()->styleSheet() + "LinearView QMenu, SidebarWidget QMenu "
                                                                               "{ "
                                                                               "    padding: 0px; "
                                                                               "    border-radius: 7px; "
                                                                               "    color:transparent; "
                                                                               "    background-color: transparent;"
                                                                               "}"
                                                                               "LinearView QMenu::item, SidebarWidget QMenu::item "
                                                                               "{"
                                                                               "    padding: 0px;"
                                                                               "}");
*/

}

void Notifications::OnContextClose(UIContext* context)
{
}

void Notifications::OnViewChange(UIContext *context, ViewFrame *frame, const QString &type)
{

}

