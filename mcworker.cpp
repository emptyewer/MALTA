#include "mcworker.h"
#include "includes.h"

MCWorker::MCWorker(QMap<QString, double> a, QList<QString> g, unsigned long c,
                   unsigned long t) {
  map = a;
  genes = g;
  count = c;
  trials = t;
  counting = true;
}

void MCWorker::doWork() {
  for (int i = 0; i < genes.length(); i++) {
    double p = map[genes[i]] / count;
    for (int k = 0; k < 10; k++) {
      int count = 0;
      for (unsigned long j = 0; j < trials; j++) {
        if (counting) {
          double r =
              static_cast<double>(rand() / static_cast<double>(RAND_MAX));
          if (r < p) {
            count = count + 1;
          }
        } else {
          emit finished();
        }
      }
      emit update_value(count, genes[i]);
    }
  }
  emit finished();
}

void MCWorker::stopWork() { counting = false; }
