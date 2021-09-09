/*
 * sniffex.c
 * Copyright (c) 2005 The Tcpdump Group
 */

#define APP_NAME		"sniffex"
#define APP_DESC		"Sniffer example using libpcap"
#define APP_COPYRIGHT	"Copyright (c) 2005 The Tcpdump Group"
#define APP_DISCLAIMER	"THERE IS ABSOLUTELY NO WARRANTY FOR THIS PROGRAM."

#include <pcap.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>


FILE *outfile;
static int start_write ;


/* default snap length (maximum bytes per packet to capture) */
#define SNAP_LEN 1518

/* ethernet headers are always exactly 14 bytes [1] */
#define SIZE_ETHERNET 14

/* Ethernet addresses are 6 bytes */
#define ETHER_ADDR_LEN	6

/* 以太网头部 */
struct ethernet_header {
        u_char  ether_dhost[ETHER_ADDR_LEN];    /* destination host address */
        u_char  ether_shost[ETHER_ADDR_LEN];    /* source host address */
        u_short ether_type;                     /* IP? ARP? RARP? etc */
};

/* IP 头部 */
struct ip_header {
        u_char  ip_vhl;                 /* version << 4 | header length >> 2 */
        u_char  ip_tos;                 /* type of service */
        u_short ip_len;                 /* total length */
        u_short ip_id;                  /* identification */
        u_short ip_off;                 /* fragment offset field */
        #define IP_RF 0x8000            /* reserved fragment flag */
        #define IP_DF 0x4000            /* don't fragment flag */
        #define IP_MF 0x2000            /* more fragments flag */
        #define IP_OFFMASK 0x1fff       /* mask for fragmenting bits */
        u_char  ip_ttl;                 /* time to live */
        u_char  ip_p;                   /* protocol */
        u_short ip_sum;                 /* checksum */
        struct  in_addr ip_src,ip_dst;  /* source and dest address */
};
#define IP_HL(ip)               (((ip)->ip_vhl) & 0x0f)
#define IP_V(ip)                (((ip)->ip_vhl) >> 4)

/* udp头部  */ 
struct upd_header{
	u_short th_sport; /* source port */
	u_short th_dport; /* destination port */
	u_short th_length; /* length  */
	u_short th_checksum; /* checksum */
};

/* rtp头部 */
/*
struct rtp_header{
	unsigned char cc : 4;		
	unsigned char x : 1;	
	unsigned char p : 1;
	unsigned char v : 2;	
	unsigned char pt : 7;
	unsigned char m : 1;
	unsigned short seq;	
	unsigned int ts;
	unsigned int ssrc; 
};
*/

typedef struct rtp_header {
#ifdef ORTP_BIGENDIAN
	uint16_t version : 2;
	uint16_t padbit : 1;
	uint16_t extbit : 1;
	uint16_t cc : 4;
	uint16_t markbit : 1;
	uint16_t paytype : 7;
#else
	uint16_t cc : 4;
	uint16_t extbit : 1;
	uint16_t padbit : 1;
	uint16_t version : 2;
	uint16_t paytype : 7;
	uint16_t markbit : 1;
#endif
	uint16_t seq_number;
	uint32_t timestamp;
	uint32_t ssrc;
	uint32_t csrc[16];
} rtp_header_t;


void
got_packet(u_char *args, const struct pcap_pkthdr *header, const u_char *packet);

void
print_payload(const u_char *payload, int len);

void
print_hex_ascii_line(const u_char *payload, int len, int offset);

void
print_app_banner(void);

void
print_app_usage(void);

/*
 * app name/banner
 */
void
print_app_banner(void)
{

	printf("%s - %s\n", APP_NAME, APP_DESC);
	printf("%s\n", APP_COPYRIGHT);
	printf("%s\n", APP_DISCLAIMER);
	printf("\n");

return;
}

/*
 * print help text
 */
void
print_app_usage(void)
{

	printf("Usage: %s [interface]\n", APP_NAME);
	printf("\n");
	printf("Options:\n");
	printf("    interface    Listen on <interface> for packets.\n");
	printf("\n");

return;
}

/*
 * print data in rows of 16 bytes: offset   hex   ascii
 *
 * 00000   47 45 54 20 2f 20 48 54  54 50 2f 31 2e 31 0d 0a   GET / HTTP/1.1..
 */
void
print_hex_ascii_line(const u_char *payload, int len, int offset)
{

	int i;
	int gap;
	const u_char *ch;

	/* offset */
	printf("%05d   ", offset);

	/* hex */
	ch = payload;
	for(i = 0; i < len; i++) {
		printf("%02x ", *ch);
		ch++;
		/* print extra space after 8th byte for visual aid */
		if (i == 7)
			printf(" ");
	}
	/* print space to handle line less than 8 bytes */
	if (len < 8)
		printf(" ");

	/* fill hex gap with spaces if not full line */
	if (len < 16) {
		gap = 16 - len;
		for (i = 0; i < gap; i++) {
			printf("   ");
		}
	}
	printf("   ");

	/* ascii (if printable) */
	ch = payload;
	for(i = 0; i < len; i++) {
		if (isprint(*ch))
			printf("%c", *ch);
		else
			printf(".");
		ch++;
	}

	printf("\n");

return;
}

/*
 * print packet payload data (avoid printing binary data)
 */
void
print_payload(const u_char *payload, int len)
{

	int len_rem = len;
	int line_width = 16;			/* number of bytes per line */
	int line_len;
	int offset = 0;					/* zero-based offset counter */
	const u_char *ch = payload;

	if (len <= 0)
		return;

	/* data fits on one line */
	if (len <= line_width) {
		print_hex_ascii_line(ch, len, offset);
		return;
	}

	/* data spans multiple lines */
	for ( ;; ) {
		/* compute current line length */
		line_len = line_width % len_rem;
		/* print line */
		print_hex_ascii_line(ch, line_len, offset);
		/* compute total remaining */
		len_rem = len_rem - line_len;
		/* shift pointer to remaining bytes to print */
		ch = ch + line_len;
		/* add offset */
		offset = offset + line_width;
		/* check if we have line width chars or less */
		if (len_rem <= line_width) {
			/* print last line and get out */
			print_hex_ascii_line(ch, len_rem, offset);
			break;
		}
	}

return;
}

/*
 * dissect/print packet
 */
void
got_packet(u_char *args, const struct pcap_pkthdr *header, const u_char *packet)

{


	/*
	* 以太网头部是固定的
	* IP 头部是计算出来的
	* upd头部也是固定的
	* rtp头部也是固定的
	*/

	static int count = 1;                   /* packet counter */
	/*
	printf("count = %d", count);
	if (count> 5000) {
		printf("end");
		return;
	}
	*/

	/* declare pointers to packet headers */
	const struct ethernet_header *ethernet_hdr;  /* The ethernet header [1] */
	const struct ip_header *ip_hdr;              /* The IP header */
	const struct upd_header *udp_hdr; /* The UDP header */
	const struct rtp_header *rtp_hdr;
	const char *payload;                    /* Packet payload */

	int size_ip;
	int size_udp = 8;
	int size_payload;

	printf("\nPacket number %d:\n", count);
	count++;

	// 网卡头部
	ethernet_hdr = (struct ethernet_header*)(packet);

	// IP头部
	ip_hdr = (struct ip_header*)(packet + SIZE_ETHERNET);
	size_ip = IP_HL(ip_hdr)*4;  // 计算出来的
	if (size_ip < 20) {
		printf("   * Invalid IP header length: %u bytes\n", size_ip);
		return;
	}

	/* print source and destination IP addresses */
	printf("       From: %s\n", inet_ntoa(ip_hdr->ip_src));
	printf("         To: %s\n", inet_ntoa(ip_hdr->ip_dst));

	// 其他的协议的包，直接返回
	switch(ip_hdr->ip_p) {
		case IPPROTO_UDP:
			printf("   Protocol: UDP\n");
			break;
		default:
			printf("   Protocol: unknown\n");
			return;
	}

	udp_hdr = (struct upd_header*)(packet + SIZE_ETHERNET + size_ip);


	printf("   Src port: %d\n", ntohs(udp_hdr->th_sport));
	printf("   Dst port: %d\n", ntohs(udp_hdr->th_dport));

	printf("Number of ip ip->ip_len: %d\n", ntohs(ip_hdr->ip_len));


#define SIZE_RTP 12;
	rtp_hdr = (struct rtp_header*)(packet + SIZE_ETHERNET + size_ip + size_udp);

	printf("Number of cc: %d\n", rtp_hdr->cc);
	printf("Number of x: %d\n", rtp_hdr->extbit);
	printf("Number of p: %d\n", rtp_hdr->padbit);
	printf("Number of v: %d\n", rtp_hdr->version);
	printf("Number of pt: %d\n", rtp_hdr->paytype);
	printf("Number of m: %d\n", rtp_hdr->markbit);
	printf("Number of seq: %d\n", ntohs(rtp_hdr->seq_number));  
	printf("ts: %d\n", ntohl(rtp_hdr->timestamp));
	printf("Number of ssrc: %x\n", ntohl(rtp_hdr->ssrc));

	printf("count = %d", count);

	// payload = (u_char *)(packet + SIZE_ETHERNET + size_ip + size_udp + SIZE_RTP);

	payload = (u_char *)(packet + SIZE_ETHERNET + size_ip + 20);
	// print_payload(payload, 160);


	int user_ts = ntohl(rtp_hdr->timestamp);

	if ( (start_write == 0) &&  (user_ts % (8000) == 0)) {
		// 开始写文件
		start_write = 1;
	}

	// 将数据写入到文件中
	// start_write == 1 && rtp_hdr->paytype == 9
	if (start_write == 1) {
		int ret = fwrite(payload, 1, 160, outfile);
		printf("write file");
	}
	// sleep(1);
	// sleep(1);
	// voice data
	/*
	rtp_payload = payload + SIZE_RTP;

	int size_rtp_payload = size_payload - SIZE_RTP;

	
	if (size_rtp_payload > 0) {
		printf("   Payload (%d bytes):\n", rtp_payload);
		print_payload(rtp_payload, size_rtp_payload);
	}
	*/
	

	/*
	 *  OK, this packet is TCP.
	 */



return;
}

int main(int argc, char **argv)
{
	start_write = 0;

	char *filename = "rr_recv_gsm__1111.wav";
	outfile = fopen(filename, "wb");
	if (outfile == NULL) {

		perror("Cannnot open file for writing\n");
		return -1;
	} else {
	
		printf("open file \n");
	}

	char *dev = NULL;			/* capture device name */
	char errbuf[PCAP_ERRBUF_SIZE];		/* error buffer */
	pcap_t *handle;				/* packet capture handle */

	// char filter_exp[] = "udp src portrange 1025-65535 && dst host 18955666655.phone.com";		/* filter expression [3] */
	// char filter_exp[] = "udp src portrange 1025-65535 && src host 18833555533.phone.com";
	// char filter_exp[] = "udp port 7788";
	// char filter_exp[] = "udp src portrange 1025-65535 && src host 18955666688.linphone.com";
	char filter_exp[] = "udp src portrange 1025-65535 && src host 18955666655.phone.com";


	struct bpf_program fp;			/* compiled filter program (expression) */
	bpf_u_int32 mask;			/* subnet mask */
	bpf_u_int32 net;			/* ip */
	int num_packets = 1000;			/* number of packets to capture */

	print_app_banner();

	/* check for capture device name on command-line */
	if (argc == 2) {
		dev = argv[1];
	}
	else if (argc > 2) {
		fprintf(stderr, "error: unrecognized command-line options\n\n");
		print_app_usage();
		exit(EXIT_FAILURE);
	}
	else {
		/* find a capture device if not specified on command-line */
		dev = pcap_lookupdev(errbuf);
		if (dev == NULL) {
			fprintf(stderr, "Couldn't find default device: %s\n",
			    errbuf);
			exit(EXIT_FAILURE);
		}
	}

	/* get network number and mask associated with capture device */
	if (pcap_lookupnet(dev, &net, &mask, errbuf) == -1) {
		fprintf(stderr, "Couldn't get netmask for device %s: %s\n",
		    dev, errbuf);
		net = 0;
		mask = 0;
	}

	/* print capture info */
	printf("Device: %s\n", dev);
	printf("Number of packets: %d\n", num_packets);
	printf("Filter expression: %s\n", filter_exp);

	/* open capture device */
	handle = pcap_open_live(dev, SNAP_LEN, 1, 1000, errbuf);
	if (handle == NULL) {
		fprintf(stderr, "Couldn't open device %s: %s\n", dev, errbuf);
		exit(EXIT_FAILURE);
	}

	/* make sure we're capturing on an Ethernet device [2] */
	if (pcap_datalink(handle) != DLT_EN10MB) {
		fprintf(stderr, "%s is not an Ethernet\n", dev);
		exit(EXIT_FAILURE);
	}

	/* compile the filter expression */
	if (pcap_compile(handle, &fp, filter_exp, 0, net) == -1) {
		fprintf(stderr, "Couldn't parse filter %s: %s\n",
		    filter_exp, pcap_geterr(handle));
		exit(EXIT_FAILURE);
	}

	/* apply the compiled filter */
	if (pcap_setfilter(handle, &fp) == -1) {
		fprintf(stderr, "Couldn't install filter %s: %s\n",
		    filter_exp, pcap_geterr(handle));
		exit(EXIT_FAILURE);
	}

	/* now we can set our callback function */
	pcap_loop(handle, num_packets, got_packet, NULL);

	/* cleanup */
	pcap_freecode(&fp);
	pcap_close(handle);

	fclose(outfile);
	printf("\nCapture complete.\n");

return 0;
}

