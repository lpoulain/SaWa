int tcp_client_connect(struct socket **conn_socket);
int tcp_client_send(struct socket *sock, const char *buf, const size_t length, unsigned long flags);
int tcp_client_receive(struct socket *sock, char *str, unsigned long flags);
