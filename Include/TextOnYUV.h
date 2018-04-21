
#ifndef TEXTONYUV_H_

#define TEXTONYUV_H_

#include <map>
using namespace std;
#include <pthread.h>

class CTextOnYuv
{
public:
	CTextOnYuv();
	virtual ~CTextOnYuv();


	
	int DrawText( char *szText, 
					int iFont, 
					int nStartX,
					int nStartY,
					int nImageWidth, 
					int nImageHeight, 
					unsigned char *pYUV );
    //获取szText中的字符、汉字的字符点阵数据储存于pCodeList，pHalfList指示每个点阵是否是汉字(0-汉字)，nCount指示pCodeList/pHalfList的个数
    // nCount输入/输出参数，输入时只是前面连个数组的最大个数，输出时指示个数
    int GetCodeList(char* szText, int iFont, unsigned char** pCodeList, int* pHalfList, int &nCount);
    // 获取字库中的点阵,szText为1个汉字或者一个字符, pdz存储点阵数组大小为iFont*iFont,如果为字符则为iFont*iFont/2
    int GetDz(char* szText, int iFont, unsigned char* &pdz, int &nHalf);
private:

	void GetWordCount( char *szText, 
								  int *&pnCodes, 
								  int &nCount, 
								  int &nHalfFont );

	
	int  GetDZK( int nCode, 
						int iFont, 
						unsigned char *&pZk, 
						int &nHalf );


	int GetDzkInMem( int nCode, int nFont, unsigned char *&pZk );


	
	void AddDzkToMem( int nCode, int nFont, unsigned char *pZk );

	map<int/*code*/, unsigned char*/*szCode*/> m_mapDzk;

	pthread_mutex_t m_lock;
};

#endif 

