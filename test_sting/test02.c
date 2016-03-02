//--------------------------------------------------
#include <mint/osbind.h> 
#include <stdio.h>

#include "version.h"
#include "global.h"
#include "transprt.h"
#include "stdlib.h"
#include "out.h"

//--------------------------------------------------
extern TPL *tpl;
extern BYTE *rBuf, *wBuf;

void doTest0200(BYTE tcpNotUdp);
void doTest0202(BYTE tcpNotUdp, BYTE test0202not0204);

int trySend0202   (int handle, int tcpNotUdp, int size, int timeoutSecs);
int tryReceive0202(int handle, int size, int timeoutSecs);
int tryReceive0204(int handle, int size, int timeoutSecs);

void doTest0206    (BYTE tcpNotUdp);
int  tryReceive0206(int handle);

void doTest0208    (BYTE tcpNotUdp);

void doTest0210    (WORD testNumber);
int  sendAndReceive(BYTE tcpNotUdp, DWORD blockSize, int handle);

void doTest02(void)
{
    // CNbyte_count + CNget_char
    doTest0200(1);      // TCP
    doTest0200(0);      // UDP

    // CNget_NDB
    doTest0202(1, 1);   // TCP
    doTest0202(0, 1);   // UDP
    
    // CNget_block
    doTest0202(1, 0);   // TCP
    doTest0202(0, 0);   // UDP
    
    // CNgets
    doTest0206(1);      // TCP
    doTest0206(0);      // UDP
    
    // CNgetinfo
    doTest0208(1);      // TCP
    doTest0208(0);      // UDP
    
    // TCP_open() and UDP_open - addressing modes tests
    int i;
    for(i = 0x0210; i <= 0x0222; i++) {
        doTest0210(i);
    }
}

void doTest0200(BYTE tcpNotUdp)
{
    int handle, res;
    
    if(tcpNotUdp) {
        out_test_header(0x0200, "TCP CNbyte_count + CNget_char");
        handle = TCP_open(SERVER_ADDR, SERVER_PORT_START, 0, 2000);
    } else {
        out_test_header(0x0201, "UDP CNbyte_count + CNget_char");
        handle = UDP_open(SERVER_ADDR, SERVER_PORT_START + 4);
    }
    
    if(handle < 0) {
        out_result_error_string(0, handle, "open failed");
        return;
    }
    
    if(tcpNotUdp) {
        res = TCP_send(handle, wBuf, 1000);
    } else {
        res = UDP_send(handle, wBuf, 1000);
    }
    
    if(res != E_NORMAL) {
        out_result_error_string(0, res, "send failed");
        goto test0200end;
    }
    
    DWORD now, start;
    start = getTicks();
    
    int toReceive = 1000;
    int idx = 0;
    int mismatch = 0;
    
    while(1) {
        // nothing more to receive? quit
        if(toReceive <= 0) {

            if(!mismatch) {         // no mismatch?   good
                out_result(1);
            } else {                // data mismatch? fail
                out_result_string(0, "received enough data, but mismatch");
            }
             
            goto test0200end;
        }
    
        //----------
        // something to receive?
        int gotBytes = CNbyte_count(handle);    // how much data we have?
        if(gotBytes > 0) {
            int i, a;
            
            for(i=0; i<gotBytes; i++) {         // try to receive it all
                a = CNget_char(handle);
                
                if(a != wBuf[idx]) {            // check if the data is matching the sent data
                    mismatch = 1;
                }
                idx++;
            }
            
            toReceive -= gotBytes;              // now we need to receive less
        }    
        
        //----------
        // check for timeout
        now = getTicks();
        
        if((now - start) > 600) {
            out_result_string(0, "timeout while receiving");
            goto test0200end;
        }
    }
    
test0200end:
    if(tcpNotUdp) {
        TCP_close(handle, 0, 0);
    } else {
        UDP_close(handle);
    }
}

void doTest0202(BYTE tcpNotUdp, BYTE test0202not0204)
{
    int handle, res;
    
    // open connection
    if(tcpNotUdp) {
        if(test0202not0204) {
            out_test_header(0x0202, "TCP CNget_NDB");
        } else {
            out_test_header(0x0204, "TCP CNget_block");
        }
        
        handle = TCP_open(SERVER_ADDR, SERVER_PORT_START, 0, 2000);
    } else {
        if(test0202not0204) {
            out_test_header(0x0203, "UDP CNget_NDB");
        } else {
            out_test_header(0x0205, "UDP CNget_block");
        }
        
        handle = UDP_open(SERVER_ADDR, SERVER_PORT_START + 4);
    }
    
    if(handle < 0) {
        out_result_error_string(0, handle, "open failed");
        return;
    }
    
    int j;
    for(j=0; j<10; j++) {
        // send data
        res = trySend0202(handle, tcpNotUdp, 1000, 3);

        if(!res) {
            out_result_string(0, "timeout on send");
            goto test0202end;
        }
    
        // get data
        if(test0202not0204) {
            res = tryReceive0202(handle, 1000, 3);
        } else {
            res = tryReceive0204(handle, 1000, 3);
        }

        if(res) {       // if not 0 (not good)
            goto test0202end;
        }
    }
    
    out_result(1);                 // everything OK
    
test0202end:
    // close connection
    if(tcpNotUdp) {
        TCP_close(handle, 0, 0);
    } else {
        UDP_close(handle);
    }
}

int trySend0202(int handle, int tcpNotUdp, int size, int timeoutSecs)
{
    int   res;
    int   timeoutTics = timeoutSecs * 200;
    DWORD timeout     = getTicks() + timeoutTics;
    
    while(1) {
        if(tcpNotUdp) {
            res = TCP_send(handle, wBuf, size);
        } else {
            res = UDP_send(handle, wBuf, size);
        }

        if(res == E_NORMAL) {                       // if good, success
            return 1;
        }
        
        if(getTicks() >= timeout) {                 // if timeout, fail
            return 0;
        }
    }
}

int tryReceive0202(int handle, int size, int timeoutSecs)
{
    int   timeoutTics = timeoutSecs * 200;
    DWORD timeout     = getTicks() + timeoutTics;
    
    NDB *ndb;
    
    while(1) {
        ndb = CNget_NDB(handle);

        if(getTicks() >= timeout) {         // if timeout, fail
            out_result_string(0, "timeout on CNget_NDB");
            return -1;
        }
        
        if(ndb) {                           // some received? process it
            break;
        }
    }
        
    int res = 0;                            // good (for now)
   
    if(ndb->len != size) {                  // block size mismatch, fail
        res = -2;
        out_result_string(0, "CNget_NDB size mismatch");
    }
    
    if(res == 0) {                          // only if size OK
        int i;
        for(i=0; i<size; i++) {
            if(ndb->ndata[i] != wBuf[i]) {  // received data mismatch? fail
                res = -3;
                out_result_string(0, "CNget_NDB data mismatch");
                break;
            }
        }
    }
    
    KRfree (ndb->ptr);                      // free the ram
    KRfree (ndb);

    return res;                             // return error code
}

int tryReceive0204(int handle, int size, int timeoutSecs)
{
    int   timeoutTics = timeoutSecs * 200;
    DWORD timeout     = getTicks() + timeoutTics;
    
    int res;
    
    while(1) {
        res = CNget_block(handle, rBuf, size);
    
        if(getTicks() >= timeout) {         // if timeout, fail
            out_result_string(0, "timeout on CNget_block");
            return -1;
        }

        if(res != E_NODATA) {
            break;
        }
    }
        
    if(res != size) { 
        out_result_error_string(0, res, "CNget_block failed");
        return res;
    }        

    res = memcmp(wBuf, rBuf, size);
    
    if(res != 0) {
        out_result_string(0, "data mismatch");
        return -2;
    }
    
    return 0;           // good
}

void doTest0206(BYTE tcpNotUdp)
{
    int handle, res;
    
    // open connection
    if(tcpNotUdp) {
        out_test_header(0x0206, "TCP CNgets");
        handle = TCP_open(SERVER_ADDR, SERVER_PORT_START + 2, 0, 2000);
    } else {
        out_test_header(0x0207, "UDP CNgets");
        handle = UDP_open(SERVER_ADDR, SERVER_PORT_START + 2 + 4);
    }
    
    if(handle < 0) {
        out_result_error_string(0, handle, "open failed");
        return;
    }
    
    #define GETS_LINES  32
    
    char txBuf[2];
    txBuf[0] = 0;
    txBuf[1] = GETS_LINES;
        
    if(tcpNotUdp) {
        res = TCP_send(handle, txBuf, 2);
    } else {
        res = UDP_send(handle, txBuf, 2);
    }

    if(res != E_NORMAL) {
        out_result_string(0, "timeout on send");
        goto test0206end;
    }
    
    int j;
    for(j=0; j<GETS_LINES; j++) {
        res = tryReceive0206(handle);

        if(res) {       // if not 0 (not good)
            goto test0206end;
        }
    }
    
    out_result(1);                      // everything OK
    
test0206end:
    // close connection
    if(tcpNotUdp) {
        TCP_close(handle, 0, 0);
    } else {
        UDP_close(handle);
    }
}

int tryReceive0206(int handle)
{
    int   timeoutTics = 3 * 200;
    DWORD timeout     = getTicks() + timeoutTics;

    memset(rBuf, 0, 200);                   // clear the buffer
    
    int res;
    while(1) {
        res = CNgets(handle, rBuf, 200, '\n');
    
        if(res == E_NORMAL) {               // if good, quit
            break;
        }

        if(getTicks() >= timeout) {         // if timeout, fail
            out_result_string(0, "timeout on CNgets");
            return res;
        }
    }

    int lenFromString = getIntFromStr((char *) rBuf, 4);
    int lenFromStrlen = strlen       ((char *) rBuf);
    
    if((lenFromString + 4) != lenFromStrlen) {
        out_result_string(0, "too short received string");
        return -2;
    }
    
    return 0;           // good
}

void doTest0208(BYTE tcpNotUdp)
{
   int handle, res;
   int remotePort;
    
    // open connection
    if(tcpNotUdp) {
        out_test_header(0x0208, "TCP CNgetinfo");
        remotePort = SERVER_PORT_START + 3;
        handle = TCP_open(SERVER_ADDR, remotePort, 0, 2000);
    } else {
        out_test_header(0x0209, "UDP CNgetinfo");
        remotePort = SERVER_PORT_START + 3 + 4;
        handle = UDP_open(SERVER_ADDR, remotePort);
    }
    
    if(handle < 0) {
        out_result_error_string(0, handle, "open failed");
        return;
    }
    
    CIB *cib = CNgetinfo(handle);
    
    if(!cib) {
        out_result_string(0, "CNgetinfo failed");
        goto test0208end;
    }
    
    //---------------------------
    // verify protocol
    if(tcpNotUdp) {
        if(cib->protocol != P_TCP) {
            out_result_string(0, "protocol not TCP");
            goto test0208end;
        }
    } else {
        if(cib->protocol != P_UDP) {
            out_result_string(0, "protocol not UDP");
            goto test0208end;
        }
    }
    
    //---------------------------
    // verify remote port
    if(cib->address.rport != remotePort) {
        out_result_string(0, "remote port bad");
        goto test0208end;
    }
    
    // verify remote address
    if(cib->address.rhost != SERVER_ADDR) {
        out_result_string(0, "remote address bad");
        goto test0208end;
    }

    // verify status
    if(cib->status != 0) {
        out_result_error_string(0, cib->status, "status not 0");
        goto test0208end;
    }

    //---------------------------
    // send closing time
    #define CLOSING_TIME_MS     1000
    
    char txBuf[2];
    txBuf[0] = (BYTE) (CLOSING_TIME_MS >> 8);       // upper byte
    txBuf[1] = (BYTE) (CLOSING_TIME_MS     );       // lower byte
        
    if(tcpNotUdp) {
        res = TCP_send(handle, txBuf, 2);
    } else {
        res = UDP_send(handle, txBuf, 2);
    }
    
    if(res != E_NORMAL) {
        out_result_error_string(0, res, "send failed");
        goto test0208end;
    }
    
    //---------------------------
    // if it's TCP, wait for closing and check status
    DWORD start = getTicks();
    if(tcpNotUdp) {
        // wait for connection to close
        res = TCP_wait_state(handle, TCLOSED, 3);
        
        if(res != E_NORMAL) {                       // if didn't get to TCLOSED state, fail
            out_result_error_string(0, res, "waiting for close failed");
            goto test0208end;
        }
    }
    DWORD end   = getTicks();
    DWORD ms    = (end - start) * 5;                // calculate how long it took to find out, that the socket has closed on the other side
    
    out_result_error(1, ms);                        // everything OK
    
test0208end:
    // close connection
    if(tcpNotUdp) {
        TCP_close(handle, 0, 0);
    } else {
        UDP_close(handle);
    }    
}

void doTest0210(WORD testNumber)
{
    int handle, res, ok;
    
    CAB cab;
    cab.lhost = 0;                      // local  host
    cab.lport = 10000;                  // local  port
    cab.rhost = SERVER_ADDR;            // remove host
    cab.rport = SERVER_PORT_START;      // remove port
    
    int tcpNotUdp           = 1;        // by default - TCP
    int activeNotPassive    = 1;        // by default - active connection
    
    switch(testNumber) {
        /////////////////
        // remote host is normal IP
        case 0x0210:                // TCP, rem_host = IP addr, rem_port = port number (not TCP_ACTIVE, not TCP_PASSIVE) --> ACTIVE
            out_test_header(testNumber, "TCP_open - IP specified, port specified");
            handle = TCP_open(SERVER_ADDR, SERVER_PORT_START, 0, 2000);
            break;
            
        case 0x0211:                // TCP, rem_host = 0,       rem_port = port number (not TCP_ACTIVE, not TCP_PASSIVE) --> PASSIVE, rem_port is local port number
            activeNotPassive = 0;   // passive connection
            out_test_header(testNumber, "TCP_open - IP specified, port 0");
            handle = TCP_open(SERVER_ADDR, 0, 0, 2000);
            break;

        /////////////////
        // remote host is CAB *, TCP_ACTIVE
        case 0x0212:                // TCP, rem_host = CAB *,   rem_port = TCP_ACTIVE, rport & rhost specify remote address, cab->lport is local port, cab->lhost may be 0 (in that case it's filled with local ip)
            out_test_header(testNumber, "TCP_open - TCP_ACTIVE, got remote & local");
            handle = TCP_open(&cab,        TCP_ACTIVE, 0, 2000);
            break;

        case 0x0213:                // TCP, rem_host = CAB *,   rem_port = TCP_ACTIVE, rport & rhost specify remote address, cab->lport is 0, next free port will be used (use CNgetinfo() to find out used port #), cab->lhost may be 0 (in that case it's filled with local ip)
            out_test_header(testNumber, "TCP_open - TCP_ACTIVE, got only remote");
            cab.lport = 0;
            handle = TCP_open(&cab,        TCP_ACTIVE, 0, 2000);
            break;
        
        case 0x0214:                // TCP, rem_host = 0,       rem_port = TCP_ACTIVE --> either will crash, or returns E_PARAMETER
            out_test_header(testNumber, "TCP_open - TCP_ACTIVE, rem_host is NULL");
            handle = TCP_open(NULL,        TCP_ACTIVE, 0, 2000);
            
            ok = (handle == E_PARAMETER) ? 1 : 0;
            out_result(ok);
            return;
            
            break;

        /////////////////
        // remote host is CAB *, TCP_PASSIVE
        case 0x0215:                // TCP, rem_host = CAB *,   rem_port = TCP_PASSIVE, cab->lport is local port
            activeNotPassive = 0;   // passive connection
            out_test_header(testNumber, "TCP_open - TCP_PASSIVE, got local port");
            handle = TCP_open(&cab,        TCP_PASSIVE, 0, 2000);
            break;

        case 0x0216:                // TCP, rem_host = CAB *,   rem_port = TCP_PASSIVE, cab->lport is 0, next free port will be used (use CNgetinfo() to find out used port #)
            activeNotPassive = 0;   // passive connection
            out_test_header(testNumber, "TCP_open - TCP_PASSIVE, local port is 0");
            cab.lport = 0;
            handle = TCP_open(&cab,        TCP_PASSIVE, 0, 2000);
            break;
        
        case 0x0217:                // TCP, rem_host = 0,       rem_port = TCP_PASSIVE --> either will crash, or will open PASSIVE connection on next free port
            activeNotPassive = 0;   // passive connection
            out_test_header(testNumber, "TCP_open - TCP_PASSIVE, rem_host is NULL");
            handle = TCP_open(NULL,        TCP_PASSIVE, 0, 2000);
            break;
            
        ////////////////////////////////////////////////////////
        case 0x0220:                // UDP, rem_host is IP,    rem_port is port # (not 0 / UDP_EXTEND) -> remote is to IP:port, local port is next free port
            tcpNotUdp = 0;          // it's UDP
            out_test_header(testNumber, "UDP_open - IP specified, port specified");
            handle = UDP_open(SERVER_ADDR, SERVER_PORT_START);
            break;

        case 0x0221:                // UDP, rem_host is CAB *, rem_port is 0 / UDP_EXTEND, cab->lport is port # (not 0) -> open UDP on this local port
            tcpNotUdp = 0;          // it's UDP
            out_test_header(testNumber, "UDP_open - UDP_EXTEND, got local port");
            handle = UDP_open(&cab, UDP_EXTEND);
            break;
        
        case 0x0222:                // UDP, rem_host is CAB *, rem_port is 0 / UDP_EXTEND, cab->lport is 0              -> open UDP on first free local port
            tcpNotUdp = 0;          // it's UDP
            out_test_header(testNumber, "UDP_open - UDP_EXTEND, local port is 0");
            cab.lport = 0;
            handle = UDP_open(&cab, UDP_EXTEND);
            break;
            
        ////////////////////////////////////////////////////////
        // bad test case number? quit
        default:    return;
    }
    
    if(handle < 0) {
        out_result_error_string(0, handle, "open failed");
        return;
    }
    
    //---------------------
    // for passive connection, send local port to server, wait for connection
    if(!activeNotPassive) {
        //---------
        // find out local port
        CIB *cib = CNgetinfo(handle);
        
        if(!cib) {
            out_result_string(0, "CNgetinfo() failed");
            goto test0210end;
        }
        
        //---------
        // send local port to server
        BYTE tmp[6];
        
        tmp[0] = (cib->address.lhost >> 24) & 0xff;
        tmp[1] = (cib->address.lhost >> 16) & 0xff;
        tmp[2] = (cib->address.lhost >>  8) & 0xff;
        tmp[3] = (cib->address.lhost      ) & 0xff;

        tmp[4] = (cib->address.lport >>  8) & 0xff;
        tmp[5] = (cib->address.lport      ) & 0xff;
        
        int handle2;
        
        handle2 = TCP_open(SERVER_ADDR, SERVER_PORT_START + 20, 0, 1000);
        
        if(handle2 < 0) {
            out_result_error_string(0, handle2, "tell port TCP_open() failed");
            goto test0210end;
        }
        
        res = TCP_wait_state(handle2, TESTABLISH, 3);
        
        if(res != E_NORMAL) {
            out_result_error_string(0, res, "tell port TCP_wait_state() failed");
            goto test0210end;
        }
        
        res = TCP_send(handle2, tmp, 6);
        
        if(res != E_NORMAL) {
            out_result_error_string(0, res, "tell port TCP_send() failed");
            goto test0210end;
        }
        
        TCP_close(handle2, 0, 0);
        
        //---------
        // wait until server connects back
        res = TCP_wait_state(handle, TESTABLISH, 5);
        
        if(res != E_NORMAL) {
            out_result_error_string(0, res, "server didn't connect back");
            goto test0210end;
        }
    }
    
    //---------------------
    // send & receive data
    res = sendAndReceive(tcpNotUdp, 1000, handle);
    
    if(!res) {                              // if single block-send-and-receive operation failed, quit and close
        goto test0210end;
    }
    
    //---------------------
    // success flows through here
    out_result(1);                          // success!    
    
test0210end:
    if(tcpNotUdp) {
        TCP_close(handle, 0, 0);
    } else {
        UDP_close(handle);
    }
}

