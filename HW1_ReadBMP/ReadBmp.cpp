// ReadBmp.cpp : Defines the entry point for the console application.
//

// #include "stdafx.h"
#include <fstream>
#include <iostream>

#include <string>
using namespace std;

void SaveBmp(
    const char* filename,
    unsigned char* u8ID,
    unsigned char* u8FileSize,
    unsigned char* u8Reserved,
    unsigned char* u8DataOffset,
    unsigned char* u8InfoHdSize,
    unsigned char* u8Width,
    unsigned char* u8Height,
    unsigned char* u8Planes,
    unsigned char* u8BitPerPix,
    unsigned char* u8Compress,
    unsigned char* u8DataSize,
    unsigned char* u8HRes,
    unsigned char* u8VRes,
    unsigned char* u8NumColor,
    unsigned char* u8IColor,
    unsigned char* u8Data,
    int nDataSize
) {
    ofstream fout(filename, ios::binary);
    if (!fout) {
        cout << "Cannot open output file: " << filename << endl;
        return;
    }

    // BMP File Header (14 bytes)
    fout.write((char*)u8ID,        2);
    fout.write((char*)u8FileSize,  4);
    fout.write((char*)u8Reserved,  4);
    fout.write((char*)u8DataOffset,4);

    // DIB Header (BITMAPINFOHEADER, 40 bytes)
    fout.write((char*)u8InfoHdSize,4);
    fout.write((char*)u8Width,     4);
    fout.write((char*)u8Height,    4);
    fout.write((char*)u8Planes,    2);
    fout.write((char*)u8BitPerPix, 2);
    fout.write((char*)u8Compress,  4);
    fout.write((char*)u8DataSize,  4);
    fout.write((char*)u8HRes,      4);
    fout.write((char*)u8VRes,      4);
    fout.write((char*)u8NumColor,  4);
    fout.write((char*)u8IColor,    4);

    // Pixel data
    fout.write((char*)u8Data, nDataSize);

    fout.close();
}

int main()
{
	
	unsigned char u8ID[2];
	unsigned char u8FileSize[4];
	unsigned char u8Reserved[4];
	unsigned char u8DataOffset[4];
	int nFileSize, nOffset;

	unsigned char u8InfoHdSize[4];
	unsigned char u8Width[4];
	unsigned char u8Height[4];
	unsigned char u8Planes[2];
	unsigned char u8BitPerPix[2];
	unsigned char u8Compress[4];
	unsigned char u8DataSize[4];
	unsigned char u8HRes[4];
	unsigned char u8VRes[4];
	unsigned char u8NumColor[4];
	unsigned char u8IColor[4];
	int nInfoSize, nWidth, nHeight, nPlane, nBitPerPix, nCompress, nDataSize, nHRes, nVRes, nNumColor, nIColor;


	unsigned char *u8BgrData = NULL;
	unsigned char *u8OM_b = NULL;
	unsigned char *u8OM_g = NULL;
	unsigned char *u8OM_r = NULL;
	unsigned char *u8OM_vertical = NULL;
	unsigned char *u8OM_horizontal = NULL;

	int **IM = NULL;
	int r, g, b;
	int nPx, nPy;

	string str1, str2;

	// Open .bmp file
	ifstream fp;
	fp.open("lena.bmp", ios::binary);
	
	fp.read((char*)u8ID, sizeof(u8ID));
	fp.read((char*)u8FileSize, sizeof(u8FileSize));
	fp.read((char*)u8Reserved, sizeof(u8Reserved));
	fp.read((char*)u8DataOffset, sizeof(u8DataOffset));

	fp.read((char*)u8InfoHdSize, sizeof(u8InfoHdSize));
	fp.read((char*)u8Width, sizeof(u8Width));
	fp.read((char*)u8Height, sizeof(u8Height));
	fp.read((char*)u8Planes, sizeof(u8Planes));
	fp.read((char*)u8BitPerPix, sizeof(u8BitPerPix));
	fp.read((char*)u8Compress, sizeof(u8Compress));
	fp.read((char*)u8DataSize, sizeof(u8DataSize));
	fp.read((char*)u8HRes, sizeof(u8HRes));
	fp.read((char*)u8VRes, sizeof(u8VRes));
	fp.read((char*)u8NumColor, sizeof(u8NumColor));
	fp.read((char*)u8IColor, sizeof(u8IColor));

	
	
	
	nFileSize = (int)u8FileSize[3] << 24 | (int)u8FileSize[2] << 16 | (int)u8FileSize[1] << 8 | (int)u8FileSize[0];
	nOffset = (int)u8DataOffset[3] << 24 | (int)u8DataOffset[2] << 16 | (int)u8DataOffset[1] << 8 | (int)u8DataOffset[0];

	nInfoSize = (int)u8InfoHdSize[3] << 24 | (int)u8InfoHdSize[2] << 16 | (int)u8InfoHdSize[1] << 8 | (int)u8InfoHdSize[0];
	nWidth = (int)u8Width[3] << 24 | (int)u8Width[2] << 16 | (int)u8Width[1] << 8 | (int)u8Width[0];
	nHeight = (int)u8Height[3] << 24 | (int)u8Height[2] << 16 | (int)u8Height[1] << 8 | (int)u8Height[0];
	nPlane = (int)u8Planes[1] << 8 | (int)u8Planes[0];
	nBitPerPix = (int)u8BitPerPix[1] << 8 | (int)u8BitPerPix[0];
	nCompress = (int)u8Compress[3] << 24 | (int)u8Compress[2] << 16 | (int)u8Compress[1] << 8 | (int)u8Compress[0];
	nDataSize = (int)u8DataSize[3] << 24 | (int)u8DataSize[2] << 16 | (int)u8DataSize[1] << 8 | (int)u8DataSize[0];
	nHRes = (int)u8HRes[3] << 24 | (int)u8HRes[2] << 16 | (int)u8HRes[1] << 8 | (int)u8HRes[0];
	nVRes = (int)u8VRes[3] << 24 | (int)u8VRes[2] << 16 | (int)u8VRes[1] << 8 | (int)u8VRes[0];
	nNumColor = (int)u8NumColor[3] << 24 | (int)u8NumColor[2] << 16 | (int)u8NumColor[1] << 8 | (int)u8NumColor[0];
	nIColor = (int)u8IColor[3] << 24 | (int)u8IColor[2] << 16 | (int)u8IColor[1] << 8 | (int)u8IColor[0];

	
	//************* Q1: Calculate padding byte, nPadBytes = ? *************//
	int nPadBytes = (4 - (nWidth*3)%4)%4;
	//npadbyte = (width * 3) % 4 == 0 ? 0 : 4 - (width * 3) % 4;

	u8BgrData = new unsigned char[nDataSize];
	fp.read((char*)u8BgrData, nDataSize);

	IM = new int *[nHeight];

	//************* Q2: What's different? *************//
	IM[0] = new int [nHeight * nWidth];
	for (int i = 1; i < nHeight; i++)
		IM[i] = IM[i - 1] + nWidth;
	
	
	// for (int i = 0; i < nHeight; i++)
	// 	IM[i] = new int[nWidth];
	
	//-------------------RGB2Gray-------------------//
	for (int y = 0; y < nHeight; y ++)
	{
		for (int x = 0; x < nWidth; x++)
		{
			b = u8BgrData[y * nWidth * 3 + x * 3];
			g = u8BgrData[y * nWidth * 3 + x * 3 + 1];
			r = u8BgrData[y * nWidth * 3 + x * 3 + 2];
			IM[nHeight - 1 - y][x] = (b + g + r) / 3;
		}
	}




	cout << "ID: "  << u8ID[0] << " " << u8ID[1] << endl;
	cout << "File size: " << nFileSize << endl;
	cout << "Reserved: " << (int)u8Reserved[0] << " " << (int)u8Reserved[1] << " " << (int)u8Reserved[2] << " " << (int)u8Reserved[3] << endl;
	cout << "Offset: " << nOffset << endl << endl;

	cout << "Info Size: " << nInfoSize << endl;
	cout << "Width: " << nWidth << endl;
	cout << "Height: " << nHeight << endl;
	cout << "Plane: " << nPlane << endl;
	cout << "Bits per pixel: " << nBitPerPix << endl;
	cout << "Compress: " << nCompress << endl;
	cout << "Data size: " << nDataSize << endl;
	cout << "H Resolution: " << nHRes << endl;
	cout << "V Resolution: " << nVRes << endl;
	cout << "Used Colors: " << nNumColor << endl;
	cout << "Important Colors: " << nIColor << endl;
	
	fp.close();

	// Query intensity
	/*while (true)
	{
		cout << "Type x y to query intensity of image(x , y), or type q to exit:";
		cin >> str1;
		str2 = "q";
		if (str1 == str2)
		{
			cout << "Exit...\n";
			break;
		}
		cin >> str2;

		nPx = stoi(str1);
		nPy = stoi(str2);

		cout << "Intensity of image(x, y): " << IM[nPy][nPx] << endl;
			
	}*/

	//------------------- Process output -------------------//
	u8OM_b = new unsigned char[nDataSize];
	u8OM_g = new unsigned char[nDataSize];
	u8OM_r = new unsigned char[nDataSize];
	u8OM_vertical = new unsigned char[nDataSize];
	u8OM_horizontal = new unsigned char[nDataSize];
	int cnt = 0;

	for (int y = 0; y < nHeight; y++)
	{
		for (int x = 0; x < nWidth; x++)
		{
			u8OM_b[cnt] = u8BgrData[y * nWidth * 3 + x * 3];
			u8OM_b[cnt+1] = 0;
			u8OM_b[cnt+2] = 0;

			u8OM_g[cnt+1] = u8BgrData[y * nWidth * 3 + x * 3 + 1];
			u8OM_g[cnt] = 0;
			u8OM_g[cnt+2] = 0;

			u8OM_r[cnt+2] = u8BgrData[y * nWidth * 3 + x * 3 + 2];
			u8OM_r[cnt] = 0;
			u8OM_r[cnt+1] = 0;

			u8OM_vertical[cnt] = u8BgrData[(nHeight-y-1) * nWidth * 3 + x * 3]; // b
			u8OM_vertical[cnt+1] = u8BgrData[(nHeight-y-1) * nWidth * 3 + x * 3 + 1]; // g
			u8OM_vertical[cnt+2] = u8BgrData[(nHeight-y-1) * nWidth * 3 + x * 3 + 2]; // r

			u8OM_horizontal[cnt] = u8BgrData[y * nWidth * 3 + (nWidth-x-1) * 3]; // b
			u8OM_horizontal[cnt+1] = u8BgrData[y * nWidth * 3 + (nWidth-x-1) * 3 + 1]; // g
			u8OM_horizontal[cnt+2] = u8BgrData[y * nWidth * 3 + (nWidth-x-1) * 3 + 2]; // r

			cnt += 3;
		}
		
	}

	//-------------------Save array to bmp-------------------//
	// HW1: Save the R, G, and B channels separately. 
	SaveBmp("lena_B.bmp",
		u8ID, u8FileSize, u8Reserved, u8DataOffset,
		u8InfoHdSize, u8Width, u8Height, u8Planes, u8BitPerPix,
		u8Compress, u8DataSize, u8HRes, u8VRes, u8NumColor, u8IColor,
		u8OM_b, nDataSize);

	SaveBmp("lena_G.bmp",
		u8ID, u8FileSize, u8Reserved, u8DataOffset,
		u8InfoHdSize, u8Width, u8Height, u8Planes, u8BitPerPix,
		u8Compress, u8DataSize, u8HRes, u8VRes, u8NumColor, u8IColor,
		u8OM_g, nDataSize);

	SaveBmp("lena_R.bmp",
		u8ID, u8FileSize, u8Reserved, u8DataOffset,
		u8InfoHdSize, u8Width, u8Height, u8Planes, u8BitPerPix,
		u8Compress, u8DataSize, u8HRes, u8VRes, u8NumColor, u8IColor,
		u8OM_r, nDataSize);
	
	// HW2: Flip lena.bmp vertically and horizontally. 
	SaveBmp("lena_vertical.bmp",
		u8ID, u8FileSize, u8Reserved, u8DataOffset,
		u8InfoHdSize, u8Width, u8Height, u8Planes, u8BitPerPix,
		u8Compress, u8DataSize, u8HRes, u8VRes, u8NumColor, u8IColor,
		u8OM_vertical, nDataSize);

	SaveBmp("lena_horizontal.bmp",
		u8ID, u8FileSize, u8Reserved, u8DataOffset,
		u8InfoHdSize, u8Width, u8Height, u8Planes, u8BitPerPix,
		u8Compress, u8DataSize, u8HRes, u8VRes, u8NumColor, u8IColor,
		u8OM_horizontal, nDataSize);


	// Free memory
	if (u8BgrData != NULL)
	{
		delete[] u8BgrData;
		u8BgrData = NULL;
	}

	if (u8OM_b != NULL)
	{
		delete[] u8OM_b;
		u8OM_b = NULL;
	}
	if (u8OM_g != NULL)
	{
		delete[] u8OM_g;
		u8OM_g = NULL;
	}
	if (u8OM_r != NULL)
	{
		delete[] u8OM_r;
		u8OM_r = NULL;
	}
	if (u8OM_vertical != NULL)
	{
		delete[] u8OM_vertical;
		u8OM_vertical = NULL;
	}
	if (u8OM_horizontal != NULL)
	{
		delete[] u8OM_horizontal;
		u8OM_horizontal = NULL;
	}
	

	if (IM != NULL)
	{
		if (IM[0] != NULL)
		{
			delete[] IM[0];
			IM[0] = NULL;
		}

		delete[] IM;
	}

	//system("pause");

	return 0;
}

