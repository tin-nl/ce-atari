#ifndef __TRANSLATEDBOOTMEDIA_H_
#define __TRANSLATEDBOOTMEDIA_H_

#include <stdio.h>
#include "../datatypes.h"
#include "imedia.h"

#define TRANSLATEDBOOTMEDIA_SIZE	(32 * 1024)

class TranslatedBootMedia: public IMedia
{
public:
    TranslatedBootMedia();
    ~TranslatedBootMedia();

    virtual bool iopen(char *path, bool createIfNotExists);
    virtual void iclose(void);

    virtual bool isInit(void);
    virtual bool mediaChanged(void);
    virtual void setMediaChanged(bool changed);
    virtual void getCapacity(DWORD &bytes, DWORD &sectors);

    virtual bool readSectors(DWORD sectorNo, DWORD count, BYTE *bfr);
    virtual bool writeSectors(DWORD sectorNo, DWORD count, BYTE *bfr);

	void updateBootsectorConfigWithACSIid(BYTE acsiId);
	
private:

    DWORD	BCapacity;			// device capacity in bytes
    DWORD	SCapacity;			// device capacity in sectors

	bool	gotImage;
    BYTE	*imageBuffer;
	
	bool	loadDataIntoBuffer(void);
	void 	updateBootsectorConfig(void);
	int		getConfigPosition(void);
	DWORD	getDword(BYTE *bfr);
	void	setDword(BYTE *bfr, DWORD val);
	
	void	updateBootsectorChecksum(void);
	WORD	swapNibbles(WORD val);
};

#endif // __TRANSLATEDBOOTMEDIA_H_
