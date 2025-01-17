#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "config.h"
#include "common.h"
#include "avframe.h"

#include "include/AVAPIs.h"
#include "include/IOTCAPIs.h"
#include "avframeinfo.h"

int FRAME_INFO_SIZE = 64;
int VIDEO_BUF_SIZE = 400000;

#define AUDIO_BUF_SIZE 1024

extern void CArrayToGoArray();

AVFrame lastFrame = {
    .timestamp = 0,
    .millisecond = 0
};

void *thread_ReceiveVideo(void *arg)
{
	DPRINTF("[P2P][ReceiveVideo] Write to fifo pipe: %d\n", MODE_RTSP_FIFO);
	DPRINTF("[P2P][ReceiveVideo] Running...\n");

	setvbuf(stdout, NULL, _IONBF, 0);

	int avIndex = *(int *)arg;

	for (;;)
	{
		//usleep((1000/30) * 1000);

		char videoBuffer[VIDEO_BUF_SIZE];
		char frameInfo[FRAME_INFO_SIZE];
		int outBufSize = 0;
		int outFrmSize = 0;
		int outFrmInfoSize = 0;
		unsigned int frmNo;

		int ret = avRecvFrameData2(avIndex, videoBuffer, VIDEO_BUF_SIZE, &outBufSize, &outFrmSize, frameInfo, FRAME_INFO_SIZE, &outFrmInfoSize, &frmNo);
		//DPRINTF("Video Receive thread loop avIndex=%d, ret=%d\n", avIndex, ret);

		if(ret == AV_ER_DATA_NOREADY)
		{
			//DPRINTF("AV_ER_DATA_NOREADY[%d]\n", avIndex);
			usleep(100);
			continue;
		}
		else if(ret == AV_ER_LOSED_THIS_FRAME)
		{
			DPRINTF("[P2P][ReceiveVideo] Lost video frame NO[%d]\n", frmNo);
			continue;
		}
		else if(ret == AV_ER_SESSION_CLOSE_BY_REMOTE)
		{
			DPRINTF("[P2P][ReceiveVideo] AV_ER_SESSION_CLOSE_BY_REMOTE\n");
			break;
		}
		else if(ret == AV_ER_REMOTE_TIMEOUT_DISCONNECT)
		{
			DPRINTF("[P2P][ReceiveVideo] AV_ER_REMOTE_TIMEOUT_DISCONNECT\n");
			break;
		}
		else if(ret == IOTC_ER_INVALID_SID)
		{
			DPRINTF("[P2P][ReceiveVideo] Session cant be used anymore\n");
			break;
		}
		else if(ret == AV_ER_INCOMPLETE_FRAME)
		{
			DPRINTF("[P2P][ReceiveVideo] Incomplete video frame NO[%d] ReadSize[%d] FrmSize[%d] FrmInfoSize[%u]\n", frmNo, outBufSize, outFrmSize, outFrmInfoSize);
		}
		else if (ret < 0)		{			printf("[P2P][ReceiveVideo] Unknown error code=%d\n", ret);			break;		}		else if (ret > 0)
		{
			AVFrame avFrame = readAvFrame(frameInfo, videoBuffer, &ret);

			if (avFrame.codec_id == 76) {

				write(MODE_RTSP_FIFO, videoBuffer, ret);
			}
		}
	}
	DPRINTF("[P2P][ReceiveVideo] Thread exit\n");
	return 0;
}

void *thread_ReceiveAudio(void *arg)
{

	int avIndex = *(int *)arg;
	printf("[P2P][ReceiveAudio] Write to fifo pipe: %d\n", MODE_RTSP_FIFO2);
	printf("[P2P][ReceiveAudio] Running...\n");

	char buf[AUDIO_BUF_SIZE];
	FRAMEINFO_t frameInfo;
	unsigned int frmNo;
	int ret;
	while (1)
	{
		/*
		ret = avCheckAudioBuf(avIndex);
		if(ret < 0) break;
		if (ret < 10) // determined by audio frame rate
		{
			sleep(100);
			continue;
		}
	*/
		ret = avRecvAudioData(avIndex, buf, AUDIO_BUF_SIZE, (char *)&frameInfo, sizeof(FRAMEINFO_t), &frmNo);
		if(ret == AV_ER_DATA_NOREADY)
		{
			//DPRINTF("AV_ER_DATA_NOREADY[%d]\n", avIndex);
			usleep(100);
			continue;
		}
		if (ret == AV_ER_SESSION_CLOSE_BY_REMOTE)
		{
			printf("[P2P][ReceiveAudio] AV_ER_SESSION_CLOSE_BY_REMOTE\n");
			break;
		}
		else if (ret == AV_ER_REMOTE_TIMEOUT_DISCONNECT)
		{
			printf("[P2P][ReceiveAudio] AV_ER_REMOTE_TIMEOUT_DISCONNECT\n");
			break;
		}
		else if (ret == IOTC_ER_INVALID_SID)
		{
			printf("[P2P][ReceiveAudio] Session cant be used anymore\n");
			break;
		}
		else if (ret == AV_ER_LOSED_THIS_FRAME)
		{
			printf("[P2P][ReceiveAudio] AV_ER_LOSED_THIS_FRAME[%d]\n", frmNo);
			continue;
		}
		else if (ret < 0)
		{
			printf("[P2P][ReceiveAudio] Unknown error code=%d\n", ret);			break;
		}
		else
		{
			if (frameInfo.codec_id == 140)
			{
				//printf("FRAME[%d] ", ret);
				write(MODE_RTSP_FIFO2, buf, ret);
			}
			else
			{
				//write(MODE_RTSP_FIFO2, buf, ret);
				//printf("UNK[%d] ", ret);
			}
		}

	}
	DPRINTF("[P2P][ReceiveAudio] Thread exit\n");
	return 0;
}

int startReceive(int *avIndex) {
	pthread_t ThreadReceiveVideo_ID = 0, ThreadReceiveAudio_ID = 0;

	int ret;
	if ((ret=pthread_create(&ThreadReceiveVideo_ID, NULL, &thread_ReceiveVideo, avIndex)))
	{
		DPRINTF("[P2P] Create Video Receive thread failed\n");
		return 1;
	}

	if(RUN_AUDIO) { 
		if ((ret=pthread_create(&ThreadReceiveAudio_ID, NULL, &thread_ReceiveAudio, avIndex)))
		{
			DPRINTF("[P2P] Create Audio Receive thread failed\n");
			return 1;
		}
	}

	if(ThreadReceiveVideo_ID!=0) pthread_join(ThreadReceiveVideo_ID, NULL);
	
	if(RUN_AUDIO) { 
		if(ThreadReceiveAudio_ID!=0) pthread_join(ThreadReceiveAudio_ID, NULL);
	}

	return 0;
}