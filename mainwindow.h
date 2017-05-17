#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "structs.h"
#include <QMainWindow>
#include <QSortFilterProxyModel>
#include <QStandardItemModel>
#include <QTableWidget>
#include <QThread>

using namespace std;

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow {
  Q_OBJECT
  QThread thread;

public:
  explicit MainWindow(QWidget *parent = 0);
  ~MainWindow();

private slots:
  void on_action_import_prior_triggered();
  void on_confidence_valueChanged(double arg1);
  void showPointToolTip(QMouseEvent *event);
  void on_cutoff_editingFinished();

  void on_simulate_clicked();
  void partial_simulation_finished(int, QString);
  void simulation_finished(int);

  void on_lower_slider_sliderReleased();

  void on_upper_slider_sliderReleased();

  void on_upper_slider_valueChanged(int value);

  void on_lower_slider_valueChanged(int value);

private:
  Ui::MainWindow *ui;
  prior prior_distribution;
  QMap<unsigned long, double> rounds;
  unsigned long trials;
  double target;
  QMap<double, double> sortMap;
  QStandardItemModel my_model;
  QSortFilterProxyModel proxy_model;
  int repeats;
  int threads;
  void find_range();
  void narrow_range(unsigned long start, unsigned long end);
  long double cumulative_p_at_trial(unsigned long);
  void update_counts_table();
  void plot_counts();
  void calculate_occurance();
  void update_table_colors(QTableWidget *table);
  void filter_genes();
  void list_filtered_genes();
  QList<QString> get_sorted_genes();
  void bound_genes();
  void setup_graph();
  void toggle_able(bool b);
  void launch_mc_threaad(QList<QString>);
  void control_threads();
  QStringList headers;
  long double cumulative_product_at_trial(unsigned long trial);
};

#endif // MAINWINDOW_H
