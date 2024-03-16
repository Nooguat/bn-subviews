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
#include <qmainwindow.h>
#include <qaction.h>
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
    void addView(UIContext *context, QString viewtype);
    static void hideSidebar(const UIActionContext& action);
    static void openLinearView(UIContext* context, ViewFrame* vf);
    static void openCFGView(UIContext* context, ViewFrame* vf);
    static void reloadModule(const UIActionContext& action);
private:
    static bool m_hide;
};


class LinearPane : public LinearView {
public:
    LinearPane(BinaryViewRef data, ViewFrame* view);
private:
    DisassemblyContainer* m_graph;
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
public:
    TabPane(SplitTabWidget* container, QWidget* widget, QString title, PaneHeader* header=nullptr);
    virtual QString title() override {return m_title;};
    virtual void updateStatus() override;
Q_SIGNALS:
    void updateWidgetStatus();
};
#endif // SUBVIEW_H