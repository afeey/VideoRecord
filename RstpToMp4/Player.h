#ifdef __cplusplus
extern "C" {
#endif

#include <libavcodec/avcodec.h>
#include <libavdevice/avdevice.h>
#include <libavformat/avformat.h>
#include <libavfilter/avfilter.h>
#include <libavutil/avutil.h>
#include <libswscale/swscale.h>
#include <SDL.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#ifdef __cplusplus
}
#endif

#define SFM_BREAK_EVENT  (SDL_USEREVENT + 1)

class Player
{
private:
	AVCodecContext	*pCodecCtx;
	AVCodec			*pCodec;

	AVFormatContext	*pInFormatCtx;
    AVStream		*pInStream;

	AVFormatContext *pOutFormatCtx;
	AVStream		*pOutStream;

	char*			url;		//������Ƶ·��
	int				videoIndex; //��Ƶ������
	bool			record;		//¼����Ƶ��־
	char*			filename;	//�����ļ���
	bool			stop;		//ֹͣ��־
	int				frameCount; //֡����

public:
	Player(char* url){
		pCodecCtx = NULL;
		pCodec = NULL;
		pInFormatCtx = NULL;
		pInStream = NULL;
		pOutFormatCtx = NULL;
		pOutStream = NULL;

		videoIndex = -1;
		record = false;
		this-> url = url;
		filename = "";
		stop = false;
		frameCount = 0;
	}

	// ����¼��
	void SetRecord(bool record);

	// ����¼���ļ���
	void SetFileName(char* filename);

	// ����
	int Play();

	// ¼��
	int Record();

	// ��ʼ
	int Start();

	// ֹͣ
	void Stop();

	~Player(){
	}
};