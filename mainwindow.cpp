#include "mainwindow.h"
#include "CSVparser.hpp"
#include "mcworker.h"
#include "qcustomplot.h"
#include "ui_mainwindow.h"
#include <QDir>
#include <QFileDialog>
#include <QLocale>
#include <QStandardPaths>
#include <QThread>
#include <iostream>
#include <limits>
#include <math.h>
#include <stdio.h>
#include <time.h>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow) {
  ui->setupUi(this);
  connect(ui->customPlot, SIGNAL(mousePress(QMouseEvent *)), this,
          SLOT(showPointToolTip(QMouseEvent *)));
  ui->tabWidget->setTabEnabled(1, false);
  setup_graph();
  toggle_able(false);
}

MainWindow::~MainWindow() { delete ui; }

// Action triggered to import a new file
void MainWindow::on_action_import_prior_triggered() {
  // Clear all data
  prior_distribution._counts.clear();

  QString fileName = QFileDialog::getOpenFileName(
      this, tr("Gene Count Files"),
      QStandardPaths::locate(QStandardPaths::HomeLocation, QString(),
                             QStandardPaths::LocateDirectory),
      tr("Gene Count Files (*.csv)"));
  if (QFileInfo(fileName).exists()) {
    csv::Parser file = csv::Parser(fileName.toStdString());
    headers << QString::fromStdString(file.getHeader().at(0))
            << QString::fromStdString(file.getHeader().at(1));
    QMap<QString, int> _temp;
    double max = std::numeric_limits<double>::min();
    for (unsigned int i = 0; i < file.rowCount(); i++) {
      double c = std::atof(file[i][1].c_str());
      if (c > max) {
        max = c;
      }
      QString g = QString::fromStdString(file[i][0]);
      if (c > 0) {
        if (prior_distribution._counts.contains(g)) {
          prior_distribution._counts[g + "_" + QString::number(_temp[g])] = c;
          _temp[g] += 1;
        } else {
          prior_distribution._counts[g] = c;
          _temp[g] = 1;
        }
      }
    }
    ui->cutoff->setMaximum(max);
    ui->cutoff->setMinimum(0.0);
    //    qDebug() << "Lower Slider: " << ui->lower_slider->value();
    //    qDebug() << "Upper Slider: " << ui->upper_slider->value();
    //    qDebug() << "Prior Size: " << prior_distribution._counts.size();
    calculate_occurance();
    toggle_able(true);
  }
}

void MainWindow::toggle_able(bool b) {
  ui->cutoff->setEnabled(b);
  ui->confidence->setEnabled(b);
  ui->reads->setEnabled(b);
  ui->lower_slider->setEnabled(b);
  ui->upper_slider->setEnabled(b);
  ui->simulate->setEnabled(b);
}

void MainWindow::calculate_occurance() {
  filter_genes();
  list_filtered_genes();
  bound_genes();
  find_range();
  plot_counts();
  update_table_colors(ui->prior_table);
}

void MainWindow::setup_graph() {
  ui->customPlot->xAxis->setVisible(false);
  ui->customPlot->yAxis2->setVisible(true);
  ui->customPlot->yAxis->setLabel("Log Counts");
  ui->customPlot->yAxis2->setLabel("Log Counts");
  ui->customPlot->yAxis->setScaleType(QCPAxis::stLogarithmic);
  ui->customPlot->yAxis2->setScaleType(QCPAxis::stLogarithmic);
  QSharedPointer<QCPAxisTickerLog> logTicker(new QCPAxisTickerLog);
  ui->customPlot->yAxis->setTicker(logTicker);
  ui->customPlot->yAxis2->setTicker(logTicker);
  ui->customPlot->yAxis->setNumberFormat("eb");
  ui->customPlot->yAxis->setNumberPrecision(0);
  ui->customPlot->yAxis2->setNumberFormat("eb");
  ui->customPlot->yAxis2->setNumberPrecision(0);
}

void MainWindow::filter_genes() {
  prior_distribution._filtered_counts.clear();
  prior_distribution.fcount = 0;
  QList<QString> genes = prior_distribution._counts.keys();
  QList<double> values = prior_distribution._counts.values();
  for (int j = 0; j < genes.length(); j++) {
    if (values.at(j) > ui->cutoff->value()) {
      prior_distribution._filtered_counts[genes[j]] = values[j];
      // Total count does not change with the range sliders.
      prior_distribution.fcount += values.at(j);
    }
  }
  //  qDebug() << "(filter_genes) Total: " << prior_distribution.fcount;
  //  qDebug() << "(filter_genes) Filtered Genes: "
  //  << prior_distribution._filtered_counts.keys().length();
}

void MainWindow::list_filtered_genes() {
  // Clear Prior Table
  QList<QString> filtered_genes = prior_distribution._filtered_counts.keys();
  ui->prior_table->clear();
  ui->prior_table->setHorizontalHeaderLabels(headers);
  ui->prior_table->setRowCount(filtered_genes.length());
  for (int i = 0; i < filtered_genes.length(); i++) {
    QTableWidgetItem *gene_name = new QTableWidgetItem(filtered_genes.at(i));
    ui->prior_table->setItem(i, 0, gene_name);
    QTableWidgetItem *count = new QTableWidgetItem;
    count->setData(Qt::EditRole,
                   prior_distribution._filtered_counts[filtered_genes.at(i)]);
    ui->prior_table->setItem(i, 1, count);
  }
  ui->prior_table->sortItems(1, Qt::DescendingOrder);
  ui->prior_table->update();
}

void MainWindow::bound_genes() {
  prior_distribution._bound_counts.clear();
  QList<QString> genes = get_sorted_genes();
  int lower_bound = ui->lower_slider->value() * genes.length() / 100;
  int upper_bound = (100 - ui->upper_slider->value()) * genes.length() / 100;
  //  qDebug() << "Lower Bound: " << lower_bound;
  //  qDebug() << "Upper Bound: " << upper_bound;
  for (int i = lower_bound; i < upper_bound; i++) {
    prior_distribution._bound_counts[genes[i]] =
        prior_distribution._filtered_counts[genes[i]];
  }
  //  qDebug() << "(filter_genes) Bound Genes: "
  //  << prior_distribution._bound_counts.keys().length();
}

void MainWindow::find_range() {
  trials = 1;
  target = ui->confidence->value();
  rounds.clear();
  QList<QString> bound_genes = prior_distribution._bound_counts.keys();
  QMap<QString, long double> _bound_c_np;
  _bound_c_np.clear();
  // Initialize cumulative not prob
  for (int i = 0; i < bound_genes.length(); i++) {
    QString fgene = bound_genes.at(i);
    _bound_c_np[fgene] =
        static_cast<long double>(1 - (prior_distribution._bound_counts[fgene] /
                                      prior_distribution.fcount));
  }

  long double p_atleast_one = static_cast<long double>(0.0);
  while (p_atleast_one < static_cast<long double>(0.999999)) {
    trials = trials * 2;
    long double cum_sum = static_cast<long double>(1.0);
    for (int i = 0; i < bound_genes.length(); i++) {
      QString fgene = bound_genes.at(i);
      long double np = _bound_c_np[fgene];
      _bound_c_np[fgene] = np * np;
      cum_sum = cum_sum * (1 - _bound_c_np[fgene]);
    }
    // First way of doing it.
    //    p_atleast_one =
    //        static_cast<long double>(1.0) - static_cast<long double>(cum_sum);
    // Heaps law way of doing it
    //    p_atleast_one =
    //        (prior_distribution._filtered_counts.keys().length() - cum_sum) /
    //        prior_distribution._filtered_counts.keys().length();
    p_atleast_one = cum_sum;
    //    std::cout << cum_sum << p_atleast_one
    //              << prior_distribution._filtered_counts.keys().length();
    rounds[trials] = static_cast<double>(p_atleast_one);
  }
  //  qDebug() << "(find_range) Rounds: " << rounds;
  for (int i = 1; i < rounds.keys().length(); i++) {
    unsigned long prev_key = rounds.keys().at(i - 1);
    unsigned long key = rounds.keys().at(i);
    if (target > rounds[prev_key] && target < rounds[key]) {
      narrow_range(prev_key, key);
    }
  }
  ui->reads_label->setText(QString("%L1 minimum picks").arg(trials));
  ui->reads->setValue(static_cast<int>(trials));
}

void MainWindow::narrow_range(unsigned long start, unsigned long end) {
  unsigned long trial =
      static_cast<unsigned long>(floor((end - start) / 2) + start);
  long double cprod = cumulative_product_at_trial(trial);
  if (fabs(cprod - static_cast<long double>(target)) <
      static_cast<long double>(0.01)) {
    trials = trial;
    ui->set_label->setText("Completeness of Items: " +
                           QString::number(static_cast<double>(cprod), 'f', 3));
    long double value =
        (prior_distribution.fcount - 1 + cumulative_p_at_trial(trial)) /
        prior_distribution.fcount;
    ui->element_label->setText(
        "Completeness of Set: " +
        QString::number(static_cast<double>(value), 'f', 3));
  } else if (cprod > static_cast<long double>(target)) {
    narrow_range(start, trial);
  } else {
    narrow_range(trial, end);
  }
}

long double MainWindow::cumulative_p_at_trial(unsigned long trial) {
  long double cum_sum = static_cast<long double>(0.0);
  QList<QString> genes = prior_distribution._bound_counts.keys();
  for (int i = 0; i < genes.length(); i++) {
    QString fgene = genes.at(i);
    long double p = static_cast<long double>(
        prior_distribution._bound_counts[fgene] / prior_distribution.fcount);
    long double np = static_cast<long double>(1.0) - p;
    cum_sum += pow(np, trial);
  }
  long double value = static_cast<long double>(cum_sum);
  return value;
}

long double MainWindow::cumulative_product_at_trial(unsigned long trial) {
  long double cum_prod = static_cast<long double>(1.0);
  QList<QString> genes = prior_distribution._bound_counts.keys();
  for (int i = 0; i < genes.length(); i++) {
    QString fgene = genes.at(i);
    long double p = static_cast<long double>(
        prior_distribution._bound_counts[fgene] / prior_distribution.fcount);
    long double np = static_cast<long double>(1.0) - p;
    cum_prod = cum_prod * (1 - pow(np, trial));
  }
  return static_cast<long double>(cum_prod);
}

void MainWindow::on_confidence_valueChanged(double arg1) { find_range(); }

void MainWindow::on_cutoff_editingFinished() {
  //  qDebug() << "*** Cutoff Changed ***";
  prior_distribution._filtered_counts.clear();
  prior_distribution._simulated_counts.clear();
  calculate_occurance();
}

QList<QString> MainWindow::get_sorted_genes() {
  ui->prior_table->sortItems(1, Qt::DescendingOrder);
  ui->prior_table->update();
  QAbstractItemModel *model = ui->prior_table->model();
  QList<QString> genes;
  for (int i = 0; i < model->rowCount(); i++) {
    QModelIndex index = model->index(i, 0);
    genes.append(index.data().toString());
  }
  return genes;
}

void MainWindow::plot_counts() {
  ui->customPlot->clearPlottables();
  QVector<double> x, _x;
  QVector<double> y, _y;
  x.clear();
  _x.clear();
  y.clear();
  _y.clear();

  QList<QString> genes = get_sorted_genes();
  int lower_bound = ui->lower_slider->value() * genes.length() / 100;
  int upper_bound = (100 - ui->upper_slider->value()) * genes.length() / 100;

  for (int i = 1; i < genes.length(); i++) {
    if (i >= lower_bound && i <= upper_bound) {
      x.append(i);
      y.append(prior_distribution._filtered_counts[genes.at(i)]);
    } else {
      _x.append(i);
      _y.append(prior_distribution._filtered_counts[genes.at(i)]);
    }
  }
  QCPBars *bars1 = new QCPBars(ui->customPlot->xAxis, ui->customPlot->yAxis);
  bars1->setData(x, y);
  bars1->setPen(Qt::NoPen);
  bars1->setBrush(QColor(200, 50, 100, 100));
  QCPBars *bars2 = new QCPBars(ui->customPlot->xAxis, ui->customPlot->yAxis);
  bars2->setData(_x, _y);
  bars2->setPen(Qt::NoPen);
  bars2->setBrush(QColor(20, 20, 20));
  // give the axes some labels:
  // set axes ranges, so we see all data:
  ui->customPlot->rescaleAxes();
  ui->customPlot->replot();
}

void MainWindow::update_table_colors(QTableWidget *table) {
  QColor red_color(QColor(200, 50, 100, 50));
  QColor gray_color(QColor(20, 20, 20, 50));
  QList<QString> genes = get_sorted_genes();
  int lower_bound = ui->lower_slider->value() * genes.length() / 100;
  int upper_bound = (100 - ui->upper_slider->value()) * genes.length() / 100;
  for (int j = 0; j < genes.length(); j++) {
    if (j >= lower_bound && j <= upper_bound) {
      table->item(j, 0)->setBackgroundColor(red_color);
    } else {
      table->item(j, 0)->setBackgroundColor(gray_color);
    }
  }
}

void MainWindow::showPointToolTip(QMouseEvent *event) {
  //  int x = ui->customPlot->xAxis->pixelToCoord(event->pos().x());
  //  if (x >= 0 && x <= prior_distribution.p.length()) {
  //  }
  //  ui->customPlot->setToolTip(QString("%1 , %2").arg(x).arg(y));
}

void MainWindow::on_simulate_clicked() {
  repeats = 1;
  QList<QString> genes = get_sorted_genes();
  for (int j = 0; j < genes.size(); ++j) {
    prior_distribution._simulated_counts[genes[j]] = {};
  }
  control_threads();
}

void MainWindow::control_threads() {
  threads = 1;
  QList<QString> genes = get_sorted_genes();
  while (true) {
    if (repeats > 10) {
      break;
    }
    if (threads > QThread::idealThreadCount()) {
      break;
    }
    launch_mc_threaad(genes);
    threads += 1;
    repeats += 1;
  }
}

void MainWindow::launch_mc_threaad(QList<QString> genes) {
  QThread *workerThread = new QThread;
  MCWorker *worker;
  worker = new MCWorker(prior_distribution._filtered_counts, genes,
                        prior_distribution.fcount, ui->reads->value(), repeats);

  worker->moveToThread(workerThread);
  connect(workerThread, SIGNAL(started()), worker, SLOT(doWork()));
  connect(worker, SIGNAL(finished(int)), workerThread, SLOT(quit()));
  connect(worker, SIGNAL(finished(int)), worker, SLOT(deleteLater()));
  connect(worker, SIGNAL(finished(int)), this, SLOT(simulation_finished(int)));
  connect(workerThread, SIGNAL(finished()), workerThread, SLOT(deleteLater()));
  connect(worker, SIGNAL(update_value(int, QString)), this,
          SLOT(partial_simulation_finished(int, QString)));
  workerThread->start();
  ui->simulate->setEnabled(false);
}

void MainWindow::partial_simulation_finished(int value, QString gene) {
  prior_distribution._simulated_counts[gene].append(value);
}

void MainWindow::simulation_finished(int thread) {
  ui->simulate->setEnabled(true);
  if (thread == 10) {
    update_counts_table();
    update_table_colors(ui->count_table);
    //    qDebug() << prior_distribution._simulated_counts;
    //    qDebug() << "Simulation Finished";
  } else {
    control_threads();
  }
}

void MainWindow::update_counts_table() {
  ui->tabWidget->setTabEnabled(1, true);
  ui->count_table->clear();
  ui->count_table->setHorizontalHeaderLabels({"Gene", "MC Count"});
  QList<QString> genes = get_sorted_genes();
  ui->count_table->setRowCount(genes.length());
  for (int i = 0; i < genes.length(); i++) {
    QTableWidgetItem *gene_name = new QTableWidgetItem(genes.at(i));
    ui->count_table->setItem(i, 0, gene_name);
    int total = 0;
    for (int j = 0;
         j < prior_distribution._simulated_counts[genes.at(i)].size(); ++j) {
      total += prior_distribution._simulated_counts[genes.at(i)].at(j);
    }
    double average =
        total * 1.0 / prior_distribution._simulated_counts[genes.at(i)].size();
    double std_dev = 0.0;
    for (int j = 0;
         j < prior_distribution._simulated_counts[genes.at(i)].size(); ++j) {
      double sub =
          prior_distribution._simulated_counts[genes.at(i)].at(j) - average;
      std_dev += sub * sub;
    }
    std_dev =
        std_dev / prior_distribution._simulated_counts[genes.at(i)].size();
    std_dev = sqrt(std_dev);
    QTableWidgetItem *counts = new QTableWidgetItem;
    counts->setData(Qt::EditRole, average);
    QString str;
    counts->setText(str.sprintf("%.2f ± %.2f", average, std_dev));
    ui->count_table->setItem(i, 1, counts);
  }
  ui->count_table->update();
}

void MainWindow::on_lower_slider_sliderReleased() { calculate_occurance(); }

void MainWindow::on_upper_slider_sliderReleased() { calculate_occurance(); }

void MainWindow::on_upper_slider_valueChanged(int value) { plot_counts(); }

void MainWindow::on_lower_slider_valueChanged(int value) { plot_counts(); }
