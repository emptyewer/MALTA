#include "filereader.h"
#include <QFile>
#include <QTableWidgetItem>

FileReader::FileReader() {}

prior FileReader::read_from_file(QString filename) {
  QFile file(filename);
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    qDebug() << file.errorString();
  }
  QTextStream in(&file);
  prior p;

  while (!file.atEnd()) {
    QString line = in.readLine();
    QStringList split = line.split(',');
    if (split.length() > 1) {
      qDebug() << split;
      double c = split.at(1).toDouble();
      if (c > 0.0) {
        p.genes.append(split.at(0));
        p.counts.append(c);
      }
    }
  }
  return p;
}
