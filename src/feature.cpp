#include "feature.h"

#include "context.h"
#include <typeinfo>
#include <QDebug>

Features::Features(QString step, QString path):
  Symbol("features"), m_showRepeat(false)
{
  setHandlesChildEvents(true);

  FeaturesParser parser(ctx.loader->absPath(path.arg(step)));
  m_ds = parser.parse();

  if (m_ds->records().size() == 0) {
    return;
  }

  QString hdr = "steps/%1/stephdr";
  StructuredTextParser stephdr_parser(ctx.loader->absPath(hdr.arg(step)));
  StructuredTextDataStore* hds = stephdr_parser.parse();

  StructuredTextDataStore::BlockIterPair ip = hds->getBlocksByKey(
      "STEP-REPEAT");

  qreal top_active, bottom_active, left_active, right_active;

  try {
#define GET(key) (QString::fromStdString(hds->get(key)))
    m_x_datum = GET("X_DATUM").toDouble();
    m_y_datum = GET("Y_DATUM").toDouble();
    m_x_origin = GET("X_ORIGIN").toDouble();
    m_y_origin = GET("Y_ORIGIN").toDouble();

    top_active = GET("TOP_ACTIVE").toDouble();
    bottom_active = GET("BOTTOM_ACTIVE").toDouble();
    left_active = GET("LEFT_ACTIVE").toDouble();
    right_active = GET("RIGHT_ACTIVE").toDouble();
#undef GET

    m_activeRect.setX(m_activeRect.x() + left_active);
    m_activeRect.setY(m_activeRect.y() + top_active);
    m_activeRect.setWidth(m_activeRect.width() - right_active);
    m_activeRect.setHeight(m_activeRect.height() - bottom_active);
  } catch(StructuredTextDataStore::InvalidKeyException) {
    m_x_datum = m_y_datum = m_x_origin = m_y_origin = 0;
  }

  if (ip.first == ip.second) {
    m_activeRect = QRectF();
  }

  for (StructuredTextDataStore::BlockIter it = ip.first; it != ip.second; ++it)
  {
#define GET(key) (QString::fromStdString(it->second->get(key)))
    QString name = GET("NAME").toLower();
    qreal x = GET("X").toDouble();
    qreal y = GET("Y").toDouble();
    qreal dx = GET("DX").toDouble();
    qreal dy = GET("DY").toDouble();
    int nx = GET("NX").toInt();
    int ny = GET("NY").toInt();
    qreal angle = GET("ANGLE").toDouble();
    bool mirror = (GET("MIRROR") == "YES");

    for (int i = 0; i < nx; ++i) {
      for (int j = 0; j < ny; ++j) {
        Features* step = new Features(name, path);
        step->m_virtualParent = this;
        step->setPos(QPointF(x + dx * i, -(y + dy * j)));

        QTransform trans;
        if (mirror) {
          trans.scale(-1, 1);
        }
        trans.rotate(angle);
        trans.translate(-step->x_datum(), step->y_datum());
        step->setTransform(trans);
        m_repeats.append(step);
      }
    }

#undef GET
  }

  delete hds;
}

Features::~Features()
{
  delete m_ds;
}

QRectF Features::boundingRect() const
{
  return QRectF();
}

void Features::addToScene(QGraphicsScene* scene)
{
  for (QList<Record*>::const_iterator it = m_ds->records().begin();
      it != m_ds->records().end(); ++it) {
    (*it)->addToScene(scene);
  }

  for (QList<Features*>::iterator it = m_repeats.begin();
      it != m_repeats.end(); ++it) {
    (*it)->addToScene(scene);
    (*it)->setVisible(m_showRepeat);
  }
}

void Features::setTransform(const QTransform& matrix, bool combine)
{
  for (QList<Record*>::const_iterator it = m_ds->records().begin();
      it != m_ds->records().end(); ++it) {
    Symbol* symbol = (*it)->symbol;
    QTransform trans;
    QPointF o = transform().inverted().map(symbol->pos());
    trans.translate(-o.x(), -o.y());
    trans = matrix * trans;
    trans.translate(o.x(), o.y());
    symbol->setTransform(symbol->transform() * trans, false);
  }

  for (QList<Features*>::iterator it = m_repeats.begin();
      it != m_repeats.end(); ++it) {
    QTransform trans;
    QPointF o = transform().inverted().map(pos());
    trans.translate(-o.x(), -o.y());
    trans = matrix * trans;
    trans.translate(o.x(), o.y());
    (*it)->setTransform(trans, combine);
  }

  QGraphicsItem::setTransform(matrix, true);
}

void Features::setPos(QPointF pos)
{
  setPos(pos.x(), pos.y());
}

void Features::setPos(qreal x, qreal y)
{
  QTransform trans = QTransform::fromTranslate(x, y);
  for (QList<Record*>::const_iterator it = m_ds->records().begin();
      it != m_ds->records().end(); ++it) {
    Symbol* symbol = (*it)->symbol;
    symbol->setTransform(symbol->transform() * trans, false);
  }

  for (QList<Features*>::iterator it = m_repeats.begin();
      it != m_repeats.end(); ++it) {
    (*it)->setPos(x, y);
  }

  QGraphicsItem::setTransform(trans);
  QGraphicsItem::setPos(x, y);
}

void Features::setVisible(bool status)
{
  for (QList<Record*>::const_iterator it = m_ds->records().begin();
      it != m_ds->records().end(); ++it) {
    (*it)->symbol->setVisible(status);
  }

  for (QList<Features*>::iterator it = m_repeats.begin();
      it != m_repeats.end(); ++it) {
    (*it)->setVisible(status);
  }
}

void Features::setShowStepRepeat(bool status)
{
  m_showRepeat = status;

  for (QList<Features*>::iterator it = m_repeats.begin();
      it != m_repeats.end(); ++it) {
    (*it)->setVisible(status);
  }
}

QTextEdit* Features::symbolCount()
{
    QTextEdit *output = new QTextEdit;
    FeaturesDataStore::IDMapType nameMap;
    int total;
    total = 0;
    nameMap = m_ds->symbolNameMap();

    total += createSection(output,"Line",nameMap);
    total += createSection(output,"Pad",nameMap);
    total += createSection(output,"Arc",nameMap);
    total += createSection(output,"Surface",nameMap);
    total += createSection(output,"Text",nameMap);

    output->append("\n--------------------------------------");
    output->append(QString().sprintf("Total \t %20s \t %d"," ",total));
    output->setReadOnly(true);
    return output;
}

int Features::createSection(QTextEdit *output,
            QString sectionTitle, FeaturesDataStore::IDMapType nameMap)
{
    FeaturesDataStore::CountMapType posCountMap,negCountMap;
    QString text;
    int local_total;
    local_total = 0;
    output->append(sectionTitle+
                   " List\n--------------------------------------");

    if(sectionTitle == "Line"){
        posCountMap = m_ds->posLineCountMap();
        negCountMap = m_ds->negLineCountMap();
    }else if(sectionTitle == "Pad"){
        posCountMap = m_ds->posPadCountMap();
        negCountMap = m_ds->negPadCountMap();
    }else if(sectionTitle == "Arc"){
        posCountMap = m_ds->posArcCountMap();
        negCountMap = m_ds->negArcCountMap();
    }else if(sectionTitle == "Surface"){
        text.sprintf("POS \t %20s \t %d"," ",
                     (local_total+=m_ds->posSurfaceCount()));
        output->append(text);
        text.sprintf("NEG \t %20s \t %d"," ",
                     (local_total+=m_ds->negSurfaceCount()));
        output->append(text);
        output->append(" ");
        return local_total;
    }else if(sectionTitle == "Text"){
        text.sprintf("POS \t %20s \t %d"," ",
                     (local_total+=m_ds->posTextCount()));
        output->append(text);
        text.sprintf("NEG \t %20s \t %d"," ",
                     (local_total+=m_ds->negTextCount()));
        output->append(text);
        output->append(" ");
        return local_total;
    }


    for(int i = 0;i < posCountMap.size();i++){
      if(posCountMap[i] != 0){
        text.sprintf("POS \t %20s \t %d",
                     nameMap[i].toAscii().data(),posCountMap[i]);
        output->append(text);
        local_total+=posCountMap[i];
      }
    }
    for(int i = 0;i < negCountMap.size();i++){
      if(negCountMap[i] != 0){
        text.sprintf("NEG \t %20s \t %d",
                     nameMap[i].toAscii().data(),negCountMap[i]);
        output->append(text);
        local_total+=negCountMap[i];
      }
    }
    output->append(text.sprintf("Total \t %20s \t %d","",local_total));
    output->append(" ");
    return local_total;
}
