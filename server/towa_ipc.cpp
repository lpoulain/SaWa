#include <cstddef>
#include <iostream>
#include <string>
#include <cstdlib>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include "towa_ipc.h"
#include "util.h"

using namespace std;

///////////////////////////////////////////////////////////////////////////////
// Message
///////////////////////////////////////////////////////////////////////////////

Message::Message(FILE *fp, bool hasHeader = false) {
    int contentSize = 0;
    
    this->size = 0;
    fread((char*)&(this->size), 4, 1, fp);
    Util::dumpMem((uint8_t*)&this->size, 4);
    
    if (this->size == 0) this->content = nullptr;

    if (hasHeader) {
        fread((char*)&(this->header), 1, 1, fp);
        this->size--;
    }
    else {
        contentSize = this->size;
    }    

    this->content = new uint8_t[this->size];
    fread((char*)this->content, this->size, 1, fp);
}

Message::Message(int size, uint8_t *content) {
    this->size = size;
    this->content = content;
}

Message::Message(string s) {
    this->size = s.length();
    this->content = new uint8_t[this->size];
    memcpy(this->content, s.c_str(), this->size);
}

Message::~Message() {
    delete [] this->content;
}

void Message::sendTo(FILE* fp) {
    fwrite((char*)&(this->size), 4, 1, fp);
    fwrite((char*)this->content, this->size, 1, fp);
}

uint8_t *Message::getContent() {
    return this->content;
}

int Message::getSize() {
    return this->size;
}

string Message::getString() {
    return string((const char*)this->content, this->size);
}

char Message::getHeader() {
    return this->header;
}

///////////////////////////////////////////////////////////////////////////////
// TowaPipe
///////////////////////////////////////////////////////////////////////////////

TowaPipe::TowaPipe(bool client) {
    int res;
    const char *ping_mode, *pong_mode;
    
    if (client) {
        ping_mode = "wb";
        pong_mode = "rb";
    }
    else {
        ping_mode = "rb";
        pong_mode = "wb";
    }

    // Creates the named pipe
    // If errno=17 => file already exists, so there is no problem
    // For any other error, throw an exception
    umask(0);
    res = mknod("/tmp/towa_ping", S_IFIFO|0666, 0);
    if (res < 0 && errno != 17) throw FAILURE_TOWA_IPC_CANNOT_CREATE_PIPE;
    
    res = mknod("/tmp/towa_pong", S_IFIFO|0666, 0);
    if (res < 0 && errno != 17) throw FAILURE_TOWA_IPC_CANNOT_CREATE_PIPE;

    // Opens the named pipes
/*    fp_ping = fopen("/tmp/towa_ping", ping_mode);
    if (fp_ping == 0) throw FAILURE_TOWA_IPC_CANNOT_OPEN_PIPE;
    
    fp_pong = fopen("/tmp/towa_pong", pong_mode);
    if (fp_pong == 0) throw FAILURE_TOWA_IPC_CANNOT_OPEN_PIPE;*/
    
}

Message *TowaPipe::sendMsg(string verb, string classname, string querystring) {
    Message *msg_in;
    Message msg_out(verb + string("|") + classname + string("|") + querystring);
    
    fp_ping = fopen("/tmp/towa_ping", "w");
    msg_out.sendTo(fp_ping);
    fclose(fp_ping);
    
    fp_pong = fopen("/tmp/towa_pong", "r");
    msg_in = new Message(fp_pong, true);
    fclose(fp_pong);
    
    return msg_in;
}

void TowaPipe::listenToMsg(Message *(*callback)(Message *)) {
    Message *msg_in, *msg_out;
    
    fp_ping = fopen("/tmp/towa_ping", "r");
    msg_in = new Message(fp_ping);
    fclose(fp_ping);
    
    msg_out = callback(msg_in);
    
    fp_pong = fopen("/tmp/towa_pong", "w");
    msg_out->sendTo(fp_pong);
    fclose(fp_pong);
}

TowaPipe::~TowaPipe() { }

///////////////////////////////////////////////////////////////////////////////
// TowaTCP / TowaSharedMem stubs
///////////////////////////////////////////////////////////////////////////////

Message *TowaTCP::sendMsg(string verb, string classname, string querystring) { return NULL; }
void TowaTCP::listenToMsg(Message *(*callback)(Message *)) { }

Message *TowaSharedMem::sendMsg(string verb, string classname, string querystring) { return NULL; }
void TowaSharedMem::listenToMsg(Message *(*callback)(Message *)) { }
