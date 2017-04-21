#ifndef CUSTOMTABLEWIDGETITEM_H
#define CUSTOMTABLEWIDGETITEM_H
#include <QTableWidgetItem>

class CustomTableWidgetItem : public QTableWidgetItem {
public:
  CustomTableWidgetItem(double, double);
  QVariant data(int role) const;
  bool operator<(const CustomTableWidgetItem &other) const;
  bool operator>(const CustomTableWidgetItem &other) const;

protected:
  double average;
  double std;
};

#endif // CUSTOMTABLEWIDGETITEM_H
