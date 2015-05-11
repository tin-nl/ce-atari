#include <mint/sysbind.h>
#include <mint/osbind.h>
#include <mint/ostruct.h>
#include <support.h>

#include <stdio.h>
#include "stdlib.h"
#include "out.h"

extern int drive;

void test040x(void);
void test041x(void);
void test043x(void);

BYTE filenameExists(char *filename);

void deleteRecursive(char *subPath);
void deleteIfExists(char *fname);

WORD testDcreate(char *fname);

void testSingleValidChar(WORD testCaseNo, char *fname);
void testSingleInvalidChar(WORD testCaseNo, char *fname, WORD goodErrorCode);

void test04(void)
{
    out_s("Dcreate, Ddelete, Frename, Fdelete");
    
    Dsetdrv(drive);
    (void) Dcreate("\\TEST04");
    Dsetpath("\\TEST04");
    
    test040x();
    test041x();
    test043x();

 
    Ddelete("\\TEST04");
    out_s("");
}

void test040x(void)
{
    BYTE ok;
    WORD res;

    res = testDcreate("TESTDIR");
    (res == 0) ? (ok = 1) : (ok = 0);
    out_tr_bw(0x0401, "Dcreate - just filename", ok, res);

    res = testDcreate("TEST.D");
    (res == 0) ? (ok = 1) : (ok = 0);
    out_tr_bw(0x0402, "Dcreate - filename & extension", ok, res);
    
    res = testDcreate("TESTTEST.DIR");
    (res == 0) ? (ok = 1) : (ok = 0);
    out_tr_bw(0x0403, "Dcreate - full filename & full extension", ok, res);
    
    res = testDcreate(".DIR");
    (res == 0) ? (ok = 1) : (ok = 0);
    out_tr_bw(0x0404, "Dcreate - no filename, just extension", ok, res);

    (void) Dcreate("TESTDIR");
    res = Dcreate("TESTDIR");
    (res == 0xffdc) ? (ok = 1) : (ok = 0);
    out_tr_bw(0x0405, "Dcreate - already existing dir", ok, res);
}

void test041x(void)
{
    testSingleValidChar(0x0410, "A");
    testSingleValidChar(0x0411, "a");
    testSingleValidChar(0x0412, "0");
    testSingleValidChar(0x0413, "!");
    testSingleValidChar(0x0414, "#");
    testSingleValidChar(0x0415, "$");
    testSingleValidChar(0x0416, "%");
    testSingleValidChar(0x0417, "&");
    testSingleValidChar(0x0418, "'");
    testSingleValidChar(0x0419, "(");
    testSingleValidChar(0x0420, ")");
    testSingleValidChar(0x0421, "-");
    testSingleValidChar(0x0422, "@");
    testSingleValidChar(0x0423, "^");
    testSingleValidChar(0x0424, "_");
    testSingleValidChar(0x0425, "{");
    testSingleValidChar(0x0426, "}");
    testSingleValidChar(0x0427, "~");
}

void test043x(void)
{
    testSingleInvalidChar(0x0301, "\"", 0xffdc);
    testSingleInvalidChar(0x0302, "*", 0xffdc);
    testSingleInvalidChar(0x0303, "+", 0xffdc);
    testSingleInvalidChar(0x0304, ",", 0xffdc);
    testSingleInvalidChar(0x0305, "/", 0xffdc);
    testSingleInvalidChar(0x0306, ":", 0xffdc);
    testSingleInvalidChar(0x0307, ";", 0xffdc);
    testSingleInvalidChar(0x0308, "<", 0xffdc);
    testSingleInvalidChar(0x0309, "=", 0xffdc);
    testSingleInvalidChar(0x0310, ">", 0xffdc);
    testSingleInvalidChar(0x0311, "?", 0xffdc);
    testSingleInvalidChar(0x0312, "\\", 0xffdc);
    testSingleInvalidChar(0x0313, "[", 0xffdc);
    testSingleInvalidChar(0x0314, "]", 0xffdc);
    testSingleInvalidChar(0x0315, "|", 0xffdc);
}

void testSingleValidChar(WORD testCaseNo, char *fname)
{
    WORD res, ok;
    
    res = testDcreate(fname);
    (res == 0) ? (ok = 1) : (ok = 0);

    char tmp[64] = {"Dcreate - single valid char: 'A'"};
    tmp[30] = fname[0];
    out_tr_bw(testCaseNo, tmp, ok, res);
}

void testSingleInvalidChar(WORD testCaseNo, char *fname, WORD goodErrorCode)
{
    WORD res, ok;
    res = Dcreate(fname);
    
    (res == goodErrorCode) ? (ok = 1) : (ok = 0);
    
    char tmp[64] = {"Dcreate - single invalid char: 'A'"};
    tmp[32] = fname[0];
    out_tr_bw(testCaseNo, tmp, ok, res);
}

WORD testDcreate(char *fname)
{
    deleteIfExists(fname);          // if the item exists, delete it
    if(filenameExists(fname)) {     // if delete failed, report it as a failure
        return 0xff01;
    }
    
    int res = Dcreate(fname);       // try to create dir
    
    if(res != 0) {                  // if failed to create, return its error code
        return (WORD) res;
    }

    if(!filenameExists(fname)) {    // check if the dir really exists, and if not...
        return 0xff02;
    }
    
    deleteIfExists(fname);          // delete this test dir

    if(filenameExists(fname)) {     // if delete failed, report it as a failure
        return 0xff03;
    }

    return 0;
}

void deleteIfExists(char *fname)
{
    if(filenameExists(fname)) {
        Ddelete(fname);
    }
}

BYTE filenameExists(char *filename)
{
    char dta[44];
    memset(dta, 0, 44);
    
    Fsetdta(dta);
    int res = Fsfirst(filename, 0x3f);

    if(res) {               // doesn't exist
        return 0;
    }
    
    while(1) {
        if(strcmpi(filename, &dta[30]) == 0) {       // filename matches?
            return 1;
        }
    
        res = Fsnext();
        if(res) {           // next doesn't exist?
            break;
        }
    }
    
    return 0;
}