//
// Created by vr1s on 5/8/23.
//
#include "binaryninja-api/ui/uicontext.h"
#include "ui/uicontext.h"
#include "ui/stackview.h"
#include "ui/linearview.h"
#include <qmainwindow.h>
#include <qaction.h>
#include <qtmetamacros.h>

#ifndef KSUITE_NOTIFICATIONS_H
#define KSUITE_NOTIFICATIONS_H

class ContextSidebarManager;


class Notifications : public UIContextNotification {
    static Notifications* m_instance;

    std::unordered_map<UIContext*, ContextSidebarManager*> m_ctxForSidebar;

    std::vector<size_t> m_sessionsAlreadyDisplayedPickerFor;

public:
    virtual void OnContextOpen(UIContext* context) override;
    virtual void OnContextClose(UIContext* context) override;
    virtual void OnViewChange(UIContext *context, ViewFrame *frame, const QString &type);
    LinearView* m_stackview;
    static void init();
public slots:
    void GetChildren(UIContext* context);
};



#endif //KSUITE_NOTIFICATIONS_H
