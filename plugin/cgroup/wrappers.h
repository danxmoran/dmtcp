#ifndef Wrappers_H
#define Wrappers_H

#include <string>

#include "cgroupwrapper.h"

namespace dmtcp
{
class CpuSetWrapper : public CGroupWrapper
{
  public:
    CpuSetWrapper(std::string name);
};
class CpuWrapper : public CGroupWrapper
{
  public:
    CpuWrapper(std::string name);
};
class BlkIOWrapper : public CGroupWrapper
{
  public:
    BlkIOWrapper(std::string name);
};
class MemoryWrapper : public CGroupWrapper
{
  public:
    MemoryWrapper(std::string name);
};
class DevicesWrapper : public CGroupWrapper
{
  public:
    DevicesWrapper(std::string name);
};
class NetClsWrapper : public CGroupWrapper
{
  public:
    NetClsWrapper(std::string name);
};
class NetPrioWrapper : public CGroupWrapper
{
  public:
    NetPrioWrapper(std::string name);
};
class HugeTLBWrapper : public CGroupWrapper
{
  public:
    HugeTLBWrapper(std::string name);
};
class PidsWrapper : public CGroupWrapper
{
  public:
    PidsWrapper(std::string name);
};
class NoCtrlFilesWrapper : public CGroupWrapper
{
  public:
    NoCtrlFilesWrapper(std::string subsystem, std::string name);
};
}

#endif // ifndef Wrappers_H
