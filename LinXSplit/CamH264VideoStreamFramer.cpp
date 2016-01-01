#include "CamH264VideoStreamFramer.h"
#include ".\live555\GroupsockHelper.hh" // gettimeofday

// CamH264VideoStreamFramer *********************************************************
#include "H264EndWrapper.h"		//H264 Encode

extern "C" 
{
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavdevice/avdevice.h"
#include "libswscale/swscale.h"
#include "libavutil/avutil.h"
}
int VIDEO_WIDTH, VIDEO_HEIGHT;

AVFormatContext	*pFormatCtx;
AVCodecContext	*pCodecCtx;
SwsContext *img_convert_ctx;
AVPacket *packet;
AVFrame	*pFrame, *pFrameYUV;

int InitFFMpeg(void)
{
	int				i, videoindex;
	AVCodec			*pCodec;
	AVInputFormat *ifmt;

	__try{
		av_register_all();
		avformat_network_init();
		pFormatCtx = avformat_alloc_context();

		//Register Device
		avdevice_register_all();
		ifmt = av_find_input_format("gdigrab");
		if (avformat_open_input(&pFormatCtx, "desktop", ifmt, NULL) != 0){
			printf("Couldn't open input stream.（无法打开输入流）\n");
			return -1;
		}

		if (avformat_find_stream_info(pFormatCtx, NULL) < 0)
		{
			printf("Couldn't find stream information.（无法获取流信息）\n");
			return -1;
		}
		videoindex = -1;
		for (i = 0; i < pFormatCtx->nb_streams; i++)
		if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			videoindex = i;
			break;
		}
		if (videoindex == -1)
		{
			printf("Didn't find a video stream.（没有找到视频流）\n");
			return -1;
		}
		pCodecCtx = pFormatCtx->streams[videoindex]->codec;
		pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
		if (pCodec == NULL)
		{
			printf("Codec not found.（没有找到解码器）\n");
			return -1;
		}
		if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0)
		{
			printf("Could not open codec.（无法打开解码器）\n");
			return -1;
		}

		//如果宽度不是32的倍数，将会降低sws_scale的速度，故我们保证宽度为32的倍数。
		VIDEO_WIDTH = pCodecCtx->width / 32 * 32;
		//保证高度等比例缩放，再保证为16的倍数。
		VIDEO_HEIGHT = pCodecCtx->height / 16 * 16;

		pFrame = av_frame_alloc();
		pFrameYUV = av_frame_alloc();
		uint8_t *out_buffer = (uint8_t *)av_malloc(avpicture_get_size(PIX_FMT_YUV420P, VIDEO_WIDTH, VIDEO_HEIGHT));
		avpicture_fill((AVPicture *)pFrameYUV, out_buffer, PIX_FMT_YUV420P, VIDEO_WIDTH, VIDEO_HEIGHT);

		packet = (AVPacket *)av_malloc(sizeof(AVPacket));
		//Output Information-----------------------------
		printf("File Information（文件信息）---------------------\n");
		av_dump_format(pFormatCtx, 0, NULL, 0);
		printf("-------------------------------------------------\n");

		img_convert_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt, VIDEO_WIDTH, VIDEO_HEIGHT, PIX_FMT_YUV420P, SWS_POINT, NULL, NULL, NULL);
}
__except (1) {
	fprintf(stderr, "Init FFMpeg failed\n");
	return -1;
}
	return 0;
}

void UnInitFFMpeg(void)
{
	if (pFormatCtx != NULL)
		avformat_close_input(&pFormatCtx);

	av_free(img_convert_ctx);
	av_free_packet(packet);
	av_frame_free(&pFrame);
	av_frame_free(&pFrameYUV);
}



CamH264VideoStreamFramer::CamH264VideoStreamFramer(UsageEnvironment& env,
	FramedSource* inputSource, H264EncWrapper* pH264Enc) :
	H264VideoStreamFramer(env, inputSource, False, False),
	m_pNalArray(NULL), m_iCurNalNum(0), m_iCurNal(0), m_iCurFrame(0),
	m_pH264Enc(pH264Enc)
{
	fFrameRate = 20.0; // We assume a frame rate of 25 fps, unless we learn otherwise (from parsing a Sequence Parameter Set NAL unit)

}

CamH264VideoStreamFramer::~CamH264VideoStreamFramer()
{
	UnInitFFMpeg();

	m_pH264Enc->Destroy();
	//delete m_pH264Enc;
	m_pH264Enc = NULL;
}



CamH264VideoStreamFramer* CamH264VideoStreamFramer::createNew(
	UsageEnvironment& env,
	FramedSource* inputSource)
{
	int n = InitFFMpeg();
	if (n < 0)
	{
		env.setResultMsg("Initialize FFMpeg error");
		return NULL;
	}

	// 初始化H264 Encode
	H264EncWrapper* pH264Enc = new H264EncWrapper;
	if (pH264Enc->Initialize(VIDEO_WIDTH, VIDEO_HEIGHT, 96 * 10, 25) < 0)
	{
		env.setResultMsg("Initialize x264 encoder error");
		return NULL;
	}

	CamH264VideoStreamFramer* fr;
	fr = new CamH264VideoStreamFramer(env, inputSource, pH264Enc);
	return fr;
}

Boolean CamH264VideoStreamFramer::currentNALUnitEndsAccessUnit()
{
	if (m_iCurNal >= m_iCurNalNum)
	{
		m_iCurFrame++;
		return True;
	}
	else
	{
		return False;
	}
}

// live555每一次调用doGetNextFrame()时，先查看是否还有上一个视频帧的NAL Unit没有
// 发送完成，如果没有，继续发送，如果没有，先从摄像头获取一个YUV的数据帧，x264
// 编码YUV成一个NAL Unit数组，再开始发送NAL数组，一次发送一个NAL Unit
void CamH264VideoStreamFramer::doGetNextFrame()
{
	TNAL* pNal = NULL;
	unsigned char* pOrgImg;
	// 查找是否还有未发送完的NAL Unit
	if ((m_pNalArray != NULL) && (m_iCurNal < m_iCurNalNum))
	{
		pNal = &m_pNalArray[m_iCurNal];
	}
	else
	{
		// 清空NAL Unit数组
		m_pH264Enc->CleanNAL(m_pNalArray, m_iCurNalNum);
		m_iCurNal = 0;

		// 获取最新视频帧,YUV
		int ret,got_picture = 0;
		ret = av_read_frame(pFormatCtx, packet);
		ret = avcodec_decode_video2(pCodecCtx, pFrame, &got_picture, packet);
		if (got_picture){
			sws_scale(img_convert_ctx, (const uint8_t* const*)pFrame->data, pFrame->linesize, 0, pCodecCtx->height, pFrameYUV->data, pFrameYUV->linesize);
		}
		pOrgImg = pFrameYUV->data[0];
		gettimeofday(&fPresentationTime, NULL);

		// H264 Encode
		m_pH264Enc->Encode(pOrgImg, m_pNalArray, m_iCurNalNum);
		pNal = &m_pNalArray[m_iCurNal];
		//printf(INF, "Frame[%d], Nal[%d:%d]: size = %d", m_iCurFrame, m_iCurNalNum, m_iCurNal, pNal->size);
		av_free_packet(packet);
	}
	m_iCurNal++;

	unsigned char* realData = pNal->data;
	unsigned int realLen = pNal->size;

	if (realLen < fMaxSize)
	{
		memcpy(fTo, realData, realLen);
	}
	else
	{
		memcpy(fTo, realData, fMaxSize);
		fNumTruncatedBytes = realLen - fMaxSize;
	}

	fDurationInMicroseconds = 200000;

	fFrameSize = realLen;
	afterGetting(this);
}