#ifndef SCSI_H
#define SCSI_H

#include <string>

#include "../acsidatatrans.h"
#include "../settings.h"
#include "imedia.h"
#include "nomedia.h"
#include "testmedia.h"
#include "translatedbootmedia.h"
#include "sdmedia.h"

#include "../datatypes.h"
#include "../isettingsuser.h"

#define SCSI_ACCESSTYPE_FULL            0
#define SCSI_ACCESSTYPE_READ_ONLY       1
#define SCSI_ACCESSTYPE_NO_DATA         2

#define SOURCETYPE_NONE                 0
#define SOURCETYPE_IMAGE                1
#define SOURCETYPE_IMAGE_TRANSLATEDBOOT 2
#define SOURCETYPE_DEVICE               3
#define SOURCETYPE_SD_CARD				4
#define SOURCETYPE_TESTMEDIA            100

#define MAX_ATTACHED_MEDIA              9

#define TRANSLATEDBOOTMEDIA_FAKEPATH	"TRANSLATED BOOT MEDIA"

typedef struct {
    std::string hostPath;                       // specifies host path to image file or device
    int         hostSourceType;                 // type: image or device
    int         accessType;                     // access type: read only, read write, no data

    IMedia      *dataMedia;                     // pointer to the data provider object
    bool        dataMediaDynamicallyAllocated;  // set to true if dataMedia was created by new and delete should be used, otherwise set to false and delete won't be used 

    int         devInfoIndex;                   // index in devInfo[]
} TScsiConf;

typedef struct {
    int     attachedMediaIndex; // index in attachedMedia[]

    BYTE 	accessType;         // SCSI_ACCESSTYPE_FULL || SCSI_ACCESSTYPE_READ_ONLY || SCSI_ACCESSTYPE_NO_DATA

    BYTE	LastStatus;			// last returned SCSI status
    BYTE	SCSI_ASC;			// additional sense code
    BYTE	SCSI_ASCQ;			// additional sense code qualifier
    BYTE	SCSI_SK;			// sense key
} TDevInfo;


class Scsi: public ISettingsUser
{
public:
    Scsi(void);
    virtual ~Scsi();

    void reloadSettings(int type);

    void setAcsiDataTrans(AcsiDataTrans *dt);

    bool attachToHostPath(std::string hostPath, int hostSourceType, int accessType);
    void dettachFromHostPath(std::string hostPath);
	void detachAll(void);
    void detachAllUsbMedia(void);
	
    void processCommand(BYTE *command);

	void setSdCardCapacity(DWORD capInSectors);
	
    void updateTranslatedBootMedia(void);
    
private:
    AcsiDataTrans   *dataTrans;

    BYTE            acsiId;                 // current acsi ID for the command
    IMedia          *dataMedia;             // current data media valid for current ACSI ID

    NoMedia         	noMedia;
    TestMedia       	testMedia;
	TranslatedBootMedia	tranBootMedia;
	SdMedia				sdMedia;

    BYTE            *dataBuffer;
    BYTE            *dataBuffer2;

    BYTE            shitHasHappened;

    TScsiConf       attachedMedia[MAX_ATTACHED_MEDIA];
    TDevInfo        devInfo[8];

	AcsiIDinfo		acsiIdInfo;
	
    BYTE *cmd;
    BYTE inquiryName[10];

    bool isICDcommand(void);

	// for 6-byte long commands - from scsi6
    void ProcScsi6(void);

	void SCSI_RequestSense(void);
	void SCSI_FormatUnit(void);

    void SCSI_ReadWrite6(bool read);

	void SCSI_Inquiry(void);
	void SCSI_ModeSense6(void);

	void SendOKstatus(void);
	void ReturnStatusAccordingToIsInit(void);
	void ReturnUnitAttention(void);
	void ClearTheUnitAttention(void);
    void returnInvalidCommand(void);

	void SendEmptySecotrs(WORD sectors);

	// for commands longer than 6 bytes - from scsiICD
	void ProcICD(void); 

	void SCSI_ReadCapacity(void);
	void ICD7_to_SCSI6(void);
    void SCSI_ReadWrite10(bool read);
	void SCSI_Verify(void);
	
	void showCommand(WORD id, WORD length, WORD errCode);

    bool readSectors(DWORD sectorNo, DWORD count);
    bool writeSectors(DWORD sectorNo, DWORD count);
    bool compareSectors(DWORD sectorNo, DWORD count);
    bool eraseMedia(void);


    void loadSettings(void);

    int  findEmptyAttachSlot(void);
    int  findAttachedMediaByHostPath(std::string hostPath);
    void dettachBySourceType(int hostSourceType);
    void dettachByIndex(int index);
    bool attachMediaToACSIid(int mediaIndex, int hostSourceType, int accessType);
    void detachMediaFromACSIidByIndex(int index);

    void initializeAttachedMediaVars(int index);
};

#endif
