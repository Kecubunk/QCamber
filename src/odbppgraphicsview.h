#ifndef __ODBPPGRAPHICSVIEW_H__
#define __ODBPPGRAPHICSVIEW_H__

#include <QGraphicsView>
#include "feature.h"
#include "profile.h"

#include "odbppgraphicsscene.h"

class ODBPPGraphicsView: public QGraphicsView {
  Q_OBJECT

public:
  typedef enum { None = 0, AreaZoom, MousePan } ZoomMode;

  ODBPPGraphicsView(QWidget* parent = 0);
  ~ODBPPGraphicsView();

  void setZoomMode(ZoomMode mode);
  void loadProfile(QString step);
  void setBackgroundColor(QColor color);
  void setHighlight(bool status);
  void clearHighlight(void);

  void clearScene(void);
  void addItem(Symbol* symbol);
  void removeItem(Symbol* symbol);
  void zoomToProfile(void);
  void scaleView(qreal scaleFactor);
  void scrollView(int dx, int dy);

public slots:
  void zoomToRect(QRectF rect);

protected:
  virtual void wheelEvent(QWheelEvent* event);
  virtual void keyPressEvent(QKeyEvent* event);

private:
  ODBPPGraphicsScene* m_scene;
  ZoomMode m_zoomMode;
  Profile* m_profile;
};

#endif /* __ODBPPGRAPHICSVIEW_H__ */
