#include <stdio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "rpc_common.h"
#include "De_Serialization/serialize.h"


void
rpc_send_recv(ser_buff_t *client_send_ser_buff,
              ser_buff_t *client_recv_ser_buff)
{
    int sock_fd = 0;
    long rc = 0;
    unsigned int addr_len = sizeof(struct sockaddr_in);
    struct sockaddr_in server_addr;
    
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    struct hostent *host = (struct hostent *)gethostbyname(SERVER_IP);
    server_addr.sin_addr = *((struct in_addr *)host->h_addr);
    
    sock_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if(sock_fd == -1)
    {
        perror("socket()");
        exit(EXIT_FAILURE);
    }
    
    rc = sendto(sock_fd, client_send_ser_buff->b,
                get_serialize_buffer_data_size(client_send_ser_buff),
                0, (struct sockaddr *)&server_addr,
                sizeof(struct sockaddr_in));
    if(rc == -1)
    {
        perror("sendto()");
        exit(EXIT_FAILURE);
    }
    
    printf("%s() : %ld bytes sent\n", __FUNCTION__, rc);
    
    rc = recvfrom(sock_fd, client_recv_ser_buff->b,
                get_serialize_buffer_length(client_recv_ser_buff),
                0, (struct sockaddr *)&server_addr,
                &addr_len);
    if(rc == -1)
    {
        perror("recvfrom()");
        exit(EXIT_FAILURE);
    }
    
    printf("%s() : %ld bytes received\n", __FUNCTION__, rc);
}

ser_buff_t *
multiply_client_stub_marshal(int a, int b)
{
    ser_buff_t *client_send_ser_buffer = NULL;
    init_serialized_buffer_defined_size(&client_send_ser_buffer,
                                        MAX_SEND_RECV_BUFFER_SIZE);
    
    serialize_data(client_send_ser_buffer, (char *)&a, sizeof(int));
    serialize_data(client_send_ser_buffer, (char *)&b, sizeof(int));
    
    return client_send_ser_buffer;
}

int
multiply_client_stub_unmarshal(ser_buff_t *client_recv_ser_buff)
{
    int result = 0;
    
    deserialize_data((char *)&result, client_recv_ser_buff, sizeof(int));
    
    return result;
}

int
multiply_rpc(int a, int b)
{
    ser_buff_t *client_send_ser_buff = multiply_client_stub_marshal(a, b);
    ser_buff_t *client_recv_ser_buff = NULL;
    
    init_serialized_buffer_defined_size(&client_recv_ser_buff,
                                        MAX_SEND_RECV_BUFFER_SIZE);
    rpc_send_recv(client_send_ser_buff, client_recv_ser_buff);
    
    int res = 0;
    
    res = multiply_client_stub_unmarshal(client_recv_ser_buff);
    
    free_serialize_buffer(client_send_ser_buff);
    free_serialize_buffer(client_recv_ser_buff);
    
    client_send_ser_buff = NULL;
    client_recv_ser_buff = NULL;
    
    return res;
}

int
to_multiply(void)
{
    int a, b;
    
    printf("Enter the 1st number : ");
    scanf("%d", &a);
    printf("Enter the 2nd number : ");
    scanf("%d", &b);
    
    int res = multiply_rpc(a, b);
    
    printf("result = %d\n", res);
        
    return 0;
}

