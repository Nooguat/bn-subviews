#include "UI/DockableSidebar.h"
#include "binaryninja-api/ui/uicontext.h"
#include "ui/action.h"
#include "ui/filecontext.h"
#include "ui/tabwidget.h"
#include "ui/uicontext.h"
#include "ui/stackview.h"
#include "ui/linearview.h"
#include "ui/uitypes.h"
#include "ui/viewframe.h"
#include <qcoreevent.h>
#include <qmainwindow.h>
#include <qaction.h>
#include <qobject.h>
#include <qtmetamacros.h>
#include "ui/disassemblyview.h"


#ifndef SUBVIEW_H
#define SUBVIEW_H

class SubView : public UIContextNotification {
public:
    static SubView* m_instance;
    std::vector<QObject*> sidebar_widgets = {};
    virtual void OnContextOpen(UIContext* context) override;
    virtual void OnContextClose(UIContext* context) override;
    virtual void OnAfterOpenFile(UIContext* context, FileContext* file, ViewFrame* frame) override;
    virtual void OnViewChange(UIContext *context, ViewFrame *frame, const QString &type) override;
    static void init();
    static void openView(const UIActionContext& action);
    static void addView(UIContext *context, QString viewtype);
    static void hideSidebar(const UIActionContext& action);
    static void openLinearView(UIContext* context, ViewFrame* vf);
    static void openCFGView(UIContext* context, ViewFrame* vf);
    static void reloadModule(const UIActionContext& action);
    static void NavigateBack(const UIActionContext& context);
    // Needed those for LV before, could be useful someday
    static void back(const UIActionContext& context);
    static void forward(const UIActionContext& context);
    static void refresh_var(const UIActionContext& context);
    static void refresh_stack(const UIActionContext& context);

private:
    static bool m_hide;
    static int m_cfg_count;
};


class ViewFilter : public QObject {
public:
    ViewFilter(QMainWindow* main);
    ViewFilter(){};
    bool eventFilter(QObject* watched, QEvent* event) override;
private:
    QMainWindow* m_main;
};

class ContainerFilter : public QObject {
public:
    ContainerFilter(QMainWindow* main);
    ContainerFilter(){};
    bool eventFilter(QObject* watched, QEvent* event) override;
private:
    QMainWindow* m_main;
};

class ClickFilter : public QObject {
public:
    ClickFilter(QMainWindow* main);
    bool eventFilter(QObject *watched, QEvent *event) override;
private:
    QMainWindow* m_main;
};

class SubViewBarStyle : public DockableTabStyle {
public:
    SubViewBarStyle();
    virtual void paintTab(const QWidget* widget, QStylePainter& p, const DockableTabInfo& info, int idx, int count,
        int active, DockableTabInteractionState state, const QRect& rect) const override;
};

class TabPane : public Pane {
Q_OBJECT
    QString m_title;
    SplitTabWidget* m_container;
    ViewPane* m_vp;
public:
    TabPane(SplitTabWidget* container, QWidget* widget, QString title, PaneHeader* header=nullptr);
    virtual QString title() override {return m_title;};
    virtual void updateStatus() override;
    virtual bool event(QEvent* event) override;
    SplitTabWidget* getContainer() {return m_container;};
    ViewFrame* viewFrame() const;

Q_SIGNALS:
    void updateWidgetStatus();
};
#endif // SUBVIEW_H