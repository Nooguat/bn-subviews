#ifndef KSUITE_FRAMABLESIDEBAR_H
#define KSUITE_FRAMABLESIDEBAR_H

#include <QtWidgets>
#include "binaryninja-api/ui/uicontext.h"
#include "binaryninja-api/ui/sidebar.h"


class FramableSidebar : public QWidget {
public:
    FramableSidebar(UIContext* context);
};

#endif