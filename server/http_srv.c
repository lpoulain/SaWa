#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <search.h>
#include "sawa.h"
#include "thread_pool.h"

const char *HTTP_200 = "HTTP/1.1 200 OK\nKeep-Alive: timeout=15, max=95\nConnection: Keep-Alive\n";
const char *HTTP_404 = "HTTP/1.1 404 Not Found\nContent-Type: text/html; charset=UTF-8\nContent-Length: 12\n\nError 404 - Not found\n\n";
const char *HTTP_500 = "HTTP/1.1 500 Server Error\n\n";
const char *root_dir = "./wwwroot";
const char *index_html = "index.html";
int HTTP_200_len;
int HTTP_404_len;
int HTTP_500_len;
int root_dir_len;
int index_html_len;
struct hsearch_data htab;

#define request_message_len 1016

int strncasestr(const char *s, const char *find, const int max)
{
	char c, sc = 0;
	size_t len;
    const char *s_end = s + max;
    
	if ((c = *find++) != 0) {
		c = (char)tolower((unsigned char)c);
		len = strlen(find);
		do {
			do {
                sc = *s++;
                if (s >= s_end) return 0;
			} while ((char)tolower((unsigned char)sc) != c);
		} while (strncasecmp(s, find, len) != 0);
		s--;
	}
//    printf("Found: %lx %lx\n", (unsigned long)s, (unsigned long)s_end);
//    printf("{{%s}}\n", s);
	return 1;
}

struct request_message {
    unsigned char data[request_message_len];
    struct request_message *next;
};

struct web_file {
    char *content;
    int size;
};

struct web_file *get_file(char *path) {
    ENTRY e, *ep;
    struct stat st;
    int fd, file_size, n;
    struct web_file *the_file;
    
    // Check if the file is cached. If it is, return it
    e.key = path;
    n = hsearch_r(e, FIND, &ep, &htab);
    if (n) {
        free(path);
        return (struct web_file *)ep->data;
    }
  
    // If not, find the file size
    stat(path, &st);
    file_size = st.st_size;

    // The file doesn't exist
    if (file_size == 0) return NULL;

    // The file exists, loads it in memory
    the_file = (struct web_file *)malloc(sizeof(struct web_file));
    the_file->size = file_size;
    the_file->content = malloc(file_size);
    
    fd = open(path, O_RDONLY);
    lseek(fd, 0, SEEK_SET);
    read(fd, the_file->content, the_file->size);
    close(fd);

    // Then save it in the cache
    e.key = path;
    e.data = the_file;
    hsearch_r(e, ENTER, &ep, &htab);
    
    return the_file;
}

// Process the HTTP request
// Request 0 if close connection
// Request 1 if keep alive
int process_HTTP_request(int socket_fd, struct request_message *msg, unsigned int size) {
    struct request_message *tmp_msg = msg;
    unsigned int offset;
    unsigned char op;
    char *buffer_out, *buffer_in = (char*)msg;
    char *url;
    int first_line_end = strlen(buffer_in), url_start, url_end, url_length, ext_start, header_size;
    int keep_alive = 0;
    struct web_file *the_file;

    // Analyze the HTTP request
    if (strncmp(buffer_in, "GET /", 5)) {
        write(socket_fd, HTTP_500, HTTP_500_len);
        free(buffer_in);
        return;
    }
    
    url_start = 4;
    url_end = 5;
    while (url_end < first_line_end && buffer_in[url_end] != ' ' && buffer_in[url_end] != '?')
        url_end++;

    url_length = url_end - url_start;

    if (buffer_in[url_end-1] == '/')
        url = malloc(url_length + root_dir_len + index_html_len + 1);
    else
        url = malloc(url_length + root_dir_len + 1);
    
    strcpy(url, root_dir);
    strncpy(url + root_dir_len, buffer_in + url_start, url_length);

    if (buffer_in[url_end-1] == '/') {
        strcpy(url + root_dir_len + url_length, index_html);
        url_length += index_html_len;
    }
        
    url[url_length + root_dir_len] = 0;
    keep_alive = strncasestr(buffer_in, "Connection: Keep-Alive", request_message_len);
    
    // Analysis over. Processing the request
    if (debug) printf("[Socket %d] requested file: [%s]. Keep-alive=%d\n", socket_fd, url, keep_alive);
    
//    if (keep_alive) printf("{%s}\n", buffer_in);

    the_file = get_file(url);
    
    // We are done reading the HTTP request, free the resource
//    free(url);
    while (tmp_msg != NULL) {
        tmp_msg = tmp_msg->next;
        free(msg);
        msg = tmp_msg;
    }
    
    // If the file doesn't exist, return an HTTP 404 message
    if (!the_file) {
        write(socket_fd, HTTP_404, HTTP_404_len);
        return;
    }
    
    // Otherwise, write the HTTP headers
    buffer_out = malloc(200);
    sprintf(buffer_out, "%sContent-Length: %d\nContent-Type: text/html;\n\n", HTTP_200, the_file->size);
    header_size = strlen(buffer_out);
    
    write(socket_fd, buffer_out, header_size);
    write(socket_fd, the_file->content, the_file->size);
    
    free(buffer_out);
    
    return keep_alive;
}

void HTTP_listen(struct connection_thread *thread_info) {
    int socket_fd = thread_info->client_sock;
    int n = 1, size=0;
    unsigned int expected_size;
    struct request_message *top_msg = (struct request_message *)malloc(sizeof(struct request_message));
    memset(top_msg, 0, sizeof(struct request_message));
    struct request_message *msg = top_msg, *tmp_msg;
    int keep_alive;

    while (1) {
        n = read(socket_fd, (char *)msg, request_message_len);
        
        if (n <= 0) {
            free(top_msg);
            return;
        }
        if (debug) printf("[Socket %d] %d bytes request\n", socket_fd, n);
        
        // If this is the end of the message, send it
        if (n < request_message_len || strcmp((char*)msg + request_message_len - 4, "\r\n\r\n")) {
            // If the connection is not Keep-Alive, break the connection
            if (!process_HTTP_request(socket_fd, top_msg, n)) return;
            
            // Otherwise be ready for another message
            top_msg = (struct request_message *)malloc(sizeof(struct request_message));
            top_msg->next = NULL;
            msg = top_msg;
        }
        
        // If the message is too large to fit in a request_message structure
        // Create another one and keep listening
        else {
            tmp_msg = (struct request_message *)malloc(sizeof(struct request_message));
            msg->next = tmp_msg;
            msg = tmp_msg;
        }
    }
}

void HTTP_init() {
    op_listen = HTTP_listen;
    server_port = 8080;
    
    HTTP_200_len = strlen(HTTP_200);
    HTTP_404_len = strlen(HTTP_404);
    HTTP_500_len = strlen(HTTP_500);
    root_dir_len = strlen(root_dir);
    index_html_len = strlen(index_html);    
    
    hcreate_r(100, &htab);
}

void HTTP_cleanup() {
    hdestroy_r(&htab);
}