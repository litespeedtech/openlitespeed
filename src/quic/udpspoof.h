#ifndef _UDPSPOOF_H_
#define _UDPSPOOF_H_

#include <sys/socket.h>
typedef int    rawsock_t;

#ifdef __cplusplus
extern "C" {
#endif

struct ipheader {
    unsigned char      iph_ihl:4, iph_ver:4;
    unsigned char      iph_tos;
    unsigned short int iph_len;
    unsigned short int iph_ident;
    unsigned short int iph_offset;
    unsigned char      iph_ttl;
    unsigned char      iph_protocol;
    unsigned short int iph_chksum;
    unsigned int       iph_sourceip;
    unsigned int       iph_destip;
};


// UDP header's structure
struct udpheader {
    unsigned short int     udph_srcport;
    unsigned short int     udph_destport;
    unsigned short int udph_len;
    unsigned short int udph_chksum;
};


struct sockaddr;
rawsock_t init_rawsock();
int send_udp_spoof( rawsock_t sd, char * raw_sock_buf, int buf_size, 
            unsigned int src_ip, unsigned int dst_ip, 
            unsigned short int src_port, unsigned short int dst_port,
            unsigned short ip_id );


#ifdef __cplusplus
}
#endif


#endif

