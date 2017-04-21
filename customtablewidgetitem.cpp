#include "customtablewidgetitem.h"

CustomTableWidgetItem::CustomTableWidgetItem(double a, double s) {
  average = a;
  std = s;
}

bool CustomTableWidgetItem::
operator<(const CustomTableWidgetItem &other) const {
  return average < other.average;
}

bool CustomTableWidgetItem::
operator>(const CustomTableWidgetItem &other) const {
  return average > other.average;
}

QVariant CustomTableWidgetItem::data(int role) const {
  QString str;
  return str.sprintf("%.2f ± %.2f", average, std);
}
