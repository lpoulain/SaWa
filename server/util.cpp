#include <iostream>
#include <iomanip>
#include <string.h>

#include "util.h"
#include "sawa.h"
#include "server.h"
#include "display.h"

using namespace std;

int Util::debug_int;
string Util::debug_str;

void Util::setDebugInfo(int info) {
    debug_int = info;
}

void Util::setDebugInfo(string info) {
    debug_str = info;
}

void Util::displayError(int failure_code) {
    switch(failure_code) {
        case FAILURE_CREATE_SOCKET:
            cout << "Error: cannot create socket on port " << debug_int << endl;
            break;
        case FAILURE_BIND:
            cout << "Error: cannot bind socket on port " << debug_int << endl;
            break;
        case FAILURE_ACCEPT:
            cout << "Error: cannot accept incoming connection" << endl;
            break;
        case FAILURE_MUTEX_INIT:
            cout << "Error: cannot initialize mutex" << endl;
            break;
        case FAILURE_OPEN_FILE:
            cout << "Error: cannot open file '" << debug_str << "' (error=" << debug_int << ")" << endl;
            break;
        default:
            cout << "Error #" << failure_code << endl;
            break;
    }
}

int Util::strnCaseStr(const char *s, const char *find, const int max)
{
	char c, sc = 0;
	size_t len;
    const char *s_end = s + max;
    
	if ((c = *find++) != 0) {
		c = (char)tolower((uint8_t)c);
		len = strlen(find);
		do {
			do {
                sc = *s++;
                if (s >= s_end) return 0;
			} while ((char)tolower((uint8_t)sc) != c);
		} while (strncasecmp(s, find, len) != 0);
		s--;
	}

    return 1;
}

char ascii[256] = {
    '.', '.', '.', '.', '.', '.', '.', '.', '.', '.', '.', '.', '.', '.', '.', '.',
    '.', '.', '.', '.', '.', '.', '.', '.', '.', '.', '.', '.', '.', '.', '.', '.',
    ' ', '!', '"', '#', '$', '%', '&', '\'', '(', ')', '*', '+', ',', '-', '.', '/',
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', ':', ';', '<', '=', '>', '?',
    '@', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O',
    'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', '[', '\\', ']', '^', '_',
    '`', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o',
    'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', '{', '|', '}', '~', '.',
    '.', '.', '.', '.', '.', '.', '.', '.', '.', '.', '.', '.', '.', '.', '.', '.',
    '.', '.', '.', '.', '.', '.', '.', '.', '.', '.', '.', '.', '.', '.', '.', '.',
    '.', '.', '.', '.', '.', '.', '.', '.', '.', '.', '.', '.', '.', '.', '.', '.',
    '.', '.', '.', '.', '.', '.', '.', '.', '.', '.', '.', '.', '.', '.', '.', '.',
    '.', '.', '.', '.', '.', '.', '.', '.', '.', '.', '.', '.', '.', '.', '.', '.',
    '.', '.', '.', '.', '.', '.', '.', '.', '.', '.', '.', '.', '.', '.', '.', '.',
    '.', '.', '.', '.', '.', '.', '.', '.', '.', '.', '.', '.', '.', '.', '.', '.',
    '.', '.', '.', '.', '.', '.', '.', '.', '.', '.', '.', '.', '.', '.', '.', '.'
};

void Util::dumpMem(uint8_t *addr, int size) {
    int i, j=0, k;
    char ascii_row[16];
    
    if (debug_flag == 0) return;
    
    memset(ascii_row, ' ', 16);
    
    while (1) {
        if (j >= size) return;
        cout << " " << setfill('0') << setw(4) << hex << j;
        for (i=0; i<16; i++) {
            if (j >= size) {
                for (k=i; k<16; k++) cout << "   ";
                cout << "   ";
                for (k=0; k<16; k++) cout << ascii_row[k];
                cout << endl;
                return;
            }
            ascii_row[i] = ascii[addr[j]];
            cout << " " << setfill('0') << setw(2) << hex << int(addr[i]);
            j++;
        }
        cout << "   ";
        for (k=0; k<16; k++) cout << ascii_row[k];
        cout << endl;
        memset(ascii_row, ' ', 16);
    }
}
