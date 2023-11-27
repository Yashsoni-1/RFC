#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "De_Serialization/serialize.h"
#include "rpc_common.h"

int
multiply(int a, int b)
{
    return a * b;
}

int
multiply_server_stub_unmarshal(ser_buff_t *server_recv_ser_buff)
{
    int a, b;
    
    deserialize_data((char *)&a, server_recv_ser_buff, sizeof(int));
    deserialize_data((char *)&b, server_recv_ser_buff, sizeof(int));
    
    printf("%s() : recvd a is %d and b is %d", __FUNCTION__, a, b);
    
    return multiply(a, b);
}

void
multiply_server_stub_marshal(int result, ser_buff_t *server_send_ser_buff)
{
    serialize_data(server_send_ser_buff, (char *)&result, sizeof(int));
}

void
rpc_server_process_msg(ser_buff_t *server_send_ser_buff,
                       ser_buff_t *server_recv_ser_buff)
{
    int res = multiply_server_stub_unmarshal(server_recv_ser_buff);
    
    multiply_server_stub_marshal(res, server_send_ser_buff);
}

int to_rcp_server(void)
{
    int sock_fd = 0;
    long rc = 0;
    int opt = 1;
    unsigned int addr_len = sizeof(struct sockaddr_in);
    struct sockaddr_in server_addr, client_addr;
    
    sock_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if(sock_fd == -1)
    {
        perror("socket()");
        exit(EXIT_FAILURE);
    }
    
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;
    
    rc = setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt));
    if(rc == -1)
    {
        perror("setsockopt()");
        exit(EXIT_FAILURE);
    }
    
    rc = setsockopt(sock_fd, SOL_SOCKET, SO_REUSEPORT, (char *)&opt, sizeof(opt));
    if(rc == -1)
    {
        perror("setsockopt()");
        exit(EXIT_FAILURE);
    }
    
    rc = bind(sock_fd, (struct sockaddr *)&server_addr,
              sizeof(struct sockaddr_in));
    if(rc == -1)
    {
        perror("bind()");
        exit(EXIT_FAILURE);
    }
    
    ser_buff_t *server_recv_ser_buff = NULL,
    *server_send_ser_buff = NULL;
    
    init_serialized_buffer_defined_size(&server_recv_ser_buff,
                                        MAX_SEND_RECV_BUFFER_SIZE);
    init_serialized_buffer_defined_size(&server_send_ser_buff,
                                        MAX_SEND_RECV_BUFFER_SIZE);
    
    printf("Server ready to service rpc calls\n");
    
READ:
    
    reset_serialize_buffer(server_recv_ser_buff);
    
    rc = recvfrom(sock_fd, server_recv_ser_buff->b,
                  get_serialize_buffer_length(server_recv_ser_buff),
                  0, (struct sockaddr *)&client_addr,
                  &addr_len);
    if(rc == -1)
    {
        perror("recvfrom()");
        exit(EXIT_FAILURE);
    }
    
    printf("No of bytes recvd from client = %ld\n", rc);
    
    reset_serialize_buffer(server_send_ser_buff);
    
    rpc_server_process_msg(server_send_ser_buff,
                           server_recv_ser_buff);
    
    rc = sendto(sock_fd, server_send_ser_buff->b,
                get_serialize_buffer_data_size(server_send_ser_buff),
                0, (struct sockaddr *)&client_addr, addr_len);
    if(rc == -1)
    {
        perror("sendto()");
        exit(EXIT_FAILURE);
    }
    
    printf("rpc server replied with %ld bytes msg\n", rc);
    
    goto READ;
    
    return 0;
}
