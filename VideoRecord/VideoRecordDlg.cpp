
// VideoRecordDlg.cpp : ʵ���ļ�
//

#include "stdafx.h"
#include "VideoRecord.h"
#include "VideoRecordDlg.h"
#include "afxdialogex.h"
#include "resource.h"
#include "PlayerCore.h"
#include <string>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CVideoRecordDlg �Ի���



CVideoRecordDlg::CVideoRecordDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CVideoRecordDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CVideoRecordDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CVideoRecordDlg, CDialogEx)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_START, &CVideoRecordDlg::OnBnClickedStart)
	ON_BN_CLICKED(IDC_CLOSE, &CVideoRecordDlg::OnBnClickedClose)
END_MESSAGE_MAP()


// CVideoRecordDlg ��Ϣ�������

BOOL CVideoRecordDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();
	
	//CString filepath = L"rtsp://admin:@192.168.1.168:80/ch0_0.264";
	CString filepath = L"rtsp://218.204.223.237:554/live/1/66251FC11353191F/e7ooqwcfbqjoo80j.sdp";
	
	GetDlgItem(IDC_URL)->SetWindowTextW(filepath);

	// ���ô˶Ի����ͼ�ꡣ��Ӧ�ó��������ڲ��ǶԻ���ʱ����ܽ��Զ�
	//  ִ�д˲���
	SetIcon(m_hIcon, TRUE);			// ���ô�ͼ��
	SetIcon(m_hIcon, FALSE);		// ����Сͼ��

	// TODO: �ڴ���Ӷ���ĳ�ʼ������

	return TRUE;  // ���ǽ��������õ��ؼ������򷵻� TRUE
}

// �����Ի��������С����ť������Ҫ����Ĵ���
//  �����Ƹ�ͼ�ꡣ����ʹ���ĵ�/��ͼģ�͵� MFC Ӧ�ó���
//  �⽫�ɿ���Զ���ɡ�

void CVideoRecordDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // ���ڻ��Ƶ��豸������

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// ʹͼ���ڹ����������о���
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// ����ͼ��
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//���û��϶���С������ʱϵͳ���ô˺���ȡ�ù��
//��ʾ��
HCURSOR CVideoRecordDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CVideoRecordDlg::OnBnClickedStart()
{

	CString text("");
	GetDlgItem(IDC_START)->GetWindowTextW(text);

	if(text=="��ʼ"){
		GetDlgItem(IDC_START)->SetWindowTextW(L"ֹͣ");

		CString url_text("");
		GetDlgItem(IDC_URL)->GetWindowTextW(url_text);

		int len =WideCharToMultiByte(CP_ACP,0,url_text,-1,NULL,0,NULL,NULL);  
		char *url =new char[len +1];  
		WideCharToMultiByte(CP_ACP,0,url_text,-1,url,len,NULL,NULL ); 

		CTime tm = CTime::GetCurrentTime();
		CString nameStr = tm.Format("%Y%m%d%H%M%S.mp4");

		len =WideCharToMultiByte(CP_ACP,0,nameStr,-1,NULL,0,NULL,NULL);  
		char *filename =new char[len + 1];  
		WideCharToMultiByte(CP_ACP,0,nameStr,-1,filename,len,NULL,NULL );
		
		Player_Open(url,filename);
		Player_Play();

	}else{
		GetDlgItem(IDC_START)->SetWindowTextW(L"��ʼ");
	}
}

void CVideoRecordDlg::OnBnClickedClose()
{
	CDialog::OnCancel();
}
