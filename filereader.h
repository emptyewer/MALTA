#ifndef FILEREADER_H
#define FILEREADER_H
#include "includes.h"
#include "structs.h"

class FileReader {
public:
  FileReader();
  prior read_from_file(QString filename);
};

#endif // FILEREADER_H
