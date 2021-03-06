#ifndef TESTMEDIA_H
#define TESTMEDIA_H

#include <stdio.h>
#include "../datatypes.h"
#include "imedia.h"

class TestMedia: public IMedia
{
public:
    TestMedia();
    virtual ~TestMedia();

    virtual bool iopen(char *path, bool createIfNotExists);
    virtual void iclose(void);

    virtual bool isInit(void);
    virtual bool mediaChanged(void);
    virtual void setMediaChanged(bool changed);
    virtual void getCapacity(int64_t &bytes, int64_t &sectors);

    virtual bool readSectors(int64_t sectorNo, DWORD count, BYTE *bfr);
    virtual bool writeSectors(int64_t sectorNo, DWORD count, BYTE *bfr);

private:

};

#endif // TestMedia_H
