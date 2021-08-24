#include "ortp/rtpsession.h"
#include "ortp/ortp.h"

uint32_t g_user_ts;
#define TIME_STAMP_INC 160
#define BYTES_PER_COUNT 65535

RtpSession * rtpInit(const char *ipStr, const int port) {
	RtpSession *session;
	char *ssrc;

	g_user_ts = 0;
	ortp_init();
	ortp_scheduler_init();
	session = rtp_session_new(RTP_SESSION_SENDONLY);

	rtp_session_set_scheduling_mode(session, 1);
	rtp_session_set_blocking_mode(session, 1);

	rtp_session_set_remote_addr(session, ipStr, port);
	rtp_session_set_payload_type(session, 0);

	ssrc = getenv("SSRC");
	if (ssrc != NULL) {
	
		printf("using SSRC=%i.\n", atoi(ssrc));
		rtp_session_set_ssrc(session,atoi(ssrc));
	}

	return session;
}

int rtpSend(RtpSession *session, const char *buffer, int len) {

	int curOffset = 0;
	int sendBytes = 0;
	int clockslide = 500;
	int sendCount = 0; 
	ortp_message("send data len %i\n", len);

	while (curOffset < len) {
		if (len <= BYTES_PER_COUNT) {
			sendBytes = len;
		} else {
		
			if (curOffset + BYTES_PER_COUNT <= len) {
			
				sendBytes = BYTES_PER_COUNT;
			} else {
			
				sendBytes = len - curOffset;
			}
		}
		ortp_message("send data bytes %i\n ", sendBytes);
		rtp_session_send_with_ts(session, (char *)(buffer + curOffset), sendBytes, g_user_ts);

		sendCount++;
		curOffset += sendBytes;
		g_user_ts += TIME_STAMP_INC;

		if (sendCount % 10 == 0) {
		
			usleep(20000);
		}
	}
	return 0;
}

int rtpExit(RtpSession *session) {

	g_user_ts = 0;
	rtp_session_destroy(session);
	ortp_exit();
	ortp_global_stats_display();
	return 0;
}

int main() {
	/*
	// int size = sizeof(RtpSession);
	size_t size = sizeof(RtpSession);
	printf("size = %d", size);
	return 0;
	*/

	char *pBuffer = "123445356234134234532523654323413453425236244123425234";

	RtpSession *pRtpSession = NULL;

	pRtpSession = rtpInit("192.201.0.51", 8888);
	if (pRtpSession == NULL) {

		printf("error rtpInit");
		return 0;
	} 

	while (1) {
	
		if (rtpSend(pRtpSession, pBuffer, 20) != 0) {
		
			printf("error rptINit");
			break;
		}
		usleep(1000);
		printf("sleep");
	}

	rtpExit(pRtpSession);
	return 0;
}