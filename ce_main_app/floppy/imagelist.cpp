#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <string>
#include <algorithm>

#include <unistd.h>

#include "imagelist.h"
#include "../utils.h"
#include "../downloader.h"
#include "../debug.h"

ImageList::ImageList(void)
{
    isLoaded = false;
}

bool ImageList::exists(void)
{
    int res = access(IMAGELIST_LOCAL, F_OK);            // check if file exists

    if(res == 0) {
        return true;
    }
    
    // ok, so the file does not exist

    std::string status;
    Downloader::status(status, DWNTYPE_FLOPPYIMG_LIST); // check if it's downloaded at this moment

    if(!status.empty()) {                               // the file is being downloaded, but we don't have it yet
        return false;
    }    

    // start the download
    TDownloadRequest tdr;
    tdr.srcUrl          = IMAGELIST_URL;
    tdr.checksum        = 0;                            // don't check checksum
    tdr.dstDir          = IMAGELIST_LOCAL_DIR;
    tdr.downloadType    = DWNTYPE_FLOPPYIMG_LIST;
    tdr.pStatusByte     = NULL;                     // don't update this status byte
    Downloader::add(tdr);

    return false;
}

bool ImageList::loadList(void)
{
    vectorOfImages.clear();
    isLoaded = false;

    FILE *f = fopen(IMAGELIST_LOCAL, "rt");             // open file

    if(!f) {
        return false;
    }

    char tmp[1024];
    fgets(tmp, 1023, f);                                // skip version line

    ImageListItem li;                                   // store url and checksum in structure

    while(!feof(f)) {
        memset(tmp, 0, 1024);

        char *c = fgets(tmp, 1023, f);                  // read one line
        
        if(c == NULL) {                                 // didn't read anything?
            continue;
        }
        
        char *tok;
        tok = strtok(tmp, ",");                         // init strtok by passing pointer to string

        if(tok == NULL) {
            continue;
        }

        li.url = tok;                                   // store URL

        std::string path, file;
        Utils::splitFilenameFromPath(li.url, path, file);
        li.imageName = file;                            // also store only image name (without URL)

        tok = strtok(NULL, ",");                        // get next token - checksum

        if(tok == NULL) {
            continue;
        }

        int checksum, res;
        res = sscanf(tok, "0x%x", &checksum);           // get the checksum

        if(res != 1) {
            continue;
        }

        li.checksum = checksum;                         // store checksum

        while(1) {                                      // now move beyond checksum by looking for 0 as string terminator
            if(*tok == 0) {     
                break;
            }
            tok++;
        }

        tok++;                                          // tok now points to the start of games string
    
        if(*tok == 0) {                                 // nothing in this image? skip it
            continue;
        }

        li.games = tok;                                                                     // store games
        std::transform(li.games.begin(), li.games.end(), li.games.begin(), ::tolower);      // convert them to lowercase
        li.marked = false;

        vectorOfImages.push_back(li);                   // store it in vector
    }    

    fclose(f);

    isLoaded = true;
    return true;
}

void ImageList::search(char *part)
{
    std::string sPart = part;
    std::transform(sPart.begin(), sPart.end(), sPart.begin(), ::tolower);                   // convert to lowercase

    vectorOfResults.clear();                                        // clear results

    int cnt = vectorOfImages.size();

    // if not loaded, or the list is empty
    if(!isLoaded || cnt < 1) {                                      
        return;
    }

    // if search string is too short
    bool searchTooShort = false;
    if(strlen(part) < 2) {                                          
        searchTooShort = true;
    }

    // go through the list of images, copy the right ones
    for(int i=0; i<cnt; i++) {
        std::string &games = vectorOfImages[i].games;       
        std::string game;
        SearchResult sr;                                            

        if(searchTooShort) {
            sr.game         = games;                                // store stuff in search result structure
            sr.imageIndex   = i;

            vectorOfResults.push_back(sr);                          // store search result in vector
            continue;                                               // skip searching of the games string
        }

        size_t pos = 0;                                             // start searching from start
        while(1) {
            pos = games.find(sPart, pos);                           // search for part in games

            if(pos == std::string::npos) {                          // not found? quit this
                break;
            }

            getSingleGame(games, game, pos);                        // extract single game from list of games

            sr.game         = game;                                 // store stuff in search result structure
            sr.imageIndex   = i;

            vectorOfResults.push_back(sr);                          // store search result in vector

            pos++;                                                  // move to next char
        }
    }
}

void ImageList::getSingleGame(std::string &games, std::string &game, size_t pos)
{
    int len = games.length();

    if(len <= 0) {
        game = "";
        return;
    }

    int i;
    size_t start    = 0;
    size_t end      = (len - 1);

    for(i=pos; i>0; i--) {                                                      // find starting coma
        if(games[i] == ',') {
            start = i + 1;                                                      // store position without that coma
            break;
        }
    }

    for(i=pos; i<len; i++) {                                                    // find ending coma
        if(games[i] == ',') {
            end = i - 1;                                                        // store position without that coma
            break;
        }
    }

    if(start >= (size_t) len) {                                                 // if start would be out of range, fix it
        start = len - 1;
    }
    
    if(end >= (size_t) len) {                                                   // if end would be out of range, fix it
        end = len - 1;
    }
    
    game = games.substr(start, (end - start + 1));                              // get only the single game
}

void ImageList::getResultByIndex(int index, char *bfr)
{
    memset(bfr, 0, 68);                                                         // clear the memory
    
    if(index < 0 || index >= (int) vectorOfResults.size()) {                    // if out of range
        return;
    }

    int imageIndex = vectorOfResults[index].imageIndex;

    strncpy(bfr, vectorOfImages[imageIndex].imageName.c_str(), 64);             // copy in the name of image
    
    int len = strlen(bfr);
    if(len < 12) {                                                              // pad the file name with spaces to 12 chars total
        int i;
        
        for(i=len; i<12; i++) {
            bfr[i] = ' ';
        }
    }
    
    if(vectorOfImages[imageIndex].marked) {                                     // if image is marked for download
        strcat(bfr, " * ");                                                     // add ' * ' string
    } else {
        strcat(bfr, " - ");                                                     // add ' - ' string
    }

    int imgNameLen = strlen(bfr);
    int lenOfRest = 67 - imgNameLen;

    strncpy(bfr + imgNameLen, vectorOfResults[index].game.c_str(), lenOfRest);  // copy in the name of game
}

void ImageList::markImage(int index)
{
    if(index < 0 || index >= (int) vectorOfResults.size()) {                    // if out of range
        return;
    }

    int imageIndex = vectorOfResults[index].imageIndex;                         // get image index for selected result
    
    if(imageIndex < 0 || imageIndex >= (int) vectorOfImages.size()) {           // if out of range
        return;
    }
    
    vectorOfImages[imageIndex].marked = !vectorOfImages[imageIndex].marked;     // toggle the marked flag
}

int ImageList::getSearchResultsCount(void)
{
    return vectorOfResults.size();
}

bool ImageList::getFirstMarkedImage(std::string &url, int &checksum, std::string &filename)
{
    int cnt = (int) vectorOfImages.size();

    for(int i=0; i<cnt; i++) {                                                  // go through images and see which one is checked
        if(vectorOfImages[i].marked) {
            url         = vectorOfImages[i].url;
            checksum    = vectorOfImages[i].checksum;
            filename    = vectorOfImages[i].imageName;           

            vectorOfImages[i].marked = false;                                   // unmark it, return success
            return true;
        }
    }

    url         = "";
    checksum    = 0;
    filename    = "";

    return false;                                                               // not found, fail
}

void ImageList::refreshList(void)
{
    unlink(IMAGELIST_LOCAL);                // delete current image list

    vectorOfImages.clear();                 // remove the loaded list from memory
    isLoaded = false;

    exists();                               // this should now start the new image list download
}


