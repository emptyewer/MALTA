#ifndef MCWORKER_H
#define MCWORKER_H
#include <QMap>
#include <QObject>

class MCWorker : public QObject {
  Q_OBJECT
public:
  explicit MCWorker(QMap<QString, double>, QList<QString>, double c,
                    unsigned long t, int);

public slots:
  void stopWork();
  void linearSearch();

signals:
  void linear_search_finished(int, QMap<QString, unsigned long>);

private:
  QMap<QString, double> map;
  QList<QString> genes;
  double count;
  unsigned long trials;
  bool counting;
  int thread;
};

#endif // MCWORKER_H
