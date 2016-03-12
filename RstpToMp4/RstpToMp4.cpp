// RstpToMp4.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <windows.h>
#include <process.h> 

#ifdef __cplusplus 
extern "C" { 
#endif

#include <libavcodec/avcodec.h> 
#include <libavdevice/avdevice.h> 
#include <libavformat/avformat.h> 
#include <libavfilter/avfilter.h> 
#include <libavutil/avutil.h> 
#include <libswscale/swscale.h>

#include <stdlib.h> 
#include <stdio.h> 
#include <string.h> 
#include <math.h>

#ifdef __cplusplus 
} 
#endif

static AVFormatContext *i_fmt_ctx;
static AVStream *i_video_stream;

static AVFormatContext *o_fmt_ctx;
static AVStream *o_video_stream;

static bool bStop = false;

static unsigned __stdcall rtsp2mp4(void * pThis);

int _tmainxxx(int argc, _TCHAR* argv[])
{
	//ffplayer();
    bStop = false;
    HANDLE   hth;  
    unsigned  uiThreadID;  
  
    hth = (HANDLE)_beginthreadex( NULL,         // security  
                                   0,            // stack size  
                                   rtsp2mp4,  
                                   NULL,           // arg list  
                                   0,  //CREATE_SUSPENDED so we can later call ResumeThread()  
                                   &uiThreadID );  
  
    if ( hth == 0 ) 
    {
        printf("Failed to create thread\n"); 
        return -1;
    }

    printf("按任意键停止录像\n");
    getchar();
    bStop = true;
    printf("按任意键退出\n");
    getchar();
       
    return 0;
}

static unsigned __stdcall rtsp2mp4(void * pThis)
{
    avcodec_register_all();
    av_register_all();
    avformat_network_init();

    /* should set to NULL so that avformat_open_input() allocate a new one */
    i_fmt_ctx = NULL;
    char rtspUrl[] = "rtsp://218.204.223.237:554/live/1/66251FC11353191F/e7ooqwcfbqjoo80j.sdp";
    const char *filename = "C:\\1.mp4";
    if (avformat_open_input(&i_fmt_ctx, rtspUrl, NULL, NULL)!=0)
    {
        fprintf(stderr, "could not open input file\n");
        return -1;
    }

    if (avformat_find_stream_info(i_fmt_ctx, NULL)<0)
    {
        fprintf(stderr, "could not find stream info\n");
        return -1;
    }

    //av_dump_format(i_fmt_ctx, 0, argv[1], 0);

    /* find first video stream */
    for (unsigned i=0; i<i_fmt_ctx->nb_streams; i++)
    {
        if (i_fmt_ctx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
        {
            i_video_stream = i_fmt_ctx->streams[i];
            break;
        }
    }
    if (i_video_stream == NULL)
    {
        fprintf(stderr, "didn't find any video stream\n");
        return -1;
    }

    avformat_alloc_output_context2(&o_fmt_ctx, NULL, NULL, filename);

    /*
    * since all input files are supposed to be identical (framerate, dimension, color format, ...)
    * we can safely set output codec values from first input file
    */
    o_video_stream = avformat_new_stream(o_fmt_ctx, NULL);
    {
        AVCodecContext *c;
        c = o_video_stream->codec;
        c->bit_rate = 400000;
        c->codec_id = i_video_stream->codec->codec_id;
        c->codec_type = i_video_stream->codec->codec_type;
        c->time_base.num = i_video_stream->time_base.num;
        c->time_base.den = i_video_stream->time_base.den;
        fprintf(stderr, "time_base.num = %d time_base.den = %d\n", c->time_base.num, c->time_base.den);
        c->width = i_video_stream->codec->width;
        c->height = i_video_stream->codec->height;
        c->pix_fmt = i_video_stream->codec->pix_fmt;
        printf("%d %d %d", c->width, c->height, c->pix_fmt);
        c->flags = i_video_stream->codec->flags;
        c->flags |= CODEC_FLAG_GLOBAL_HEADER;
        c->me_range = i_video_stream->codec->me_range;
        c->max_qdiff = i_video_stream->codec->max_qdiff;

        c->qmin = i_video_stream->codec->qmin;
        c->qmax = i_video_stream->codec->qmax;

        c->qcompress = i_video_stream->codec->qcompress;
    }

    avio_open(&o_fmt_ctx->pb, filename, AVIO_FLAG_WRITE);

    avformat_write_header(o_fmt_ctx, NULL);

    int last_pts = 0;
    int last_dts = 0;

    int64_t pts, dts;
    while (!bStop)
    {
        AVPacket i_pkt;
        av_init_packet(&i_pkt);
        i_pkt.size = 0;
        i_pkt.data = NULL;
        if (av_read_frame(i_fmt_ctx, &i_pkt) <0 )
            break;
        /*
        * pts and dts should increase monotonically
        * pts should be >= dts
        */
        i_pkt.flags |= AV_PKT_FLAG_KEY;
        pts = i_pkt.pts;
        i_pkt.pts += last_pts;
        dts = i_pkt.dts;
        i_pkt.dts += last_dts;
        i_pkt.stream_index = 0;

        //printf("%lld %lld\n", i_pkt.pts, i_pkt.dts);
        static int num = 1;
        printf("frame %d\n", num++);
        av_interleaved_write_frame(o_fmt_ctx, &i_pkt);
        //av_free_packet(&i_pkt);
        //av_init_packet(&i_pkt);
         Sleep(10);
    }
    last_dts += dts;
    last_pts += pts;

    avformat_close_input(&i_fmt_ctx);

    av_write_trailer(o_fmt_ctx);

    avcodec_close(o_fmt_ctx->streams[0]->codec);
    av_freep(&o_fmt_ctx->streams[0]->codec);
    av_freep(&o_fmt_ctx->streams[0]);

    avio_close(o_fmt_ctx->pb);
    av_free(o_fmt_ctx);

    return 0;
}



