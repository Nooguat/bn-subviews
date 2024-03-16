#include "FramableSidebar.h"
#include "binaryninjacore.h"
#include "ui/theme.h"
#include "ui/sidebar.h"
// Includes below needed to remove the include of this
//#include "SharedCache/dscwidget.h"
#include "shared.h"
#include <QtCore/QAbstractItemModel>
#include <QtCore/QSortFilterProxyModel>
#include <QtWidgets/QTreeView>

#include "binaryninja-api/ui/filter.h"
#include "binaryninja-api/ui/sidebar.h"
#include "binaryninja-api/ui/uitypes.h"

#include "priv.h"
#include "binaryninja-api/ui/xreflist.h"
#include "binaryninja-api/ui/typeview.h"
#include "binaryninja-api/ui/variablelist.h"
#include "binaryninja-api/ui/stackview.h"

#include "binaryninja-api/ui/stringsview.h"
#include "binaryninja-api/ui/taglist.h"
#include "binaryninja-api/ui/minigraph.h"
#include "binaryninja-api/ui/memorymap.h"
#include "binaryninja-api/ui/viewframe.h"
#include "binaryninja-api/ui/progresstask.h"

#include <QtCore/QMimeData>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QVBoxLayout>
#include <filesystem>
#include <QtWidgets>

#include <QPainter>
#include <QStyleOptionButton>
#include <QDebug>
#include <QStylePainter>


FramableSidebar::FramableSidebar(UIContext* context){
  return;
}