#ifndef PROCCGROUP_H
#define PROCCGROUP_H

#define NAMESIZE 1024

typedef struct CtrlFileHeader {
  char name[NAMESIZE];
  size_t fileSize;
} CtrlFileHeader;

typedef struct ProcCGroup {
  char name[NAMESIZE];
  size_t numCtrlFiles;
} ProcCGroup;

#endif // ifndef PROCCGROUP_H
