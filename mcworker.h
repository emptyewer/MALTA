#ifndef MCWORKER_H
#define MCWORKER_H
#include <QMap>
#include <QObject>

class MCWorker : public QObject {
  Q_OBJECT
public:
  explicit MCWorker(QMap<QString, double>, QList<QString>, unsigned long c,
                    unsigned long t);

public slots:
  void doWork();
  void stopWork();

signals:
  void update_value(int, QString);
  void finished();

private:
  template class QMap<QString, double> map;
  QList<QString> genes;
  unsigned long count;
  unsigned long trials;
  bool counting;
};

#endif // MCWORKER_H
