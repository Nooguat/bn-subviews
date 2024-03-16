#include "ui/uicontext.h"
#include "ui/viewframe.h"
#include "ui/render.h"
#include "ui/linearview.h"
#include <QLabel>
#include <QPoint>
#include <QMouseEvent>
#include <QMainWindow>
class PocWindow : public QWidget {
Q_OBJECT
public:
    QLabel* real_view;
    QPoint base_pos;
    PocWindow(QString value) : QWidget() {real_view = new QLabel(value);};
    // virtual void mousePressEvent(QMouseEvent *event) override;
    // virtual void mouseMoveEvent(QMouseEvent *event) override;
public slots:
    void testWindow();
};
