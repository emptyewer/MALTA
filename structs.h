#ifndef STRUCTS_H
#define STRUCTS_H
#include <QMap>

struct prior {
  //  QList<QString> genes;
  //  QList<double> counts;
  //  QList<QString> filtered_genes;
  //  QList<double> filtered_counts;
  //  QList<QList<double>> simulated_counts;
  //  QList<double> p;
  //  QList<double> np;
  //  QList<double> cum_np;
  //
  QMap<QString, double> _counts;
  QMap<QString, double> _filtered_counts;
  QMap<QString, double> _bound_counts;
  QMap<QString, QList<double>> _simulated_counts;
  unsigned int fcount;
};

struct greater_comp {
  template <class T> bool operator()(T const &a, T const &b) const {
    return a > b;
  }
};

#endif // STRUCTS_H
