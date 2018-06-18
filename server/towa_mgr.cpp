#include <iostream>
#include <string>
#include <cstdlib>
#include <unordered_set>
#include <vector>
#include <unistd.h> /* for fork */
#include <sys/types.h> /* for pid_t */
#include <sys/wait.h> /* for wait */
#include <sys/stat.h>
#include <dirent.h>
#include "towa_mgr.h"
#include "tinyxml2.h"

using namespace std;
using namespace tinyxml2;

TowaServlet::TowaServlet(const char *n, const char *u) {
    this->name = string(n);
    this->path = string(u);
}

TowaServlet *TowaMgr::readWebXML(string war) {
    XMLDocument doc;
    string pathWebXml = string("./classpath/") + war + "/WEB-INF/web.xml";
    doc.LoadFile(pathWebXml.c_str());
    XMLElement *elt = doc.FirstChildElement( "web-app" )->FirstChildElement( "servlet-mapping" );
    const char *servletName = elt->FirstChildElement("servlet-name")->GetText();
    const char *urlPattern = elt->FirstChildElement("url-pattern")->GetText();
    
    TowaServlet *servlet = new TowaServlet(servletName, urlPattern);
    return servlet;
}

void TowaMgr::uncompressWar(string war_name) {
//    mode_t mask = getumask();
    string war_file = "unzip " + war_name + ".war -d " + war_name;
    mkdir(war_name.c_str(), 0775);
    popen(war_file.c_str(), "r");
}

vector<TowaServlet*> *TowaMgr::scanClasspath() {
    unordered_set<string> wars = {};
    unordered_set<string> dirs = {};
    vector<TowaServlet*> *servlets = new vector<TowaServlet*>();
    
    DIR *dirp = opendir("./classpath/");
    struct dirent *dp;
    while ((dp = readdir(dirp)) != NULL) {
        string tmp(dp->d_name);
        
        if (dp->d_type == DT_DIR) dirs.insert(tmp);
        if (dp->d_type == DT_REG && tmp.compare(tmp.size() - 4, 4, ".war", 4) == 0) wars.insert(tmp.substr(0, tmp.size() - 4));
    }
    closedir(dirp);

//    for (const string&elem : dirs) cout << "[" << elem << "]" << endl;
    for (const string&war : wars) {
        if (dirs.count(war) == 0) uncompressWar(string("./classpath/") + war);
        
        servlets->push_back(readWebXML(war));
    }
    
    return servlets;
}

void TowaMgr::launchAppPool() {
    pid=fork();
    if (pid==0) {    // Only executed by the chile process
        static char *argv[]={(char*)"towa"};
        execv("./towa" ,argv);
        exit(127); /* only if execv fails */
    }
    
    pid_t res = waitpid(pid, 0, WNOHANG);
    if (res != 0) throw FAILURE_TOWA_CANNOT_SPAWN;
}

TowaMgr::TowaMgr(TowaType t) {
    switch(t) {
        case TowaType::namedpipe:
            ipc = new TowaPipe(true);
            break;
        case TowaType::tcpip:
            ipc = new TowaTCP();
            break;
        case TowaType::sharedmem:
            ipc = new TowaSharedMem();
            break;
        default:
            throw FAILURE_TOWA_INVALID_TYPE;
    }
}

void TowaMgr::start() {
    scanClasspath();
    launchAppPool();
}

void TowaMgr::check() {
    pid_t res = waitpid(pid, 0, WNOHANG);
    
    if (res != 0) this->start();
//    cout << "Towa process: " << res << endl;
}

Message *TowaMgr::sendMsg(string verb, string classname, string querystring) {
    this->check();
    return ipc->sendMsg(verb, classname, querystring);
}
