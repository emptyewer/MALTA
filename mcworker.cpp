#include "mcworker.h"
#include "includes.h"
#include <chrono>
#include <random>

static std::mt19937
    rng(std::chrono::high_resolution_clock::now().time_since_epoch().count());

MCWorker::MCWorker(QMap<QString, double> a, QList<QString> g, double c,
                   unsigned long t, int th) {
  map = a;
  genes = g;
  count = c;
  trials = t;
  thread = th;
  counting = true;
}

void MCWorker::linearSearch() {
  QMap<QString, unsigned long> gene_picks;
  QMap<double, QString> gene_prob;
  double cum_count = 0;
  for (int i = 0; i < genes.length(); i++) {
    gene_picks[genes[i]] = 0;
    double p = map[genes[i]] / count;
    cum_count = cum_count + p;
    gene_prob[cum_count] = genes[i];
  }

  for (unsigned long j = 0; j < trials; j++) {
    double random_number =
        static_cast<double>(rng()) / (static_cast<double>(rng.max()));
    QString picked_gene = gene_prob.upperBound(random_number).value();
    gene_picks[picked_gene] = gene_picks[picked_gene] + 1;
  }
  emit linear_search_finished(thread, gene_picks);
}

void MCWorker::stopWork() { counting = false; }
