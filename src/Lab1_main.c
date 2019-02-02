#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
#include <math.h>
#include <unistd.h>

#define VALID_ARGS "IOh"
#define APPEND_TO_SPLIT "_split.bmp"
#define APPEND_TO_DECIM "_decim.bmp"
#define HELP_MESSAGE "This is a help message for LAB1 program?\n[UPORIN_MAINONLY STATUS: ENGAGED]\n========================================\nQ:How to run this program.\nA:LAB1_BMP -I <input.bmp> [-O output.bmp]\nAuthor: Alexey Ponomarenko-Timofeev.\nBrotip: Don't write programs this way kids!"
#define MAX_FNAME_LEN 30

#pragma pack(2)
typedef struct{
  unsigned short int type;
  unsigned int size;
  unsigned short int reserved1,reserved2;
  unsigned int offset;
}BMPHEAD;

#pragma pack()
typedef struct{
  unsigned int size;
  unsigned int width,height;
  unsigned short planes;
  unsigned short int bits;
  unsigned int compressed;
  unsigned int imageSize;
  unsigned int xRes,yRes;
  unsigned int nColors;
  unsigned int importantColors;
}BMPINFOHEAD;

typedef struct{
  unsigned char pixel[3];
}BMP24PIXEL;

typedef struct{
  unsigned char pixel[3];
}YCbCr24PIXEL;

int main(int argc, char *argv[]){
  FILE *pInFile = NULL, *pOutFile = NULL;
  BMPHEAD head = {0};
  BMPINFOHEAD iHead = {0};
#ifdef DEBUG
  BMPHEAD dHead = {0};
  BMPINFOHEAD dIHead = {0};
#endif
  BMP24PIXEL *pPixelsRGB = NULL, *pPixelsRGB2 = NULL;
  YCbCr24PIXEL *pPixelsYCbCr = NULL, *pPixelsYCbCrNech = NULL, *pPixelsYCbCrArith = NULL, *pPixelsYCbCrRcvr = NULL;
  unsigned int i = 0, j = 0, k = 0, histogram[256][30] = {{0},{0},{0},{0},{0},{0},{0},{0},{0},{0},{0},{0},{0},{0},{0},{0},{0},{0},{0},{0},{0},{0},{0},{0},{0},{0},{0},{0},{0},{0}};
  unsigned char dBytes = 0, namePos = 0;
  char *pLoc = NULL, outName[MAX_FNAME_LEN] = {'\0'}, *pDPCMData = NULL;
  float M[6] = {0.0}, D[6] = {0.0}, R[15] = {0.0}, PSNR = 0.0, H[30] = {0.0};
  unsigned short temp = 0;

#ifdef DEBUG
  (void)puts("=================================================\n===================DEBUG BUILD===================\n=================================================");
#endif
            
  for(i = 0; i < argc; i++){
    switch (getopt(argc, argv, VALID_ARGS)){
      case('I'):{
	pInFile = fopen(argv[i+2], "rb");
	(namePos == 0) ? (namePos = i+2) : (namePos = namePos);
	
	if(!pInFile){
	  (void)fputs("Error: can't open BMP file for reading!\n", stderr);
	  return EXIT_FAILURE;
	}
	break;
      }
      case('O'):{
	pOutFile = fopen(argv[i+2], "wb");
	namePos = i+2;
	
	if(!pOutFile){
	  (void)fputs("Error: can't open BMP file for writing!\n", stderr);
	}
	break;
      }
      case('h'):{
	(void)puts(HELP_MESSAGE);
	
	if( pInFile ){
	  (void)fclose(pInFile);
	}else if( pOutFile ){
	  (void)fclose(pOutFile);
	}else{
	  return EXIT_SUCCESS;
	}
      }
      case('?'):{
	(void)printf("Warning: unknown argument: %s.\n", argv[i+1]);
	break;
      }
    }
  }
  
  if(!pOutFile){
    (void)strcat(&(outName[0]), argv[namePos]);
    pLoc = strstr(&(outName[0]), ".bmp");
    if(pLoc){
      *pLoc = '\0';
      *(pLoc + 1) = '\0';
      *(pLoc + 2) = '\0';
      *(pLoc + 3) = '\0';
    }
  }
  
  (void)strcat( &(outName[0]), "_split_R.bmp");
  
  pOutFile = fopen(&(outName[0]), "wb");
  
  if(!pInFile){
    (void)puts("Error: provide input filename for the program at least, or use -h!");
    return EXIT_FAILURE;
  }

  if(1 != fread(&(head), sizeof(BMPHEAD), 1, pInFile)){
    (void)puts("Error, failed to read BMP header!");
  }
  
  if(ferror(pInFile)){
    (void)fputs("Error: failure while reading header from BMP file.\n", stderr);
    (void)fclose(pInFile);
    (void)fclose(pOutFile);
    return EXIT_FAILURE;
  }
 
  if(1 != fread(&(iHead), sizeof(BMPINFOHEAD), 1, pInFile)){
    (void)puts("Error, failed to read BMP INFO header!");
  } 
  
  if(ferror(pInFile)){
    (void)fputs("Error: failure while reading info header from BMP file.\n", stderr);
    (void)fclose(pInFile);
    (void)fclose(pOutFile);
    return EXIT_FAILURE;
  }

  if(head.type != 0x4D42){
    (void)fputs("Error: given file is not a BMP file!\n", stderr);
    (void)fclose(pInFile);
    (void)fclose(pOutFile);
    return EXIT_FAILURE;
  }
  
  (void)puts("Succesfully verified headers, reading pixel data.");

  #ifdef DEBUG
  (void)puts("DEBUGGING INFO");
  (void)printf("Image height: %d.\nImage width: %d.\n", iHead.height, iHead.width);
  #endif
  
  dBytes = (4-((iHead.width*3) & 3)) & 3;
  pPixelsRGB = malloc(head.size - sizeof(BMPHEAD) - sizeof(BMPINFOHEAD));
  
  if(!pPixelsRGB){
    (void)fputs("Error: failed to allocate memory for RGB pixel data.\n", stderr);
    (void)fclose(pInFile);
    (void)fclose(pOutFile);
    return EXIT_FAILURE;
  }
  
  for(i = 0; i < iHead.height; i++){
    if(iHead.width != fread(pPixelsRGB + (i*iHead.width), sizeof(BMP24PIXEL), iHead.width, pInFile)){
      (void)puts("Error reading pixel row");
    }
    (void)fseek(pInFile, dBytes, SEEK_CUR);
  }
  
  if(ferror(pInFile)){
    (void)fputs("Error: failure while reading pixel data.\n", stderr);
    (void)fclose(pInFile);
    (void)fclose(pOutFile);
    (void)free(pPixelsRGB);
    return EXIT_FAILURE;
  }
  
  (void)puts("Succesfully read pixel data, spliting components to files.");

#ifdef DEBUG
  (void)printf("Will write to file: %s\n", outName);
#endif

    (void)fwrite(&head, sizeof(BMPHEAD), 1, pOutFile);
  (void)fwrite(&iHead, sizeof(BMPINFOHEAD), 1, pOutFile);

  pLoc = 0;
  
  for(i = 0; i < iHead.height; i++){
    for(j = 0; j < iHead.width; j++){
      (void)fwrite( &pLoc, sizeof(unsigned char), 2, pOutFile);
      (void)fwrite( &((pPixelsRGB + j + i*(iHead.width))->pixel[2]), sizeof(unsigned char), 1, pOutFile);
    }
    (void)fwrite( pPixelsRGB, sizeof(unsigned char), dBytes, pOutFile);
  }

  (void)fclose(pOutFile);
  
  pLoc = strstr(outName, "R.bmp");
  
  *pLoc = '\0';
  *(pLoc + 1) = '\0';
  *(pLoc + 2) = '\0';
  *(pLoc + 3) = '\0';
  *(pLoc + 4) = '\0';
  
  (void)strcat( &(outName[0]), "G.bmp");

#ifdef DEBUG
  (void)printf("Will write to file: %s\n", outName);
#endif

  pOutFile = fopen(outName, "wb");
  
  (void)fwrite(&head, sizeof(BMPHEAD), 1, pOutFile);
  (void)fwrite(&iHead, sizeof(BMPINFOHEAD), 1, pOutFile);
  
  pLoc = 0;

  for(i = 0; i < iHead.height; i++){
    for(j = 0; j < iHead.width; j++){
      (void)fwrite( &pLoc, sizeof(unsigned char), 1, pOutFile);
      (void)fwrite( &((pPixelsRGB + j + i*(iHead.width))->pixel[1]), sizeof(unsigned char), 1, pOutFile);
      (void)fwrite( &pLoc, sizeof(unsigned char), 1, pOutFile);

    }
    (void)fwrite( pPixelsRGB, sizeof(unsigned char), dBytes, pOutFile);
  }
  
  (void)fclose(pOutFile);
  
  pLoc = strstr(outName, "G.bmp");
  
  *pLoc = '\0';
  *(pLoc + 1) = '\0';
  *(pLoc + 2) = '\0';
  *(pLoc + 3) = '\0';
  *(pLoc + 4) = '\0';
  
  (void)strcat( &(outName[0]), "B.bmp");
  
  pOutFile = fopen(outName, "wb");
  
#ifdef DEBUG
  (void)printf("Will write to file: %s\n", outName);
#endif
  
  (void)fwrite(&head, sizeof(BMPHEAD), 1, pOutFile);
  (void)fwrite(&iHead, sizeof(BMPINFOHEAD), 1, pOutFile);
  
  pLoc = 0;

  for(i = 0; i < iHead.height; i++){
    for(j = 0; j < iHead.width; j++){
      (void)fwrite( &((pPixelsRGB + j + i*(iHead.width))->pixel[0]), sizeof(unsigned char), 1, pOutFile);
      (void)fwrite( &pLoc, sizeof(unsigned char), 2, pOutFile);
    }
    (void)fwrite( pPixelsRGB, sizeof(unsigned char), dBytes, pOutFile);
  }

  (void)puts("Succesfully split three conponents, calculating correlation, writing results to correl.dat.");
  
  (void)fclose(pOutFile);
  
  pLoc = 0;
  
  for(i = 0; i < 3; i++){
    for(j = 0; j < iHead.height*iHead.width; j++){
      M[i] += (pPixelsRGB + j)->pixel[i];
    }
    M[i] /= (iHead.height*iHead.width);
  }
  
  for(i = 0; i < 3; i++){
    for(j = 0; j < (iHead.height - 1)*(iHead.width - 1); j++){
      D[i] += ((pPixelsRGB + j)->pixel[i] - M[i])*((pPixelsRGB + j)->pixel[i] - M[i]);
    }
    D[i] = sqrt(D[i] / (iHead.height*iHead.width - 1));
  }
  
  (void)printf("\033[31m--==##RED##==--\033[37m\nM(R) = %f\nD(R) = %f\n\033[32m--==##GREEN##==--\033[37m\nM(G) = %f\nD(G) = %f\n\033[34m--==##BLUE##==--\033[37m\nM(B) = %f\nD(B) = %f\n", M[2], D[2], M[1], D[1], M[0], D[0]);
  
  for(i = 0; i < iHead.width*iHead.height; i++){
    R[0] += ((pPixelsRGB + i)->pixel[2] - M[2])*((pPixelsRGB + i)->pixel[1] - M[1]);
  }
  R[0] /= (iHead.width * iHead.height * D[1] * D[2]);
  
  (void)printf("Correlation for\033[31m red\033[37m and\033[32m green\033[37m component: %f\n", R[0]);
  R[0] = 0;
  
  for(i = 0; i < iHead.width*iHead.height; i++){
    R[0] += ((pPixelsRGB + i)->pixel[2] - M[2])*((pPixelsRGB + i)->pixel[0] - M[0]);
  }
  R[0] /= (iHead.width * iHead.height * D[0] * D[2]);
  
  (void)printf("Correlation for\033[31m red\033[37m and\033[34m blue\033[37m component: %f\n", R[0]);
  R[0] = 0;
  
  for(i = 0; i < iHead.width*iHead.height; i++){
    R[0] += ((pPixelsRGB + i)->pixel[1] - M[1])*((pPixelsRGB + i)->pixel[0] - M[0]);
  }
  R[0] /= (iHead.width * iHead.height * D[0] * D[1]);
  
  (void)printf("Correlation for\033[32m green\033[37m and\033[34m blue\033[37m component: %f\n", R[0]);
  
#ifndef HIDDENDAT
  pOutFile = fopen("correl_RGB_0_offset.dat", "w");
#else
  pOutFile = fopen(".correl_RGB_0_offset.dat", "w");
#endif

  for(j = 1; j < iHead.width >> 2; j++){
    M[0] = M[1] = M[2] = M[3] = M[4] = M[5] = D[0] = D[1] = D[2] = D[3] = D[4] = D[5] = R[0] = R[1] = R[2] = 0;
    for(i = 0; i < iHead.height; i++){
      for(k = 0; k < j; k++){
	M[0] += (pPixelsRGB + k + i*(iHead.width))->pixel[0];
	M[1] += (pPixelsRGB + k + i*(iHead.width))->pixel[1];
	M[2] += (pPixelsRGB + k + i*(iHead.width))->pixel[2];
	M[3] += (pPixelsRGB - (j - k) + (i + 1)*(iHead.width))->pixel[0];
	M[4] += (pPixelsRGB - (j - k) + (i + 1)*(iHead.width))->pixel[1];
	M[5] += (pPixelsRGB - (j - k) + (i + 1)*(iHead.width))->pixel[2];
      }
    }
    M[0] /= iHead.height*j;
    M[1] /= iHead.height*j;
    M[2] /= iHead.height*j;
    M[3] /= iHead.height*j;
    M[4] /= iHead.height*j;
    M[5] /= iHead.height*j;
    for(i = 0; i < iHead.height; i++){
      for(k = 0; k < j; k++){
	D[0] += ((pPixelsRGB + k + i*(iHead.width))->pixel[0] - M[0])*((pPixelsRGB + k + i*(iHead.width))->pixel[0] - M[0]);
	D[1] += ((pPixelsRGB + k + i*(iHead.width))->pixel[1] - M[1])*((pPixelsRGB + k + i*(iHead.width))->pixel[1] - M[1]);
	D[2] += ((pPixelsRGB + k + i*(iHead.width))->pixel[2] - M[2])*((pPixelsRGB + k + i*(iHead.width))->pixel[2] - M[2]);
	D[3] += ((pPixelsRGB - (j - k) + (i + 1)*(iHead.width))->pixel[0] - M[3])*((pPixelsRGB - (j - k) + (i + 1)*(iHead.width))->pixel[0] - M[3]);
	D[4] += ((pPixelsRGB - (j - k) + (i + 1)*(iHead.width))->pixel[1] - M[4])*((pPixelsRGB - (j - k) + (i + 1)*(iHead.width))->pixel[1] - M[4]);
	D[5] += ((pPixelsRGB - (j - k) + (i + 1)*(iHead.width))->pixel[2] - M[5])*((pPixelsRGB - (j - k) + (i + 1)*(iHead.width))->pixel[2] - M[5]);
      }
    }

    D[0] = sqrt(D[0] / (iHead.height*j - 1));
    D[1] = sqrt(D[1] / (iHead.height*j - 1));
    D[2] = sqrt(D[2] / (iHead.height*j - 1));
    D[3] = sqrt(D[3] / (iHead.height*j - 1));
    D[4] = sqrt(D[4] / (iHead.height*j - 1));
    D[5] = sqrt(D[5] / (iHead.height*j - 1));

    for(i = 0; i < iHead.height; i++){
      for(k = 0; k < j; k++){
	R[0] += ((pPixelsRGB + k + i*(iHead.width))->pixel[0] - M[0])*((pPixelsRGB + (i + 1)*(iHead.width) - (j - k))->pixel[0] - M[3]);
	R[1] += ((pPixelsRGB + k + i*(iHead.width))->pixel[1] - M[1])*((pPixelsRGB + (i + 1)*(iHead.width) - (j - k))->pixel[1] - M[4]);
	R[2] += ((pPixelsRGB + k + i*(iHead.width))->pixel[2] - M[2])*((pPixelsRGB + (i + 1)*(iHead.width) - (j - k))->pixel[2] - M[5]);
      }
    }
    R[0] /= (iHead.height*k*D[0]*D[3]);
    R[1] /= (iHead.height*k*D[1]*D[4]);
    R[2] /= (iHead.height*k*D[2]*D[5]);
    (void)fprintf(pOutFile ,"%d\t%f\t%f\t%f\n", j, R[0], R[1], R[2]);
  }
  
  (void)fclose(pOutFile);
  
#ifndef HIDDENDAT
  pOutFile = fopen("correl_RGB_-5_offset.dat", "w");
#else
  pOutFile = fopen(".correl_RGB_-5_offset.dat", "w");
#endif

  for(j = 1; j < iHead.width >> 2; j++){
    M[0] = M[1] = M[2] = M[3] = M[4] = M[5] = D[0] = D[1] = D[2] = D[3] = D[4] = D[5] = R[0] = R[1] = R[2] = 0;
    for(i = 0; i < iHead.height - 5; i++){
      for(k = 0; k < j; k++){
	M[0] += (pPixelsRGB + k + i*(iHead.width))->pixel[0];
	M[1] += (pPixelsRGB + k + i*(iHead.width))->pixel[1];
	M[2] += (pPixelsRGB + k + i*(iHead.width))->pixel[2];
	M[3] += (pPixelsRGB - (j - k) + (i + 1 + 5)*(iHead.width))->pixel[0];
	M[4] += (pPixelsRGB - (j - k) + (i + 1 + 5)*(iHead.width))->pixel[1];
	M[5] += (pPixelsRGB - (j - k) + (i + 1 + 5)*(iHead.width))->pixel[2];
      }
    }
    M[0] /= iHead.height*j;
    M[1] /= iHead.height*j;
    M[2] /= iHead.height*j;
    M[3] /= iHead.height*j;
    M[4] /= iHead.height*j;
    M[5] /= iHead.height*j;
    for(i = 0; i < iHead.height - 5; i++){
      for(k = 0; k < j; k++){
	D[0] += ((pPixelsRGB + k + i*(iHead.width))->pixel[0] - M[0])*((pPixelsRGB + k + i*(iHead.width))->pixel[0] - M[0]);
	D[1] += ((pPixelsRGB + k + i*(iHead.width))->pixel[1] - M[1])*((pPixelsRGB + k + i*(iHead.width))->pixel[1] - M[1]);
	D[2] += ((pPixelsRGB + k + i*(iHead.width))->pixel[2] - M[2])*((pPixelsRGB + k + i*(iHead.width))->pixel[2] - M[2]);
	D[3] += ((pPixelsRGB - (j - k) + (i + 1 + 5)*(iHead.width))->pixel[0] - M[3])*((pPixelsRGB - (j - k) + (i + 1 + 5)*(iHead.width))->pixel[0] - M[3]);
	D[4] += ((pPixelsRGB - (j - k) + (i + 1 + 5)*(iHead.width))->pixel[1] - M[4])*((pPixelsRGB - (j - k) + (i + 1 + 5)*(iHead.width))->pixel[1] - M[4]);
	D[5] += ((pPixelsRGB - (j - k) + (i + 1 + 5)*(iHead.width))->pixel[2] - M[5])*((pPixelsRGB - (j - k) + (i + 1 + 5)*(iHead.width))->pixel[2] - M[5]);
      }
    }

    D[0] = sqrt(D[0] / (iHead.height*j - 1));
    D[1] = sqrt(D[1] / (iHead.height*j - 1));
    D[2] = sqrt(D[2] / (iHead.height*j - 1));
    D[3] = sqrt(D[3] / (iHead.height*j - 1));
    D[4] = sqrt(D[4] / (iHead.height*j - 1));
    D[5] = sqrt(D[5] / (iHead.height*j - 1));

    for(i = 0; i < iHead.height - 5; i++){
      for(k = 0; k < j; k++){
	R[0] += ((pPixelsRGB + k + i*(iHead.width))->pixel[0] - M[0])*((pPixelsRGB + (i + 1 + 5)*(iHead.width) - (j - k))->pixel[0] - M[3]);
	R[1] += ((pPixelsRGB + k + i*(iHead.width))->pixel[1] - M[1])*((pPixelsRGB + (i + 1 + 5)*(iHead.width) - (j - k))->pixel[1] - M[4]);
	R[2] += ((pPixelsRGB + k + i*(iHead.width))->pixel[2] - M[2])*((pPixelsRGB + (i + 1 + 5)*(iHead.width) - (j - k))->pixel[2] - M[5]);
      }
    }
    R[0] /= (iHead.height*k*D[0]*D[3]);
    R[1] /= (iHead.height*k*D[1]*D[4]);
    R[2] /= (iHead.height*k*D[2]*D[5]);
    (void)fprintf(pOutFile ,"%d\t%f\t%f\t%f\n", j, R[0], R[1], R[2]);
  }

  (void)fclose(pOutFile);

#ifndef HIDDENDAT
  pOutFile = fopen("correl_RGB_-10_offset.dat", "w");
#else
    pOutFile = fopen(".correl_RGB_-10_offset.dat", "w");
#endif

    for(j = 1; j < iHead.width >> 2; j++){
    M[0] = M[1] = M[2] = M[3] = M[4] = M[5] = D[0] = D[1] = D[2] = D[3] = D[4] = D[5] = R[0] = R[1] = R[2] = 0;
    for(i = 0; i < iHead.height - 10; i++){
      for(k = 0; k < j; k++){
	M[0] += (pPixelsRGB + k + i*(iHead.width))->pixel[0];
	M[1] += (pPixelsRGB + k + i*(iHead.width))->pixel[1];
	M[2] += (pPixelsRGB + k + i*(iHead.width))->pixel[2];
	M[3] += (pPixelsRGB - (j - k) + (i + 11)*(iHead.width))->pixel[0];
	M[4] += (pPixelsRGB - (j - k) + (i + 11)*(iHead.width))->pixel[1];
	M[5] += (pPixelsRGB - (j - k) + (i + 11)*(iHead.width))->pixel[2];
      }
    }
    M[0] /= iHead.height*j;
    M[1] /= iHead.height*j;
    M[2] /= iHead.height*j;
    M[3] /= iHead.height*j;
    M[4] /= iHead.height*j;
    M[5] /= iHead.height*j;
    for(i = 0; i < iHead.height - 10; i++){
      for(k = 0; k < j; k++){
	D[0] += ((pPixelsRGB + k + i*(iHead.width))->pixel[0] - M[0])*((pPixelsRGB + k + i*(iHead.width))->pixel[0] - M[0]);
	D[1] += ((pPixelsRGB + k + i*(iHead.width))->pixel[1] - M[1])*((pPixelsRGB + k + i*(iHead.width))->pixel[1] - M[1]);
	D[2] += ((pPixelsRGB + k + i*(iHead.width))->pixel[2] - M[2])*((pPixelsRGB + k + i*(iHead.width))->pixel[2] - M[2]);
	D[3] += ((pPixelsRGB - (j - k) + (i + 11)*(iHead.width))->pixel[0] - M[3])*((pPixelsRGB - (j - k) + (i + 11)*(iHead.width))->pixel[0] - M[3]);
	D[4] += ((pPixelsRGB - (j - k) + (i + 11)*(iHead.width))->pixel[1] - M[4])*((pPixelsRGB - (j - k) + (i + 11)*(iHead.width))->pixel[1] - M[4]);
	D[5] += ((pPixelsRGB - (j - k) + (i + 11)*(iHead.width))->pixel[2] - M[5])*((pPixelsRGB - (j - k) + (i + 11)*(iHead.width))->pixel[2] - M[5]);
      }
    }

    D[0] = sqrt(D[0] / (iHead.height*j - 1));
    D[1] = sqrt(D[1] / (iHead.height*j - 1));
    D[2] = sqrt(D[2] / (iHead.height*j - 1));
    D[3] = sqrt(D[3] / (iHead.height*j - 1));
    D[4] = sqrt(D[4] / (iHead.height*j - 1));
    D[5] = sqrt(D[5] / (iHead.height*j - 1));

    for(i = 0; i < iHead.height - 10; i++){
      for(k = 0; k < j; k++){
	R[0] += ((pPixelsRGB + k + i*(iHead.width))->pixel[0] - M[0])*((pPixelsRGB + (i + 11)*(iHead.width) - (j - k))->pixel[0] - M[3]);
	R[1] += ((pPixelsRGB + k + i*(iHead.width))->pixel[1] - M[1])*((pPixelsRGB + (i + 11)*(iHead.width) - (j - k))->pixel[1] - M[4]);
	R[2] += ((pPixelsRGB + k + i*(iHead.width))->pixel[2] - M[2])*((pPixelsRGB + (i + 11)*(iHead.width) - (j - k))->pixel[2] - M[5]);
      }
    }
    R[0] /= (iHead.height*k*D[0]*D[3]);
    R[1] /= (iHead.height*k*D[1]*D[4]);
    R[2] /= (iHead.height*k*D[2]*D[5]);
    (void)fprintf(pOutFile ,"%d\t%f\t%f\t%f\n", j, R[0], R[1], R[2]);
  }
  
  (void)fclose(pOutFile);

#ifndef HIDDENDAT
  pOutFile = fopen("correl_RGB_5_offset.dat", "w");
#else
  pOutFile = fopen(".correl_RGB_5_offset.dat", "w");
#endif
  
  for(j = 1; j < iHead.width >> 2; j++){
    M[0] = M[1] = M[2] = M[3] = M[4] = M[5] = D[0] = D[1] = D[2] = D[3] = D[4] = D[5] = R[0] = R[1] = R[2] = 0;
    for(i = 0; i < iHead.height - 5; i++){
      for(k = 0; k < j; k++){
	M[0] += (pPixelsRGB + k + (i + 5)*(iHead.width))->pixel[0];
	M[1] += (pPixelsRGB + k + (i + 5)*(iHead.width))->pixel[1];
	M[2] += (pPixelsRGB + k + (i + 5)*(iHead.width))->pixel[2];
	M[3] += (pPixelsRGB - (j - k) + (i + 1)*(iHead.width))->pixel[0];
	M[4] += (pPixelsRGB - (j - k) + (i + 1)*(iHead.width))->pixel[1];
	M[5] += (pPixelsRGB - (j - k) + (i + 1)*(iHead.width))->pixel[2];
      }
    }
    M[0] /= iHead.height*j;
    M[1] /= iHead.height*j;
    M[2] /= iHead.height*j;
    M[3] /= iHead.height*j;
    M[4] /= iHead.height*j;
    M[5] /= iHead.height*j;
    for(i = 0; i < iHead.height - 5; i++){
      for(k = 0; k < j; k++){
	D[0] += ((pPixelsRGB + k + (i + 5)*(iHead.width))->pixel[0] - M[0])*((pPixelsRGB + k + (i + 5)*(iHead.width))->pixel[0] - M[0]);
	D[1] += ((pPixelsRGB + k + (i + 5)*(iHead.width))->pixel[1] - M[1])*((pPixelsRGB + k + (i + 5)*(iHead.width))->pixel[1] - M[1]);
	D[2] += ((pPixelsRGB + k + (i + 5)*(iHead.width))->pixel[2] - M[2])*((pPixelsRGB + k + (i + 5)*(iHead.width))->pixel[2] - M[2]);
	D[3] += ((pPixelsRGB - (j - k) + (i + 1)*(iHead.width))->pixel[0] - M[3])*((pPixelsRGB - (j - k) + (i + 1)*(iHead.width))->pixel[0] - M[3]);
	D[4] += ((pPixelsRGB - (j - k) + (i + 1)*(iHead.width))->pixel[1] - M[4])*((pPixelsRGB - (j - k) + (i + 1)*(iHead.width))->pixel[1] - M[4]);
	D[5] += ((pPixelsRGB - (j - k) + (i + 1)*(iHead.width))->pixel[2] - M[5])*((pPixelsRGB - (j - k) + (i + 1)*(iHead.width))->pixel[2] - M[5]);
      }
    }

    D[0] = sqrt(D[0] / (iHead.height*j - 1));
    D[1] = sqrt(D[1] / (iHead.height*j - 1));
    D[2] = sqrt(D[2] / (iHead.height*j - 1));
    D[3] = sqrt(D[3] / (iHead.height*j - 1));
    D[4] = sqrt(D[4] / (iHead.height*j - 1));
    D[5] = sqrt(D[5] / (iHead.height*j - 1));

    for(i = 0; i < iHead.height - 5; i++){
      for(k = 0; k < j; k++){
	R[0] += ((pPixelsRGB + k + (i + 5)*(iHead.width))->pixel[0] - M[0])*((pPixelsRGB + (i + 1)*(iHead.width) - (j - k))->pixel[0] - M[3]);
	R[1] += ((pPixelsRGB + k + (i + 5)*(iHead.width))->pixel[1] - M[1])*((pPixelsRGB + (i + 1)*(iHead.width) - (j - k))->pixel[1] - M[4]);
	R[2] += ((pPixelsRGB + k + (i + 5)*(iHead.width))->pixel[2] - M[2])*((pPixelsRGB + (i + 1)*(iHead.width) - (j - k))->pixel[2] - M[5]);
      }
    }
    R[0] /= (iHead.height*k*D[0]*D[3]);
    R[1] /= (iHead.height*k*D[1]*D[4]);
    R[2] /= (iHead.height*k*D[2]*D[5]);
    (void)fprintf(pOutFile ,"%d\t%f\t%f\t%f\n", j, R[0], R[1], R[2]);
  }
  
  (void)fclose(pOutFile);
  
#ifndef HIDDENDAT
  pOutFile = fopen("correl_RGB_10_offset.dat", "w");
#else
  pOutFile = fopen(".correl_RGB_10_offset.dat", "w");
#endif

  for(j = 1; j < iHead.width >> 2; j++){
    M[0] = M[1] = M[2] = M[3] = M[4] = M[5] = D[0] = D[1] = D[2] = D[3] = D[4] = D[5] = R[0] = R[1] = R[2] = 0;
    for(i = 0; i < iHead.height - 10; i++){
      for(k = 0; k < j; k++){
	M[0] += (pPixelsRGB + k + (i + 10)*(iHead.width))->pixel[0];
	M[1] += (pPixelsRGB + k + (i + 10)*(iHead.width))->pixel[1];
	M[2] += (pPixelsRGB + k + (i + 10)*(iHead.width))->pixel[2];
	M[3] += (pPixelsRGB - (j - k) + (i + 1)*(iHead.width))->pixel[0];
	M[4] += (pPixelsRGB - (j - k) + (i + 1)*(iHead.width))->pixel[1];
	M[5] += (pPixelsRGB - (j - k) + (i + 1)*(iHead.width))->pixel[2];
      }
    }
    M[0] /= iHead.height*j;
    M[1] /= iHead.height*j;
    M[2] /= iHead.height*j;
    M[3] /= iHead.height*j;
    M[4] /= iHead.height*j;
    M[5] /= iHead.height*j;
    for(i = 0; i < iHead.height - 10; i++){
      for(k = 0; k < j; k++){
	D[0] += ((pPixelsRGB + k + (i + 10)*(iHead.width))->pixel[0] - M[0])*((pPixelsRGB + k + (i + 10)*(iHead.width))->pixel[0] - M[0]);
	D[1] += ((pPixelsRGB + k + (i + 10)*(iHead.width))->pixel[1] - M[1])*((pPixelsRGB + k + (i + 10)*(iHead.width))->pixel[1] - M[1]);
	D[2] += ((pPixelsRGB + k + (i + 10)*(iHead.width))->pixel[2] - M[2])*((pPixelsRGB + k + (i + 10)*(iHead.width))->pixel[2] - M[2]);
	D[3] += ((pPixelsRGB - (j - k) + (i + 1)*(iHead.width))->pixel[0] - M[3])*((pPixelsRGB - (j - k) + (i + 1)*(iHead.width))->pixel[0] - M[3]);
	D[4] += ((pPixelsRGB - (j - k) + (i + 1)*(iHead.width))->pixel[1] - M[4])*((pPixelsRGB - (j - k) + (i + 1)*(iHead.width))->pixel[1] - M[4]);
	D[5] += ((pPixelsRGB - (j - k) + (i + 1)*(iHead.width))->pixel[2] - M[5])*((pPixelsRGB - (j - k) + (i + 1)*(iHead.width))->pixel[2] - M[5]);
      }
    }

    D[0] = sqrt(D[0] / (iHead.height*j - 1));
    D[1] = sqrt(D[1] / (iHead.height*j - 1));
    D[2] = sqrt(D[2] / (iHead.height*j - 1));
    D[3] = sqrt(D[3] / (iHead.height*j - 1));
    D[4] = sqrt(D[4] / (iHead.height*j - 1));
    D[5] = sqrt(D[5] / (iHead.height*j - 1));

    for(i = 0; i < iHead.height - 10; i++){
      for(k = 0; k < j; k++){
	R[0] += ((pPixelsRGB + k + (i + 10)*(iHead.width))->pixel[0] - M[0])*((pPixelsRGB + (i + 1)*(iHead.width) - (j - k))->pixel[0] - M[3]);
	R[1] += ((pPixelsRGB + k + (i + 10)*(iHead.width))->pixel[1] - M[1])*((pPixelsRGB + (i + 1)*(iHead.width) - (j - k))->pixel[1] - M[4]);
	R[2] += ((pPixelsRGB + k + (i + 10)*(iHead.width))->pixel[2] - M[2])*((pPixelsRGB + (i + 1)*(iHead.width) - (j - k))->pixel[2] - M[5]);
      }
    }
    R[0] /= (iHead.height*k*D[0]*D[3]);
    R[1] /= (iHead.height*k*D[1]*D[4]);
    R[2] /= (iHead.height*k*D[2]*D[5]);
    (void)fprintf(pOutFile ,"%d\t%f\t%f\t%f\n", j, R[0], R[1], R[2]);
  }
  
  (void)puts("Succesfully calculated correlation, written to files with prefix correl_, converting to YCbCr.");
  (void)fclose(pOutFile);
  
  pPixelsYCbCr = malloc(sizeof(YCbCr24PIXEL)*iHead.width*iHead.height);
  
  if(!pPixelsYCbCr){
    (void)fputs("Error: failed to allocate memory for YCbCr pixel data.\n", stderr);
    (void)free(pPixelsRGB);
    (void)fclose(pInFile);
    return EXIT_FAILURE;
  }
  
  (void)memset( pPixelsYCbCr, 0, sizeof(YCbCr24PIXEL)*iHead.width*iHead.height);
  
  for(i = 0;i < iHead.width*iHead.height; i++){
    temp = round(0.299 * (double)(pPixelsRGB + i)->pixel[2] + 0.587 * (double)(pPixelsRGB + i)->pixel[1] + 0.114 * (double)(pPixelsRGB + i)->pixel[0]);
    if(temp > 255){
      temp = 255;
    }else if(temp < 0){
      temp = 0;
    }
    (pPixelsYCbCr + i)->pixel[0] = temp;
    temp = round(0.5643 * ((double)(pPixelsRGB + i)->pixel[0] - (double)(pPixelsYCbCr + i)->pixel[0]) + 128.0);
    if(temp > 255){
      temp = 255;
    }else if(temp < 0){
      temp = 0;
    }
    (pPixelsYCbCr + i)->pixel[1] = temp;
    temp = round(0.7132 * ((double)(pPixelsRGB + i)->pixel[2] - (double)(pPixelsYCbCr + i)->pixel[0]) + 128.0);
    if(temp > 255){
      temp = 255;
    }else if(temp < 0){
      temp = 0;
    }
    (pPixelsYCbCr + i)->pixel[2] = temp;
  }
  (void)memset(&outName, 0, sizeof(char)*MAX_FNAME_LEN);
  
  (void)strcat(outName, argv[namePos]);
  
  pLoc = strstr(outName, ".bmp");
  
  if(pLoc){
    *pLoc = '\0';
    *(pLoc + 1) = '\0';
    *(pLoc + 2) = '\0';
    *(pLoc + 3) = '\0';
  }
  
  (void)strcat(outName, "_split_Y.bmp");
  
#ifdef DEBUG
  (void)printf("Will write to file: %s\n", outName);
#endif

  pOutFile = fopen(outName, "wb");
  
  if(!pOutFile){
    (void)fputs("Error: can't open file for writing Y component.\n", stderr);
    (void)free(pPixelsRGB);
    (void)free(pPixelsYCbCr);
    (void)fclose(pInFile);
    return EXIT_FAILURE;
  }
  
  (void)fwrite(&head, sizeof(BMPHEAD), 1, pOutFile);
  (void)fwrite(&iHead, sizeof(BMPINFOHEAD), 1, pOutFile);
  
  for(i = 0; i < iHead.height; i++){
    for(j = 0; j < iHead.width; j++){
      (void)fwrite(&((pPixelsYCbCr + j + i*iHead.width)->pixel[0]), sizeof(unsigned char), 1, pOutFile);
      (void)fwrite(&((pPixelsYCbCr + j + i*iHead.width)->pixel[0]), sizeof(unsigned char), 1, pOutFile);
      (void)fwrite(&((pPixelsYCbCr + j + i*iHead.width)->pixel[0]), sizeof(unsigned char), 1, pOutFile);
    }
    (void)fwrite(pPixelsYCbCr, sizeof(unsigned char), dBytes, pOutFile);
  }
  
  (void)fclose(pOutFile);
  
  pLoc = strstr(outName, "Y.bmp");
  
  if(pLoc){
    *pLoc = '\0';
    *(pLoc + 1) = '\0';
    *(pLoc + 2) = '\0';
    *(pLoc + 3) = '\0';
    *(pLoc + 4) = '\0';
  }
  
  (void)strcat(outName, "U.bmp");

#ifdef DEBUG
  (void)printf("Will write to file: %s\n", outName);
#endif

  pOutFile = fopen(outName, "wb");
  
  if(!pOutFile){
    (void)fputs("Error: can't open file for writing U component.\n", stderr);
    (void)free(pPixelsRGB);
    (void)free(pPixelsYCbCr);
    (void)fclose(pInFile);
    return EXIT_FAILURE;
  }
  
  (void)fwrite(&head, sizeof(BMPHEAD), 1, pOutFile);
  (void)fwrite(&iHead, sizeof(BMPINFOHEAD), 1, pOutFile);
  
  for(i = 0; i < iHead.height; i++){
    for(j = 0; j < iHead.width; j++){
      (void)fwrite(&((pPixelsYCbCr + j + i*iHead.width)->pixel[1]), sizeof(unsigned char), 1, pOutFile);
      (void)fwrite(&((pPixelsYCbCr + j + i*iHead.width)->pixel[1]), sizeof(unsigned char), 1, pOutFile);
      (void)fwrite(&((pPixelsYCbCr + j + i*iHead.width)->pixel[1]), sizeof(unsigned char), 1, pOutFile);
    }
    (void)fwrite(pPixelsYCbCr, sizeof(unsigned char), dBytes, pOutFile);
  }
  
  (void)fclose(pOutFile);
  
  pLoc = strstr(outName, "U.bmp");
  
  if(pLoc){
    *pLoc = '\0';
    *(pLoc + 1) = '\0';
    *(pLoc + 2) = '\0';
    *(pLoc + 3) = '\0';
    *(pLoc + 4) = '\0';
  }
  
  (void)strcat(outName, "V.bmp");
  
#ifdef DEBUG
  (void)printf("Will write to file: %s\n", outName);
#endif

  pOutFile = fopen(outName, "wb");
  
  if(!pOutFile){
    (void)fputs("Error: can't open file for writing V component.\n", stderr);
    (void)free(pPixelsRGB);
    (void)free(pPixelsYCbCr);
    (void)fclose(pInFile);
    return EXIT_FAILURE;
  }
  
  (void)fwrite(&head, sizeof(BMPHEAD), 1, pOutFile);
  (void)fwrite(&iHead, sizeof(BMPINFOHEAD), 1, pOutFile);
  
  for(i = 0; i < iHead.height; i++){
    for(j = 0; j < iHead.width; j++){
      (void)fwrite(&((pPixelsYCbCr + j + i*iHead.width)->pixel[2]), sizeof(unsigned char), 1, pOutFile);
      (void)fwrite(&((pPixelsYCbCr + j + i*iHead.width)->pixel[2]), sizeof(unsigned char), 1, pOutFile);
      (void)fwrite(&((pPixelsYCbCr + j + i*iHead.width)->pixel[2]), sizeof(unsigned char), 1, pOutFile);
    }
    (void)fwrite(pPixelsYCbCr, sizeof(unsigned char), dBytes, pOutFile);
  }
  
  (void)puts("Succesfully written YCbCr components, calculating correlation.");
  (void)fclose(pOutFile);

#ifndef HIDDENDAT
  pOutFile = fopen("correl_YCbCr_0_offset.dat", "w");
#else
  pOutFile = fopen(".correl_YCbCr_0_offset.dat", "w");
#endif

  for(i = 0; i < 3; i++){
    for(j = 0; j < iHead.height*iHead.width; j++){
      M[i] += (pPixelsYCbCr + j)->pixel[i];
    }
    M[i] /= (iHead.height*iHead.width);
  }
  
  for(i = 0; i < 3; i++){
    for(j = 0; j < (iHead.height - 1)*(iHead.width - 1); j++){
      D[i] += ((pPixelsYCbCr + j)->pixel[i] - M[i])*((pPixelsYCbCr + j)->pixel[i] - M[i]);
    }
    D[i] = sqrt(D[i] / (iHead.height*iHead.width - 1));
  }
  
  (void)printf("--==##Y##==--\nM(R) = %f\nD(R) = %f\n--==##U##==--\nM(G) = %f\nD(G) = %f\n--==##V##==--\nM(B) = %f\nD(B) = %f\n", M[2], D[2], M[1], D[1], M[0], D[0]);    
  
  for(i = 0; i < iHead.width*iHead.height; i++){
    R[0] += ((pPixelsYCbCr + i)->pixel[2] - M[2])*((pPixelsYCbCr + i)->pixel[1] - M[1]);
  }
  R[0] /= (iHead.width * iHead.height * D[1] * D[2]);
  
  (void)printf("Correlation for Y and U component: %f\n", R[0]);
  R[0] = 0;
  
  for(i = 0; i < iHead.width*iHead.height; i++){
    R[0] += ((pPixelsYCbCr + i)->pixel[2] - M[2])*((pPixelsYCbCr + i)->pixel[0] - M[0]);
  }
  R[0] /= (iHead.width * iHead.height * D[0] * D[2]);
  
  (void)printf("Correlation for Y and V component: %f\n", R[0]);
  R[0] = 0;
  
  for(i = 0; i < iHead.width*iHead.height; i++){
    R[0] += ((pPixelsYCbCr + i)->pixel[1] - M[1])*((pPixelsYCbCr + i)->pixel[0] - M[0]);
  }
  R[0] /= (iHead.width * iHead.height * D[0] * D[1]);
  
  (void)printf("Correlation for U and V component: %f\n", R[0]);
  
  for(j = 1; j < iHead.width >> 2; j++){
    M[0] = M[1] = M[2] = M[3] = M[4] = M[5] = D[0] = D[1] = D[2] = D[3] = D[4] = D[5] = R[0] = R[1] = R[2] = 0;
    for(i = 0; i < iHead.height; i++){
      for(k = 0; k < j; k++){
	M[0] += (pPixelsYCbCr + k + i*(iHead.width))->pixel[0];
	M[1] += (pPixelsYCbCr + k + i*(iHead.width))->pixel[1];
	M[2] += (pPixelsYCbCr + k + i*(iHead.width))->pixel[2];
	M[3] += (pPixelsYCbCr - (j - k) + (i + 1)*(iHead.width))->pixel[0];
	M[4] += (pPixelsYCbCr - (j - k) + (i + 1)*(iHead.width))->pixel[1];
	M[5] += (pPixelsYCbCr - (j - k) + (i + 1)*(iHead.width))->pixel[2];
      }
    }
    M[0] /= iHead.height*j;
    M[1] /= iHead.height*j;
    M[2] /= iHead.height*j;
    M[3] /= iHead.height*j;
    M[4] /= iHead.height*j;
    M[5] /= iHead.height*j;
    for(i = 0; i < iHead.height; i++){
      for(k = 0; k < j; k++){
	D[0] += ((pPixelsYCbCr + k + i*(iHead.width))->pixel[0] - M[0])*((pPixelsYCbCr + k + i*(iHead.width))->pixel[0] - M[0]);
	D[1] += ((pPixelsYCbCr + k + i*(iHead.width))->pixel[1] - M[1])*((pPixelsYCbCr + k + i*(iHead.width))->pixel[1] - M[1]);
	D[2] += ((pPixelsYCbCr + k + i*(iHead.width))->pixel[2] - M[2])*((pPixelsYCbCr + k + i*(iHead.width))->pixel[2] - M[2]);
	D[3] += ((pPixelsYCbCr - (j - k) + (i + 1)*(iHead.width))->pixel[0] - M[3])*((pPixelsYCbCr - (j - k) + (i + 1)*(iHead.width))->pixel[0] - M[3]);
	D[4] += ((pPixelsYCbCr - (j - k) + (i + 1)*(iHead.width))->pixel[1] - M[4])*((pPixelsYCbCr - (j - k) + (i + 1)*(iHead.width))->pixel[1] - M[4]);
	D[5] += ((pPixelsYCbCr - (j - k) + (i + 1)*(iHead.width))->pixel[2] - M[5])*((pPixelsYCbCr - (j - k) + (i + 1)*(iHead.width))->pixel[2] - M[5]);
      }
    }

    D[0] = sqrt(D[0] / (iHead.height*j - 1));
    D[1] = sqrt(D[1] / (iHead.height*j - 1));
    D[2] = sqrt(D[2] / (iHead.height*j - 1));
    D[3] = sqrt(D[3] / (iHead.height*j - 1));
    D[4] = sqrt(D[4] / (iHead.height*j - 1));
    D[5] = sqrt(D[5] / (iHead.height*j - 1));

    for(i = 0; i < iHead.height; i++){
      for(k = 0; k < j; k++){
	R[0] += ((pPixelsYCbCr + k + i*(iHead.width))->pixel[0] - M[0])*((pPixelsYCbCr + (i + 1)*(iHead.width) - (j - k))->pixel[0] - M[3]);
	R[1] += ((pPixelsYCbCr + k + i*(iHead.width))->pixel[1] - M[1])*((pPixelsYCbCr + (i + 1)*(iHead.width) - (j - k))->pixel[1] - M[4]);
	R[2] += ((pPixelsYCbCr + k + i*(iHead.width))->pixel[2] - M[2])*((pPixelsYCbCr + (i + 1)*(iHead.width) - (j - k))->pixel[2] - M[5]);
      }
    }
    R[0] /= (iHead.height*k*D[0]*D[3]);
    R[1] /= (iHead.height*k*D[1]*D[4]);
    R[2] /= (iHead.height*k*D[2]*D[5]);
    (void)fprintf(pOutFile ,"%d\t%f\t%f\t%f\n", j, R[0], R[1], R[2]);
  }

  (void)fclose(pOutFile);

#ifndef HIDDENDAT
  pOutFile = fopen("correl_YCbCr_-5_offset.dat", "w");
#else
  pOutFile = fopen(".correl_YCbCr_-5_offset.dat", "w");
#endif

  for(j = 1; j < iHead.width >> 2; j++){
    M[0] = M[1] = M[2] = M[3] = M[4] = M[5] = D[0] = D[1] = D[2] = D[3] = D[4] = D[5] = R[0] = R[1] = R[2] = 0;
    for(i = 0; i < iHead.height - 5; i++){
      for(k = 0; k < j; k++){
	M[0] += (pPixelsYCbCr + k + i*(iHead.width))->pixel[0];
	M[1] += (pPixelsYCbCr + k + i*(iHead.width))->pixel[1];
	M[2] += (pPixelsYCbCr + k + i*(iHead.width))->pixel[2];
	M[3] += (pPixelsYCbCr - (j - k) + (i + 1 + 5)*(iHead.width))->pixel[0];
	M[4] += (pPixelsYCbCr - (j - k) + (i + 1 + 5)*(iHead.width))->pixel[1];
	M[5] += (pPixelsYCbCr - (j - k) + (i + 1 + 5)*(iHead.width))->pixel[2];
      }
    }
    M[0] /= iHead.height*j;
    M[1] /= iHead.height*j;
    M[2] /= iHead.height*j;
    M[3] /= iHead.height*j;
    M[4] /= iHead.height*j;
    M[5] /= iHead.height*j;
    for(i = 0; i < iHead.height - 5; i++){
      for(k = 0; k < j; k++){
	D[0] += ((pPixelsYCbCr + k + i*(iHead.width))->pixel[0] - M[0])*((pPixelsYCbCr + k + i*(iHead.width))->pixel[0] - M[0]);
	D[1] += ((pPixelsYCbCr + k + i*(iHead.width))->pixel[1] - M[1])*((pPixelsYCbCr + k + i*(iHead.width))->pixel[1] - M[1]);
	D[2] += ((pPixelsYCbCr + k + i*(iHead.width))->pixel[2] - M[2])*((pPixelsYCbCr + k + i*(iHead.width))->pixel[2] - M[2]);
	D[3] += ((pPixelsYCbCr - (j - k) + (i + 1 + 5)*(iHead.width))->pixel[0] - M[3])*((pPixelsYCbCr - (j - k) + (i + 1 + 5)*(iHead.width))->pixel[0] - M[3]);
	D[4] += ((pPixelsYCbCr - (j - k) + (i + 1 + 5)*(iHead.width))->pixel[1] - M[4])*((pPixelsYCbCr - (j - k) + (i + 1 + 5)*(iHead.width))->pixel[1] - M[4]);
	D[5] += ((pPixelsYCbCr - (j - k) + (i + 1 + 5)*(iHead.width))->pixel[2] - M[5])*((pPixelsYCbCr - (j - k) + (i + 1 + 5)*(iHead.width))->pixel[2] - M[5]);
      }
    }

    D[0] = sqrt(D[0] / (iHead.height*j - 1));
    D[1] = sqrt(D[1] / (iHead.height*j - 1));
    D[2] = sqrt(D[2] / (iHead.height*j - 1));
    D[3] = sqrt(D[3] / (iHead.height*j - 1));
    D[4] = sqrt(D[4] / (iHead.height*j - 1));
    D[5] = sqrt(D[5] / (iHead.height*j - 1));

    for(i = 0; i < iHead.height - 5; i++){
      for(k = 0; k < j; k++){
	R[0] += ((pPixelsYCbCr + k + i*(iHead.width))->pixel[0] - M[0])*((pPixelsYCbCr + (i + 1 + 5)*(iHead.width) - (j - k))->pixel[0] - M[3]);
	R[1] += ((pPixelsYCbCr + k + i*(iHead.width))->pixel[1] - M[1])*((pPixelsYCbCr + (i + 1 + 5)*(iHead.width) - (j - k))->pixel[1] - M[4]);
	R[2] += ((pPixelsYCbCr + k + i*(iHead.width))->pixel[2] - M[2])*((pPixelsYCbCr + (i + 1 + 5)*(iHead.width) - (j - k))->pixel[2] - M[5]);
      }
    }
    R[0] /= (iHead.height*k*D[0]*D[3]);
    R[1] /= (iHead.height*k*D[1]*D[4]);
    R[2] /= (iHead.height*k*D[2]*D[5]);
    (void)fprintf(pOutFile ,"%d\t%f\t%f\t%f\n", j, R[0], R[1], R[2]);
  }
  
  (void)fclose(pOutFile);

#ifndef HIDDENDAT
  pOutFile = fopen("correl_YCbCr_-10_offset.dat", "w");
#else
  pOutFile = fopen(".correl_YCbCr_-10_offset.dat", "w");
#endif

  for(j = 1; j < iHead.width >> 2; j++){
    M[0] = M[1] = M[2] = M[3] = M[4] = M[5] = D[0] = D[1] = D[2] = D[3] = D[4] = D[5] = R[0] = R[1] = R[2] = 0;
    for(i = 0; i < iHead.height - 10; i++){
      for(k = 0; k < j; k++){
	M[0] += (pPixelsYCbCr + k + i*(iHead.width))->pixel[0];
	M[1] += (pPixelsYCbCr + k + i*(iHead.width))->pixel[1];
	M[2] += (pPixelsYCbCr + k + i*(iHead.width))->pixel[2];
	M[3] += (pPixelsYCbCr - (j - k) + (i + 11)*(iHead.width))->pixel[0];
	M[4] += (pPixelsYCbCr - (j - k) + (i + 11)*(iHead.width))->pixel[1];
	M[5] += (pPixelsYCbCr - (j - k) + (i + 11)*(iHead.width))->pixel[2];
      }
    }
    M[0] /= iHead.height*j;
    M[1] /= iHead.height*j;
    M[2] /= iHead.height*j;
    M[3] /= iHead.height*j;
    M[4] /= iHead.height*j;
    M[5] /= iHead.height*j;
    for(i = 0; i < iHead.height - 10; i++){
      for(k = 0; k < j; k++){
	D[0] += ((pPixelsYCbCr + k + i*(iHead.width))->pixel[0] - M[0])*((pPixelsYCbCr + k + i*(iHead.width))->pixel[0] - M[0]);
	D[1] += ((pPixelsYCbCr + k + i*(iHead.width))->pixel[1] - M[1])*((pPixelsYCbCr + k + i*(iHead.width))->pixel[1] - M[1]);
	D[2] += ((pPixelsYCbCr + k + i*(iHead.width))->pixel[2] - M[2])*((pPixelsYCbCr + k + i*(iHead.width))->pixel[2] - M[2]);
	D[3] += ((pPixelsYCbCr - (j - k) + (i + 11)*(iHead.width))->pixel[0] - M[3])*((pPixelsYCbCr - (j - k) + (i + 11)*(iHead.width))->pixel[0] - M[3]);
	D[4] += ((pPixelsYCbCr - (j - k) + (i + 11)*(iHead.width))->pixel[1] - M[4])*((pPixelsYCbCr - (j - k) + (i + 11)*(iHead.width))->pixel[1] - M[4]);
	D[5] += ((pPixelsYCbCr - (j - k) + (i + 11)*(iHead.width))->pixel[2] - M[5])*((pPixelsYCbCr - (j - k) + (i + 11)*(iHead.width))->pixel[2] - M[5]);
      }
    }

    D[0] = sqrt(D[0] / (iHead.height*j - 1));
    D[1] = sqrt(D[1] / (iHead.height*j - 1));
    D[2] = sqrt(D[2] / (iHead.height*j - 1));
    D[3] = sqrt(D[3] / (iHead.height*j - 1));
    D[4] = sqrt(D[4] / (iHead.height*j - 1));
    D[5] = sqrt(D[5] / (iHead.height*j - 1));

    for(i = 0; i < iHead.height - 10; i++){
      for(k = 0; k < j; k++){
	R[0] += ((pPixelsYCbCr + k + i*(iHead.width))->pixel[0] - M[0])*((pPixelsYCbCr + (i + 11)*(iHead.width) - (j - k))->pixel[0] - M[3]);
	R[1] += ((pPixelsYCbCr + k + i*(iHead.width))->pixel[1] - M[1])*((pPixelsYCbCr + (i + 11)*(iHead.width) - (j - k))->pixel[1] - M[4]);
	R[2] += ((pPixelsYCbCr + k + i*(iHead.width))->pixel[2] - M[2])*((pPixelsYCbCr + (i + 11)*(iHead.width) - (j - k))->pixel[2] - M[5]);
      }
    }
    R[0] /= (iHead.height*k*D[0]*D[3]);
    R[1] /= (iHead.height*k*D[1]*D[4]);
    R[2] /= (iHead.height*k*D[2]*D[5]);
    (void)fprintf(pOutFile ,"%d\t%f\t%f\t%f\n", j, R[0], R[1], R[2]);
  }
  
  (void)fclose(pOutFile);

#ifndef HIDDENDAT
  pOutFile = fopen("correl_YCbCr_5_offset.dat", "w");
#else
  pOutFile = fopen(".correl_YCbCr_5_offset.dat", "w");
#endif
  
  for(j = 1; j < iHead.width >> 2; j++){
    M[0] = M[1] = M[2] = M[3] = M[4] = M[5] = D[0] = D[1] = D[2] = D[3] = D[4] = D[5] = R[0] = R[1] = R[2] = 0;
    for(i = 0; i < iHead.height - 5; i++){
      for(k = 0; k < j; k++){
	M[0] += (pPixelsYCbCr + k + (i + 5)*(iHead.width))->pixel[0];
	M[1] += (pPixelsYCbCr + k + (i + 5)*(iHead.width))->pixel[1];
	M[2] += (pPixelsYCbCr + k + (i + 5)*(iHead.width))->pixel[2];
	M[3] += (pPixelsYCbCr - (j - k) + (i + 1)*(iHead.width))->pixel[0];
	M[4] += (pPixelsYCbCr - (j - k) + (i + 1)*(iHead.width))->pixel[1];
	M[5] += (pPixelsYCbCr - (j - k) + (i + 1)*(iHead.width))->pixel[2];
      }
    }
    M[0] /= iHead.height*j;
    M[1] /= iHead.height*j;
    M[2] /= iHead.height*j;
    M[3] /= iHead.height*j;
    M[4] /= iHead.height*j;
    M[5] /= iHead.height*j;
    for(i = 0; i < iHead.height - 5; i++){
      for(k = 0; k < j; k++){
	D[0] += ((pPixelsYCbCr + k + (i + 5)*(iHead.width))->pixel[0] - M[0])*((pPixelsYCbCr + k + (i + 5)*(iHead.width))->pixel[0] - M[0]);
	D[1] += ((pPixelsYCbCr + k + (i + 5)*(iHead.width))->pixel[1] - M[1])*((pPixelsYCbCr + k + (i + 5)*(iHead.width))->pixel[1] - M[1]);
	D[2] += ((pPixelsYCbCr + k + (i + 5)*(iHead.width))->pixel[2] - M[2])*((pPixelsYCbCr + k + (i + 5)*(iHead.width))->pixel[2] - M[2]);
	D[3] += ((pPixelsYCbCr - (j - k) + (i + 1)*(iHead.width))->pixel[0] - M[3])*((pPixelsYCbCr - (j - k) + (i + 1)*(iHead.width))->pixel[0] - M[3]);
	D[4] += ((pPixelsYCbCr - (j - k) + (i + 1)*(iHead.width))->pixel[1] - M[4])*((pPixelsYCbCr - (j - k) + (i + 1)*(iHead.width))->pixel[1] - M[4]);
	D[5] += ((pPixelsYCbCr - (j - k) + (i + 1)*(iHead.width))->pixel[2] - M[5])*((pPixelsYCbCr - (j - k) + (i + 1)*(iHead.width))->pixel[2] - M[5]);
      }
    }

    D[0] = sqrt(D[0] / (iHead.height*j - 1));
    D[1] = sqrt(D[1] / (iHead.height*j - 1));
    D[2] = sqrt(D[2] / (iHead.height*j - 1));
    D[3] = sqrt(D[3] / (iHead.height*j - 1));
    D[4] = sqrt(D[4] / (iHead.height*j - 1));
    D[5] = sqrt(D[5] / (iHead.height*j - 1));

    for(i = 0; i < iHead.height - 5; i++){
      for(k = 0; k < j; k++){
	R[0] += ((pPixelsYCbCr + k + (i + 5)*(iHead.width))->pixel[0] - M[0])*((pPixelsYCbCr + (i + 1)*(iHead.width) - (j - k))->pixel[0] - M[3]);
	R[1] += ((pPixelsYCbCr + k + (i + 5)*(iHead.width))->pixel[1] - M[1])*((pPixelsYCbCr + (i + 1)*(iHead.width) - (j - k))->pixel[1] - M[4]);
	R[2] += ((pPixelsYCbCr + k + (i + 5)*(iHead.width))->pixel[2] - M[2])*((pPixelsYCbCr + (i + 1)*(iHead.width) - (j - k))->pixel[2] - M[5]);
      }
    }
    R[0] /= (iHead.height*k*D[0]*D[3]);
    R[1] /= (iHead.height*k*D[1]*D[4]);
    R[2] /= (iHead.height*k*D[2]*D[5]);
    (void)fprintf(pOutFile ,"%d\t%f\t%f\t%f\n", j, R[0], R[1], R[2]);
  }
  
  (void)fclose(pOutFile);

#ifndef HIDDENDAT
  pOutFile = fopen("correl_YCbCr_10_offset.dat", "w");
#else
  pOutFile = fopen(".correl_YCbCr_10_offset.dat", "w");
#endif
  
  for(j = 1; j < iHead.width >> 2; j++){
    M[0] = M[1] = M[2] = M[3] = M[4] = M[5] = D[0] = D[1] = D[2] = D[3] = D[4] = D[5] = R[0] = R[1] = R[2] = 0;
    for(i = 0; i < iHead.height - 10; i++){
      for(k = 0; k < j; k++){
	M[0] += (pPixelsYCbCr + k + (i + 10)*(iHead.width))->pixel[0];
	M[1] += (pPixelsYCbCr + k + (i + 10)*(iHead.width))->pixel[1];
	M[2] += (pPixelsYCbCr + k + (i + 10)*(iHead.width))->pixel[2];
	M[3] += (pPixelsYCbCr - (j - k) + (i + 1)*(iHead.width))->pixel[0];
	M[4] += (pPixelsYCbCr - (j - k) + (i + 1)*(iHead.width))->pixel[1];
	M[5] += (pPixelsYCbCr - (j - k) + (i + 1)*(iHead.width))->pixel[2];
      }
    }
    M[0] /= iHead.height*j;
    M[1] /= iHead.height*j;
    M[2] /= iHead.height*j;
    M[3] /= iHead.height*j;
    M[4] /= iHead.height*j;
    M[5] /= iHead.height*j;
    for(i = 0; i < iHead.height - 10; i++){
      for(k = 0; k < j; k++){
	D[0] += ((pPixelsYCbCr + k + (i + 10)*(iHead.width))->pixel[0] - M[0])*((pPixelsYCbCr + k + (i + 10)*(iHead.width))->pixel[0] - M[0]);
	D[1] += ((pPixelsYCbCr + k + (i + 10)*(iHead.width))->pixel[1] - M[1])*((pPixelsYCbCr + k + (i + 10)*(iHead.width))->pixel[1] - M[1]);
	D[2] += ((pPixelsYCbCr + k + (i + 10)*(iHead.width))->pixel[2] - M[2])*((pPixelsYCbCr + k + (i + 10)*(iHead.width))->pixel[2] - M[2]);
	D[3] += ((pPixelsYCbCr - (j - k) + (i + 1)*(iHead.width))->pixel[0] - M[3])*((pPixelsYCbCr - (j - k) + (i + 1)*(iHead.width))->pixel[0] - M[3]);
	D[4] += ((pPixelsYCbCr - (j - k) + (i + 1)*(iHead.width))->pixel[1] - M[4])*((pPixelsYCbCr - (j - k) + (i + 1)*(iHead.width))->pixel[1] - M[4]);
	D[5] += ((pPixelsYCbCr - (j - k) + (i + 1)*(iHead.width))->pixel[2] - M[5])*((pPixelsYCbCr - (j - k) + (i + 1)*(iHead.width))->pixel[2] - M[5]);
      }
    }

    D[0] = sqrt(D[0] / (iHead.height*j - 1));
    D[1] = sqrt(D[1] / (iHead.height*j - 1));
    D[2] = sqrt(D[2] / (iHead.height*j - 1));
    D[3] = sqrt(D[3] / (iHead.height*j - 1));
    D[4] = sqrt(D[4] / (iHead.height*j - 1));
    D[5] = sqrt(D[5] / (iHead.height*j - 1));

    for(i = 0; i < iHead.height - 10; i++){
      for(k = 0; k < j; k++){
	R[0] += ((pPixelsYCbCr + k + (i + 10)*(iHead.width))->pixel[0] - M[0])*((pPixelsYCbCr + (i + 1)*(iHead.width) - (j - k))->pixel[0] - M[3]);
	R[1] += ((pPixelsYCbCr + k + (i + 10)*(iHead.width))->pixel[1] - M[1])*((pPixelsYCbCr + (i + 1)*(iHead.width) - (j - k))->pixel[1] - M[4]);
	R[2] += ((pPixelsYCbCr + k + (i + 10)*(iHead.width))->pixel[2] - M[2])*((pPixelsYCbCr + (i + 1)*(iHead.width) - (j - k))->pixel[2] - M[5]);
      }
    }
    R[0] /= (iHead.height*k*D[0]*D[3]);
    R[1] /= (iHead.height*k*D[1]*D[4]);
    R[2] /= (iHead.height*k*D[2]*D[5]);
    (void)fprintf(pOutFile ,"%d\t%f\t%f\t%f\n", j, R[0], R[1], R[2]);
  }
  
  (void)fclose(pOutFile);
  (void)puts("Succesfully calculated correlation, written to files with prefix correl_, calculating histograms.");
  
  for(i = 0; i < 3; i++){
    for(j = 0; j < iHead.width*iHead.height; j++){
      histogram[(pPixelsRGB + j)->pixel[i]][i]++;
    }
  }

#ifndef HIDDENDAT
  pOutFile = fopen("histogram_R.dat", "w");
#else
  pOutFile = fopen(".histogram_R.dat", "w");
#endif

  for(i = 0; i < 256; i++){
     (void)fprintf(pOutFile, "%d\t%d\n", i, histogram[i][2]);
  }

  (void)fclose(pOutFile);

#ifndef HIDDENDAT
  pOutFile = fopen("histogram_G.dat", "w");
#else
  pOutFile = fopen(".histogram_G.dat", "w");
#endif


  for(i = 0; i < 256; i++){
     (void)fprintf(pOutFile, "%d\t%d\n", i, histogram[i][1]);
  }

  (void)fclose(pOutFile);

#ifndef HIDDENDAT
  pOutFile = fopen("histogram_B.dat", "w");
#else
  pOutFile = fopen(".histogram_B.dat", "w");
#endif


  for(i = 0; i < 256; i++){
     (void)fprintf(pOutFile, "%d\t%d\n", i, histogram[i][0]);
  }

  pPixelsRGB2 = malloc(sizeof(BMP24PIXEL)*iHead.width*iHead.height);

  if(!pPixelsRGB2){
    (void)fputs("Error: failed to allocate memory for processed RGB pixel data!", stderr);
    (void)fclose(pInFile);
    (void)fclose(pOutFile);
    (void)free(pPixelsYCbCr);
    (void)free(pPixelsRGB);
    return EXIT_FAILURE;
  }

  (void)memset(pPixelsRGB2, 0, sizeof(BMP24PIXEL)*iHead.width*iHead.height);

  for(i = 0; i < iHead.width*iHead.height; i++){
    temp = round((double)(pPixelsYCbCr + i)->pixel[0] - 0.714*((double)(pPixelsYCbCr + i)->pixel[2] - 128.0) - 0.334*((double)(pPixelsYCbCr + i)->pixel[1] - 128.0));
    if(temp > 255){
      temp = 255;
    }else if(temp < 0){
      temp = 0;
    }
    (pPixelsRGB2 + i)->pixel[2] = temp;

    temp = round((double)(pPixelsYCbCr + i)->pixel[0] + 1.402*((double)(pPixelsYCbCr + i)->pixel[2] - 128.0));
    if(temp > 255){
      temp = 255;
    }else if(temp < 0){
      temp = 0;
    }
    (pPixelsRGB2 + i)->pixel[1] = temp;

    temp = round((double)(pPixelsYCbCr + i)->pixel[0] + 1.772*((double)(pPixelsYCbCr + i)->pixel[1] - 128.0));
    if(temp > 255){
      temp = 255;
    }else if(temp < 0){
      temp = 0;
    }
    (pPixelsRGB2 + i)->pixel[0] = temp;
  }

  PSNR = 0.0;

  for(i = 0; i < iHead.width*iHead.height; i++){
    PSNR += abs((pPixelsRGB + i)->pixel[0] - (pPixelsRGB2 + i)->pixel[0]);
  }

  PSNR = 10.0 * log10(((double)iHead.width*(double)iHead.height*255.0*255.0)/PSNR);

  (void)printf("PSNR of conversion for\033[34m blue\033[37m is %f.\n", PSNR);

  PSNR = 0.0;

  for(i = 0; i < iHead.width*iHead.height; i++){
    PSNR += abs((pPixelsRGB + i)->pixel[1] - (pPixelsRGB2 + i)->pixel[1]);
  }

  PSNR = 10.0 * log10(((double)iHead.width*(double)iHead.height*255.0*255.0)/PSNR);

  (void)printf("PSNR of conversion for\033[32m green\033[37m is %f.\n", PSNR);

  PSNR = 0.0;

  for(i = 0; i < iHead.width*iHead.height; i++){
    PSNR += abs((pPixelsRGB + i)->pixel[2] - (pPixelsRGB2 + i)->pixel[2]);
  }

  PSNR = 10.0 * log10(((double)iHead.width*(double)iHead.height*255.0*255.0)/PSNR);

  (void)printf("PSNR of conversion for\033[31m red\033[37m is %f.\n", PSNR);

  (void)fclose(pOutFile);
  
#ifdef DEBUG
  (void)puts("DEBUG INFO: Writing BMP file for debug.");
  pOutFile = fopen("debug_data/debug_after.bmp", "wb");

  if(!pOutFile){
    (void)fputs("Debug error: failed to open file for writing debug BMP file!\n", stderr);
    (void)free(pPixelsYCbCr);
    (void)free(pPixelsRGB);
    (void)free(pPixelsRGB2);
    (void)fclose(pInFile);
    return EXIT_FAILURE;
  }

  (void)fwrite(&head, sizeof(BMPHEAD), 1, pOutFile);
  (void)fwrite(&iHead, sizeof(BMPINFOHEAD), 1, pOutFile);

  for(i = 0; i < iHead.height; i++){
    (void)fwrite((pPixelsRGB + i*iHead.width), sizeof(BMP24PIXEL), iHead.width, pOutFile);
    (void)fwrite(pPixelsRGB, sizeof(unsigned char), dBytes, pOutFile);
  }

  (void)fclose(pOutFile);
#endif

  for(i = 0; i < 3; i++){
    for(j = 0; j < iHead.width*iHead.height; j++){
      histogram[(pPixelsYCbCr + j)->pixel[i]][i+15]++;
    }
  }

#ifndef HIDDENDAT
  pOutFile = fopen("histogram_Cr.dat", "w");
#else
  pOutFile = fopen(".histogram_Cr.dat", "w");
#endif


  for(i = 0; i < 256; i++){
     (void)fprintf(pOutFile, "%d\t%d\n", i, histogram[i][15]);
  }

  (void)fclose(pOutFile);

#ifndef HIDDENDAT
  pOutFile = fopen("histogram_Cb.dat", "w");
#else
  pOutFile = fopen(".histogram_Cb.dat", "w");
#endif


  for(i = 0; i < 256; i++){
     (void)fprintf(pOutFile, "%d\t%d\n", i, histogram[i][16]);
  }

  (void)fclose(pOutFile);

#ifndef HIDDENDAT
  pOutFile = fopen("histogram_Y.dat", "w");
#else
  pOutFile = fopen(".histogram_Y.dat", "w");
#endif


  for(i = 0; i < 256; i++){
     (void)fprintf(pOutFile, "%d\t%d\n", i, histogram[i][17]);
  }

  (void)fclose(pOutFile);

  (void)memset(&outName, 0, sizeof(char)*MAX_FNAME_LEN);
  
  (void)strcat(outName, argv[namePos]);
  
  pLoc = strstr(outName, ".bmp");
  
  if(pLoc){
    *pLoc = '\0';
    *(pLoc + 1) = '\0';
    *(pLoc + 2) = '\0';
    *(pLoc + 3) = '\0';
  }
  
  (void)strcat(outName, "_mirror.bmp");
  
#ifdef DEBUG
  (void)printf("Will write to file: %s\n", outName);
#endif

  pOutFile = fopen(outName, "wb");
  
  if(!pOutFile){
    (void)fputs("Error: can't open file for writing mirrored image.\n", stderr);
    (void)free(pPixelsRGB);
    (void)free(pPixelsYCbCr);
    (void)fclose(pInFile);
    return EXIT_FAILURE;
  }

  (void)fwrite(&head, sizeof(BMPHEAD), 1, pOutFile);
  (void)fwrite(&iHead, sizeof(BMPINFOHEAD), 1, pOutFile);

  for(i = 0; i < iHead.height; i++){
    for(j = 0; j < iHead.width>>1; j++){
      (void)fwrite(&(pPixelsRGB+j+(i*iHead.width))->pixel, sizeof(BMP24PIXEL), 1, pOutFile);
    }
    for(; j > 0; j--){
      (void)fwrite(&(pPixelsRGB+j+(i*iHead.width))->pixel, sizeof(BMP24PIXEL), 1, pOutFile);
    }
    (void)fwrite(pPixelsRGB, sizeof(char), dBytes, pOutFile);
  }

  (void)fclose(pOutFile);

  pLoc = strstr(outName, "mirror.bmp");
  
  if(pLoc){
    *pLoc = '\0';
    *(pLoc + 1) = '\0';
    *(pLoc + 2) = '\0';
    *(pLoc + 3) = '\0';
    *(pLoc + 4) = '\0';
    *(pLoc + 5) = '\0';
    *(pLoc + 6) = '\0';
    *(pLoc + 7) = '\0';
    *(pLoc + 8) = '\0';
    *(pLoc + 9) = '\0';
  }
  
  (void)strcat(outName, "grayscale.bmp");
  
#ifdef DEBUG
  (void)printf("Will write to file: %s\n", outName);
#endif

  pOutFile = fopen(outName, "wb");
  
  if(!pOutFile){
    (void)fputs("Error: can't open file for writing grayscale image.\n", stderr);
    (void)free(pPixelsRGB);
    (void)free(pPixelsYCbCr);
    (void)fclose(pInFile);
    return EXIT_FAILURE;
  }

  (void)fwrite(&head, sizeof(BMPHEAD), 1, pOutFile);
  (void)fwrite(&iHead, sizeof(BMPINFOHEAD), 1, pOutFile);

  for(i = 0; i < iHead.height; i++){
    for(j = 0; j < iHead.width; j++){
      namePos = ((pPixelsRGB+j+(i*iHead.width))->pixel[0] + (pPixelsRGB+j+(i*iHead.width))->pixel[1] + (pPixelsRGB+j+(i*iHead.width))->pixel[2])/3.0;
      (void)fwrite(&namePos, sizeof(char), 1, pOutFile);
      (void)fwrite(&namePos, sizeof(char), 1, pOutFile);
      (void)fwrite(&namePos, sizeof(char), 1, pOutFile);
        }
    (void)fwrite(pPixelsRGB, sizeof(char), dBytes, pOutFile);
  }

  (void)fclose(pOutFile);
/*  
  pLoc = strstr(outName, "grayscale.bmp");
  
  if(pLoc){
    *pLoc = '\0';
    *(pLoc + 1) = '\0';
    *(pLoc + 2) = '\0';
    *(pLoc + 3) = '\0';
    *(pLoc + 4) = '\0';
    *(pLoc + 5) = '\0';
    *(pLoc + 6) = '\0';
    *(pLoc + 7) = '\0';
    *(pLoc + 8) = '\0';
    *(pLoc + 9) = '\0';
    *(pLoc + 10) = '\0';
    *(pLoc + 11) = '\0';
    *(pLoc + 12) = '\0';
  }
  
  (void)strcat(outName, "bw");

  for(k = 0; k < 8; k++){  

#ifdef DEBUG
  (void)printf("Will write to file: %s\n", outName);
#endif

  pOutFile = fopen(outName, "wb");
  
  if(!pOutFile){
    (void)fputs("Error: can't open file for writing grayscale image.\n", stderr);
    (void)free(pPixelsRGB);
    (void)free(pPixelsYCbCr);
    (void)fclose(pInFile);
    return EXIT_FAILURE;
  }

  iHead.bits = 1;
  head.size = sizeof(BMPHEAD) + sizeof(BMPINFOHEAD) + (round((double)iHead.width/8.0*3.0) + (4-((round((double)iHead.width/8.0*3.0))%4)))*iHead.height;

  (void)fwrite(&head, sizeof(BMPHEAD), 1, pOutFile);
  (void)fwrite(&iHead, sizeof(BMPINFOHEAD), 1, pOutFile);

    for(i = 0; i < iHead.height; i++){
      for(j = 0; j < iHead.width; j++){
        namePos = ((pPixelsYCbCr+j+(i*iHead.width))->pixel[0] + (pPixelsYCbCr+j+(i*iHead.width))->pixel[0] + (pPixelsYCbCr+j+(i*iHead.width))->pixel[0])/3.0;

        (void)fwrite()
      }
      (void)fwrite(pPixelsRGB, sizeof(char), dBytes, pOutFile);
    }
  }

  (void)fclose(pOutFile);
*/
  pPixelsYCbCrNech = malloc(sizeof(YCbCr24PIXEL)*iHead.width * iHead.height);

  for(i = 0; i < iHead.height; i+=2){
    for(j = 0; j < iHead.width; j+=2){
      (pPixelsYCbCrNech + (i>>1)*(iHead.width>>1) + (j>>1))->pixel[1] = (pPixelsYCbCr + (i*iHead.width) + j)->pixel[1];
      (pPixelsYCbCrNech + (i>>1)*(iHead.width>>1) + (j>>1))->pixel[2] = (pPixelsYCbCr + (i*iHead.width) + j)->pixel[2];
    }
  }
  
  pPixelsYCbCrArith = malloc(sizeof(YCbCr24PIXEL)*iHead.width * iHead.height);

  for(i = 0; i < iHead.height; i+=2){
    for(j = 0; j < iHead.width; j+=2, temp = 0){
      temp += ((pPixelsYCbCr + (i*iHead.width) + j)->pixel[1]);
      temp += (j + 1 > iHead.width) ? ((pPixelsYCbCr + (i*iHead.width) + j)->pixel[1]) : ((pPixelsYCbCr + (i*iHead.width) + j + 1)->pixel[1]);
      temp += (i + 1 > iHead.height) ? ((pPixelsYCbCr + (i*iHead.width) + j)->pixel[1]) : ((pPixelsYCbCr + ((i + 1)*iHead.width) + j)->pixel[1]);
      temp += (j + 1 > iHead.width || i + 1 > iHead.height ) ? ((pPixelsYCbCr + (i*iHead.width) + j)->pixel[1]) : ((pPixelsYCbCr + ((i + 1)*iHead.width) + j + 1)->pixel[1]);
      temp = round(temp/4.0);
      (pPixelsYCbCrArith + (i>>1)*(iHead.width>>1) + (j>>1))->pixel[1] = temp;

      temp = 0;

      temp += ((pPixelsYCbCr + (i*iHead.width) + j)->pixel[2]);
      temp += (j + 1 > iHead.width) ? ((pPixelsYCbCr + (i*iHead.width) + j)->pixel[2]) : ((pPixelsYCbCr + (i*iHead.width) + j + 1)->pixel[2]);
      temp += (i + 1 > iHead.height) ? ((pPixelsYCbCr + (i*iHead.width) + j)->pixel[2]) : ((pPixelsYCbCr + ((i + 1)*iHead.width) + j)->pixel[2]);
      temp += (j + 1 > iHead.width || i + 1 > iHead.height ) ? ((pPixelsYCbCr + (i*iHead.width) + j)->pixel[2]) : ((pPixelsYCbCr + ((i + 1)*iHead.width) + j + 1)->pixel[2]);
      temp = round(temp/4.0);
      (pPixelsYCbCrArith + (i>>1)*(iHead.width>>1) + (j>>1))->pixel[2] = temp;
    }
  }

#ifdef DEBUG
  pOutFile = fopen("debug_data/decimated_Cb_even.bmp", "wb");
  
  if(!pOutFile){
    (void)fputs("Error: can't write debug data for evenly decimated Cb image!", stderr);
    (void)free(pPixelsYCbCrNech);
    (void)free(pPixelsYCbCrArith);
    (void)free(pPixelsYCbCr);
    (void)free(pPixelsRGB2);
    (void)free(pPixelsRGB);
    (void)fclose(pInFile);
    return EXIT_FAILURE;
  }
  
  dHead = head;
  dIHead = iHead;
  dIHead.width = iHead.width>>1;
  dIHead.height = iHead.height>>1;
  dHead.size = sizeof(BMPHEAD) + sizeof(BMPINFOHEAD) + (sizeof(BMP24PIXEL)*dIHead.width + dBytes)*dIHead.height;
  dIHead.imageSize = (sizeof(BMP24PIXEL)*dIHead.width + dBytes)*dIHead.height;
  (void)fwrite(&dHead, sizeof(BMPHEAD), 1, pOutFile);
  (void)fwrite(&dIHead, sizeof(BMPINFOHEAD), 1, pOutFile);
  
  for(i = 0; i < dIHead.height; i++){
    for(j = 0; j < dIHead.width; j++){
      (void)fwrite(&(pPixelsYCbCrNech + j + (i*dIHead.width))->pixel[1], sizeof(char), 1, pOutFile);
      (void)fwrite(&(pPixelsYCbCrNech + j + (i*dIHead.width))->pixel[1], sizeof(char), 1, pOutFile);
      (void)fwrite(&(pPixelsYCbCrNech + j + (i*dIHead.width))->pixel[1], sizeof(char), 1, pOutFile);
    }
    (void)fwrite(pPixelsYCbCrNech, sizeof(char), dBytes, pOutFile);
  }

  (void)fclose(pOutFile);
  pOutFile = fopen("debug_data/decimated_Cr_even.bmp", "wb");
  
  if(!pOutFile){
    (void)fputs("Error: can't write debug data for evenly decimated Cr image!", stderr);
    (void)free(pPixelsYCbCrNech);
    (void)free(pPixelsYCbCrArith);
    (void)free(pPixelsYCbCr);
    (void)free(pPixelsRGB2);
    (void)free(pPixelsRGB);
    (void)fclose(pInFile);
    return EXIT_FAILURE;
  }
  
  (void)fwrite(&dHead, sizeof(BMPHEAD), 1, pOutFile);
  (void)fwrite(&dIHead, sizeof(BMPINFOHEAD), 1, pOutFile);
  
  for(i = 0; i < dIHead.height; i++){
    for(j = 0; j < dIHead.width; j++){
      (void)fwrite(&(pPixelsYCbCrNech + j + (i*dIHead.width))->pixel[2], sizeof(char), 1, pOutFile);
      (void)fwrite(&(pPixelsYCbCrNech + j + (i*dIHead.width))->pixel[2], sizeof(char), 1, pOutFile);
      (void)fwrite(&(pPixelsYCbCrNech + j + (i*dIHead.width))->pixel[2], sizeof(char), 1, pOutFile);
    }
    (void)fwrite(pPixelsYCbCrNech, sizeof(char), dBytes, pOutFile);
  }
  
  (void)fclose(pOutFile);
  pOutFile = fopen("debug_data/decimated_Cb_avg.bmp", "wb");
  
  if(!pOutFile){
    (void)fputs("Error: can't write debug data for averagely decimated Cb image!", stderr);
    (void)free(pPixelsYCbCrNech);
    (void)free(pPixelsYCbCrArith);
    (void)free(pPixelsYCbCr);
    (void)free(pPixelsRGB2);
    (void)free(pPixelsRGB);
    (void)fclose(pInFile);
    return EXIT_FAILURE;
  }
  
  (void)fwrite(&dHead, sizeof(BMPHEAD), 1, pOutFile);
  (void)fwrite(&dIHead, sizeof(BMPINFOHEAD), 1, pOutFile);
  
  for(i = 0; i < dIHead.height; i++){
    for(j = 0; j < dIHead.width; j++){
      (void)fwrite(&(pPixelsYCbCrArith + j + (i*dIHead.width))->pixel[1], sizeof(char), 1, pOutFile);
      (void)fwrite(&(pPixelsYCbCrArith + j + (i*dIHead.width))->pixel[1], sizeof(char), 1, pOutFile);
      (void)fwrite(&(pPixelsYCbCrArith + j + (i*dIHead.width))->pixel[1], sizeof(char), 1, pOutFile);
    }
    (void)fwrite(pPixelsYCbCrNech, sizeof(char), dBytes, pOutFile);
  }
  
  (void)fclose(pOutFile);
  pOutFile = fopen("debug_data/decimated_Cr_avg.bmp", "wb");
  
  if(!pOutFile){
    (void)fputs("Error: can't write debug data for averagely decimated Cb image!", stderr);
    (void)free(pPixelsYCbCrNech);
    (void)free(pPixelsYCbCrArith);
    (void)free(pPixelsYCbCr);
    (void)free(pPixelsRGB2);
    (void)free(pPixelsRGB);
    (void)fclose(pInFile);
    return EXIT_FAILURE;
  }
  
  (void)fwrite(&dHead, sizeof(BMPHEAD), 1, pOutFile);
  (void)fwrite(&dIHead, sizeof(BMPINFOHEAD), 1, pOutFile);
  
  for(i = 0; i < dIHead.height; i++){
    for(j = 0; j < dIHead.width; j++){
      (void)fwrite(&(pPixelsYCbCrArith + j + (i*dIHead.width))->pixel[2], sizeof(char), 1, pOutFile);
      (void)fwrite(&(pPixelsYCbCrArith + j + (i*dIHead.width))->pixel[2], sizeof(char), 1, pOutFile);
      (void)fwrite(&(pPixelsYCbCrArith + j + (i*dIHead.width))->pixel[2], sizeof(char), 1, pOutFile);
    }
    (void)fwrite(pPixelsYCbCrNech, sizeof(char), dBytes, pOutFile);
  }
  
  (void)fclose(pOutFile);
#endif
  
  (void)puts("Recovering evenly decimated components.");
  
  pPixelsYCbCrRcvr = malloc(iHead.height*iHead.width*sizeof(YCbCr24PIXEL));
  
  for(i = 0; i < iHead.height; i++){
    for(j = 0; j < iHead.width; j++){
      (pPixelsYCbCrRcvr + j + i*(iHead.width))->pixel[0] = (pPixelsYCbCr + j + i*(iHead.width))->pixel[0];
    }
  }

  for(j = 0; j < iHead.height; j+=2){
    for(i = 0; i < iHead.width; i+=2){
      (pPixelsYCbCrRcvr + i + j*(iHead.width))->pixel[1] = (pPixelsYCbCrNech + (i>>1) + (j>>1)*(iHead.width>>1))->pixel[1];
      if(i + 1 < iHead.width){
        (pPixelsYCbCrRcvr + i + 1 + j*(iHead.width))->pixel[1] = (pPixelsYCbCrNech + (i>>1) + (j>>1)*(iHead.width>>1))->pixel[1];
      }
      if(j + 1 < iHead.height){
        (pPixelsYCbCrRcvr + i + (j + 1)*(iHead.width))->pixel[1] = (pPixelsYCbCrNech + (i>>1) + (j>>1)*(iHead.width>>1))->pixel[1];
      }
      if(i + 1 < iHead.width && j + 1 < iHead.height){
        (pPixelsYCbCrRcvr + i + 1 + (j + 1)*(iHead.width))->pixel[1] = (pPixelsYCbCrNech + (i>>1) + (j>>1)*(iHead.width>>1))->pixel[1];
      }
    }
  }

  for(j = 0; j < iHead.height; j+=2){
    for(i = 0; i < iHead.width; i+=2){
      (pPixelsYCbCrRcvr + i + j*(iHead.width))->pixel[2] = (pPixelsYCbCrNech + (i>>1) + (j>>1)*(iHead.width>>1))->pixel[2];
      if(i + 1 < iHead.width){
        (pPixelsYCbCrRcvr + i + 1 + j*(iHead.width))->pixel[2] = (pPixelsYCbCrNech + (i>>1) + (j>>1)*(iHead.width>>1))->pixel[2];
      }
      if(j + 1 < iHead.height){
        (pPixelsYCbCrRcvr + i + (j + 1)*(iHead.width))->pixel[2] = (pPixelsYCbCrNech + (i>>1) + (j>>1)*(iHead.width>>1))->pixel[2];
      }
      if(i + 1 < iHead.width && j + 1 < iHead.height){
        (pPixelsYCbCrRcvr + i + 1 + (j + 1)*(iHead.width))->pixel[2] = (pPixelsYCbCrNech + (i>>1) + (j>>1)*(iHead.width>>1))->pixel[2];
      }
    }
  }

  (void)puts("Calculating PSNR for evenly decimated components.");

  for(i = 0; i < iHead.width*iHead.height; i++){
    temp = round((double)(pPixelsYCbCrRcvr + i)->pixel[0] - 0.714*((double)(pPixelsYCbCrRcvr + i)->pixel[2] - 128.0) - 0.334*((double)(pPixelsYCbCrRcvr + i)->pixel[1] - 128.0));
    if(temp > 255){
      temp = 255;
    }else if(temp < 0){
      temp = 0;
    }
    (pPixelsRGB2 + i)->pixel[2] = temp;

    temp = round((double)(pPixelsYCbCrRcvr + i)->pixel[0] + 1.402*((double)(pPixelsYCbCrRcvr + i)->pixel[2] - 128.0));
    if(temp > 255){
      temp = 255;
    }else if(temp < 0){
      temp = 0;
    }
    (pPixelsRGB2 + i)->pixel[1] = temp;

    temp = round((double)(pPixelsYCbCrRcvr + i)->pixel[0] + 1.772*((double)(pPixelsYCbCrRcvr + i)->pixel[1] - 128.0));
    if(temp > 255){
      temp = 255;
    }else if(temp < 0){
      temp = 0;
    }
    (pPixelsRGB2 + i)->pixel[0] = temp;
  }

  PSNR = 0.0;

  for(i = 0; i < iHead.width*iHead.height; i++){
    PSNR += abs((pPixelsRGB + i)->pixel[0] - (pPixelsRGB2 + i)->pixel[0]);
  }

  PSNR = 10.0 * log10(((double)iHead.width*(double)iHead.height*255.0*255.0)/PSNR);

  (void)printf("PSNR of conversion for\033[34m blue\033[37m is %f.\n", PSNR);

  PSNR = 0.0;

  for(i = 0; i < iHead.width*iHead.height; i++){
    PSNR += abs((pPixelsRGB + i)->pixel[1] - (pPixelsRGB2 + i)->pixel[1]);
  }

  PSNR = 10.0 * log10(((double)iHead.width*(double)iHead.height*255.0*255.0)/PSNR);

  (void)printf("PSNR of conversion for\033[32m green\033[37m is %f.\n", PSNR);

  PSNR = 0.0;

  for(i = 0; i < iHead.width*iHead.height; i++){
    PSNR += abs((pPixelsRGB + i)->pixel[2] - (pPixelsRGB2 + i)->pixel[2]);
  }

  PSNR = 10.0 * log10(((double)iHead.width*(double)iHead.height*255.0*255.0)/PSNR);

  (void)printf("PSNR of conversion for\033[31m red\033[37m is %f.\n", PSNR);

  (void)puts("Recovering averagely decimated components.");
  
  
  for(i = 0; i < iHead.height; i++){
    for(j = 0; j < iHead.width; j++){
      (pPixelsYCbCrRcvr + j + i*(iHead.width))->pixel[0] = (pPixelsYCbCr + j + i*(iHead.width))->pixel[0];
    }
  }

  for(j = 0; j < iHead.height; j+=2){
    for(i = 0; i < iHead.width; i+=2){
      (pPixelsYCbCrRcvr + i + j*(iHead.width))->pixel[1] = (pPixelsYCbCrArith + (i>>1) + (j>>1)*(iHead.width>>1))->pixel[1];
      if(i + 1 < iHead.width){
        (pPixelsYCbCrRcvr + i + 1 + j*(iHead.width))->pixel[1] = (pPixelsYCbCrArith + (i>>1) + (j>>1)*(iHead.width>>1))->pixel[1];
      }
      if(j + 1 < iHead.height){
        (pPixelsYCbCrRcvr + i + (j + 1)*(iHead.width))->pixel[1] = (pPixelsYCbCrArith + (i>>1) + (j>>1)*(iHead.width>>1))->pixel[1];
      }
      if(i + 1 < iHead.width && j + 1 < iHead.height){
        (pPixelsYCbCrRcvr + i + 1 + (j + 1)*(iHead.width))->pixel[1] = (pPixelsYCbCrArith + (i>>1) + (j>>1)*(iHead.width>>1))->pixel[1];
      }
    }
  }

  for(j = 0; j < iHead.height; j+=2){
    for(i = 0; i < iHead.width; i+=2){
      (pPixelsYCbCrRcvr + i + j*(iHead.width))->pixel[2] = (pPixelsYCbCrArith + (i>>1) + (j>>1)*(iHead.width>>1))->pixel[2];
      if(i + 1 < iHead.width){
        (pPixelsYCbCrRcvr + i + 1 + j*(iHead.width))->pixel[2] = (pPixelsYCbCrArith + (i>>1) + (j>>1)*(iHead.width>>1))->pixel[2];
      }
      if(j + 1 < iHead.height){
        (pPixelsYCbCrRcvr + i + (j + 1)*(iHead.width))->pixel[2] = (pPixelsYCbCrArith + (i>>1) + (j>>1)*(iHead.width>>1))->pixel[2];
      }
      if(i + 1 < iHead.width && j + 1 < iHead.height){
        (pPixelsYCbCrRcvr + i + 1 + (j + 1)*(iHead.width))->pixel[2] = (pPixelsYCbCrArith + (i>>1) + (j>>1)*(iHead.width>>1))->pixel[2];
      }
    }
  }

  (void)puts("Calculating PSNR for averagely decimated components.");

  for(i = 0; i < iHead.width*iHead.height; i++){
    temp = round((double)(pPixelsYCbCrRcvr + i)->pixel[0] - 0.714*((double)(pPixelsYCbCrRcvr + i)->pixel[2] - 128.0) - 0.334*((double)(pPixelsYCbCrRcvr + i)->pixel[1] - 128.0));
    if(temp > 255){
      temp = 255;
    }else if(temp < 0){
      temp = 0;
    }
    (pPixelsRGB2 + i)->pixel[2] = temp;

    temp = round((double)(pPixelsYCbCrRcvr + i)->pixel[0] + 1.402*((double)(pPixelsYCbCrRcvr + i)->pixel[2] - 128.0));
    if(temp > 255){
      temp = 255;
    }else if(temp < 0){
      temp = 0;
    }
    (pPixelsRGB2 + i)->pixel[1] = temp;

    temp = round((double)(pPixelsYCbCrRcvr + i)->pixel[0] + 1.772*((double)(pPixelsYCbCrRcvr + i)->pixel[1] - 128.0));
    if(temp > 255){
      temp = 255;
    }else if(temp < 0){
      temp = 0;
    }
    (pPixelsRGB2 + i)->pixel[0] = temp;
  }

  PSNR = 0.0;

  for(i = 0; i < iHead.width*iHead.height; i++){
    PSNR += abs((pPixelsRGB + i)->pixel[0] - (pPixelsRGB2 + i)->pixel[0]);
  }

  PSNR = 10.0 * log10(((double)iHead.width*(double)iHead.height*255.0*255.0)/PSNR);

  (void)printf("PSNR of conversion for\033[34m blue\033[37m is %f.\n", PSNR);

  PSNR = 0.0;

  for(i = 0; i < iHead.width*iHead.height; i++){
    PSNR += abs((pPixelsRGB + i)->pixel[1] - (pPixelsRGB2 + i)->pixel[1]);
  }

  PSNR = 10.0 * log10(((double)iHead.width*(double)iHead.height*255.0*255.0)/PSNR);

  (void)printf("PSNR of conversion for\033[32m green\033[37m is %f.\n", PSNR);

  PSNR = 0.0;

  for(i = 0; i < iHead.width*iHead.height; i++){
    PSNR += abs((pPixelsRGB + i)->pixel[2] - (pPixelsRGB2 + i)->pixel[2]);
  }

  PSNR = 10.0 * log10(((double)iHead.width*(double)iHead.height*255.0*255.0)/PSNR);

  (void)printf("PSNR of conversion for\033[31m red\033[37m is %f.\n", PSNR);

  for(i = 0; i < 3; i++){
    for(j = 0; j < 255; j++){
      if(histogram[j][i] != 0){
        H[i] -= (((double)histogram[j][i]/((double)iHead.width * iHead.height)) * log2((double)histogram[j][i]/(double)(iHead.width * iHead.height)));
      }
    }
  }

  for(i = 0; i < 3; i++){
    for(j = 0; j < 255; j++){
      if(histogram[j][i + 15] != 0){
        H[i + 15] -= (((double)histogram[j][i + 15]/((double)iHead.width * iHead.height)) * log2((double)histogram[j][i + 15]/(double)(iHead.width * iHead.height)));
      }
    }
  }
  
  (void)puts("#Calculating DPCM data.\\");
  
  (void)puts("#=========================================================================#");
  (void)printf("%-75s%f||\n","||Bits for compressing the\033[34m blue\033[37m component: ", H[0]);
  (void)printf("%-75s%f||\n","||Bits for compressing the\033[32m green\033[37m component: ", H[1]);
  (void)printf("%-75s%f||\n","||Bits for compressing the\033[31m red\033[37m component: ", H[2]);
  (void)printf("%-65s%f||\n","||Bits for compressing the Y component: ", H[15]);
  (void)printf("%-65s%f||\n","||Bits for compressing the Cb component: ", H[16]);
  (void)printf("%-65s%f||\n","||Bits for compressing the Cr component: ", H[17]);
  
  pDPCMData = malloc(sizeof(unsigned char)*(iHead.width - 1)*(iHead.height - 1));
  
  if(!pDPCMData){
    (void)fputs("Error: failed to allocate space for DPCM data!\n", stderr);
    (void)free(pPixelsYCbCrRcvr);
    (void)free(pPixelsYCbCrNech);
    (void)free(pPixelsYCbCrArith);
    (void)free(pPixelsYCbCr);
    (void)free(pPixelsRGB2);
    (void)free(pPixelsRGB);
    (void)fclose(pInFile);
    return EXIT_FAILURE;
  }
  
  (void)memset(pDPCMData, 0, sizeof(unsigned char)*(iHead.width - 1)*(iHead.height - 1));

  for(j = 0; j < 3; j++){
    for(i = 1; i < iHead.height; i++){
      for(k = 1; k < iHead.width; k++){
        *(pDPCMData + (k - 1) + (i - 1)*(iHead.width - 1)) = abs((pPixelsRGB + k + i*iHead.width - 1)->pixel[j] - (pPixelsRGB + k + i*iHead.width)->pixel[j]);
      }
    }
    for(i = 0; i < (iHead.width - 1)*(iHead.height - 1); i++){
      histogram[(unsigned char)*(pDPCMData + i)][j+3]++;
    }
  }

#ifndef HIDDENDAT
  pOutFile = fopen("DPCM_histogram_B.dat", "w");
#else
  pOutFile = fopen(".DPCM_histogram_B.dat", "w");
#endif

  if(!pOutFile){
    (void)fputs("Error: failed to open file to write DPCM data for blue!\n", stderr);
    (void)free(pDPCMData);
    (void)free(pPixelsYCbCrRcvr);
    (void)free(pPixelsYCbCrNech);
    (void)free(pPixelsYCbCrArith);
    (void)free(pPixelsYCbCr);
    (void)free(pPixelsRGB2);
    (void)free(pPixelsRGB);
    (void)fclose(pInFile);
    return EXIT_FAILURE;
  }

  for(i = 0; i < 255; i++){
    (void)fprintf(pOutFile, "%d\t%d\n", i, histogram[i][3]);
  }

  (void)fclose(pOutFile);

#ifndef HIDDENDAT
  pOutFile = fopen("DPCM_histogram_G.dat", "w");
#else
  pOutFile = fopen(".DPCM_histogram_G.dat", "w");
#endif
  
  if(!pOutFile){
    (void)fputs("Error: failed to open file to write DPCM data for green!\n", stderr);
    (void)free(pDPCMData);
    (void)free(pPixelsYCbCrRcvr);
    (void)free(pPixelsYCbCrNech);
    (void)free(pPixelsYCbCrArith);
    (void)free(pPixelsYCbCr);
    (void)free(pPixelsRGB2);
    (void)free(pPixelsRGB);
    (void)fclose(pInFile);
    return EXIT_FAILURE;
  }

  for(i = 0; i < 255; i++){
    (void)fprintf(pOutFile, "%d\t%d\n", i, histogram[i][4]);
  }

  (void)fclose(pOutFile);
  
#ifndef HIDDENDAT
  pOutFile = fopen("DPCM_histogram_R.dat", "w");
#else
  pOutFile = fopen(".DPCM_histogram_R.dat", "w");
#endif
  
  if(!pOutFile){
    (void)fputs("Error: failed to open file to write DPCM data for red!\n", stderr);
    (void)free(pDPCMData);
    (void)free(pPixelsYCbCrRcvr);
    (void)free(pPixelsYCbCrNech);
    (void)free(pPixelsYCbCrArith);
    (void)free(pPixelsYCbCr);
    (void)free(pPixelsRGB2);
    (void)free(pPixelsRGB);
    (void)fclose(pInFile);
    return EXIT_FAILURE;
  }

  for(i = 0; i < 255; i++){
    (void)fprintf(pOutFile, "%d\t%d\n", i, histogram[i][5]);
  }
  
  (void)fclose(pOutFile);
  
for(j = 0; j < 3; j++){
    for(i = 1; i < iHead.height; i++){
      for(k = 1; k < iHead.width; k++){
        *(pDPCMData + (k - 1) + (i - 1)*(iHead.width - 1)) = abs((pPixelsRGB + k + (i - 1)*iHead.width)->pixel[j] - (pPixelsRGB + k + i*iHead.width)->pixel[j]);
      }
    }
    for(i = 0; i < (iHead.width - 1)*(iHead.height - 1); i++){
      histogram[(unsigned char)*(pDPCMData + i)][j+6]++;
    }
  }

#ifndef HIDDENDAT
  pOutFile = fopen("DPCM_histogram_B_top.dat", "w");
#else
  pOutFile = fopen(".DPCM_histogram_B_top.dat", "w");
#endif

  if(!pOutFile){
    (void)fputs("Error: failed to open file to write DPCM data for blue!\n", stderr);
    (void)free(pDPCMData);
    (void)free(pPixelsYCbCrRcvr);
    (void)free(pPixelsYCbCrNech);
    (void)free(pPixelsYCbCrArith);
    (void)free(pPixelsYCbCr);
    (void)free(pPixelsRGB2);
    (void)free(pPixelsRGB);
    (void)fclose(pInFile);
    return EXIT_FAILURE;
  }

  for(i = 0; i < 255; i++){
    (void)fprintf(pOutFile, "%d\t%d\n", i, histogram[i][6]);
  }

  (void)fclose(pOutFile);

#ifndef HIDDENDAT
  pOutFile = fopen("DPCM_histogram_G_top.dat", "w");
#else
  pOutFile = fopen(".DPCM_histogram_G_top.dat", "w");
#endif
  
  if(!pOutFile){
    (void)fputs("Error: failed to open file to write DPCM data for green!\n", stderr);
    (void)free(pDPCMData);
    (void)free(pPixelsYCbCrRcvr);
    (void)free(pPixelsYCbCrNech);
    (void)free(pPixelsYCbCrArith);
    (void)free(pPixelsYCbCr);
    (void)free(pPixelsRGB2);
    (void)free(pPixelsRGB);
    (void)fclose(pInFile);
    return EXIT_FAILURE;
  }

  for(i = 0; i < 255; i++){
    (void)fprintf(pOutFile, "%d\t%d\n", i, histogram[i][7]);
  }

  (void)fclose(pOutFile);
  
#ifndef HIDDENDAT
  pOutFile = fopen("DPCM_histogram_R_top.dat", "w");
#else
  pOutFile = fopen(".DPCM_histogram_R_top.dat", "w");
#endif
  
  if(!pOutFile){
    (void)fputs("Error: failed to open file to write DPCM data for red!\n", stderr);
    (void)free(pDPCMData);
    (void)free(pPixelsYCbCrRcvr);
    (void)free(pPixelsYCbCrNech);
    (void)free(pPixelsYCbCrArith);
    (void)free(pPixelsYCbCr);
    (void)free(pPixelsRGB2);
    (void)free(pPixelsRGB);
    (void)fclose(pInFile);
    return EXIT_FAILURE;
  }

  for(i = 0; i < 255; i++){
    (void)fprintf(pOutFile, "%d\t%d\n", i, histogram[i][8]);
  }
  
  (void)fclose(pOutFile);
  
  for(j = 0; j < 3; j++){
    for(i = 1; i < iHead.height; i++){
      for(k = 1; k < iHead.width; k++){
        *(pDPCMData + (k - 1) + (i - 1)*(iHead.width - 1)) = abs((pPixelsRGB + (k - 1) + (i - 1)*iHead.width)->pixel[j] - (pPixelsRGB + k + i*iHead.width)->pixel[j]);
      }
    }
    for(i = 0; i < (iHead.width - 1)*(iHead.height - 1); i++){
      histogram[(unsigned char)*(pDPCMData + i)][j+9]++;
    }
  }

#ifndef HIDDENDAT
  pOutFile = fopen("DPCM_histogram_B_top_left.dat", "w");
#else
  pOutFile = fopen(".DPCM_histogram_B_top_left.dat", "w");
#endif

  if(!pOutFile){
    (void)fputs("Error: failed to open file to write DPCM data for blue!\n", stderr);
    (void)free(pDPCMData);
    (void)free(pPixelsYCbCrRcvr);
    (void)free(pPixelsYCbCrNech);
    (void)free(pPixelsYCbCrArith);
    (void)free(pPixelsYCbCr);
    (void)free(pPixelsRGB2);
    (void)free(pPixelsRGB);
    (void)fclose(pInFile);
    return EXIT_FAILURE;
  }

  for(i = 0; i < 255; i++){
    (void)fprintf(pOutFile, "%d\t%d\n", i, histogram[i][9]);
  }

  (void)fclose(pOutFile);

#ifndef HIDDENDAT
  pOutFile = fopen("DPCM_histogram_G_top_left.dat", "w");
#else
  pOutFile = fopen(".DPCM_histogram_G_top_left.dat", "w");
#endif
  
  if(!pOutFile){
    (void)fputs("Error: failed to open file to write DPCM data for green!\n", stderr);
    (void)free(pDPCMData);
    (void)free(pPixelsYCbCrRcvr);
    (void)free(pPixelsYCbCrNech);
    (void)free(pPixelsYCbCrArith);
    (void)free(pPixelsYCbCr);
    (void)free(pPixelsRGB2);
    (void)free(pPixelsRGB);
    (void)fclose(pInFile);
    return EXIT_FAILURE;
  }

  for(i = 0; i < 255; i++){
    (void)fprintf(pOutFile, "%d\t%d\n", i, histogram[i][10]);
  }

  (void)fclose(pOutFile);
  
#ifndef HIDDENDAT
  pOutFile = fopen("DPCM_histogram_R_top_left.dat", "w");
#else
  pOutFile = fopen(".DPCM_histogram_R_top_left.dat", "w");
#endif
  
  if(!pOutFile){
    (void)fputs("Error: failed to open file to write DPCM data for red!\n", stderr);
    (void)free(pDPCMData);
    (void)free(pPixelsYCbCrRcvr);
    (void)free(pPixelsYCbCrNech);
    (void)free(pPixelsYCbCrArith);
    (void)free(pPixelsYCbCr);
    (void)free(pPixelsRGB2);
    (void)free(pPixelsRGB);
    (void)fclose(pInFile);
    return EXIT_FAILURE;
  }

  for(i = 0; i < 255; i++){
    (void)fprintf(pOutFile, "%d\t%d\n", i, histogram[i][11]);
  }

  (void)fclose(pOutFile);
  
  for(j = 0; j < 3; j++){
    for(i = 1; i < iHead.height; i++){
      for(k = 1; k < iHead.width; k++){
        *(pDPCMData + (k - 1) + (i - 1)*(iHead.width - 1)) = abs(((double)(pPixelsRGB + (k - 1) + (i - 1)*iHead.width)->pixel[j] + (pPixelsRGB + k + (i - 1)*iHead.width)->pixel[j] + (pPixelsRGB + (k - 1) + i*iHead.width)->pixel[j])/3.0 - (pPixelsRGB + k + i*iHead.width)->pixel[j]);
      }
    }
    for(i = 0; i < (iHead.width - 1)*(iHead.height - 1); i++){
      histogram[(unsigned char)*(pDPCMData + i)][j+12]++;
    }
  }

#ifndef HIDDENDAT
  pOutFile = fopen("DPCM_histogram_B_average.dat", "w");
#else
  pOutFile = fopen(".DPCM_histogram_B_average.dat", "w");
#endif

  if(!pOutFile){
    (void)fputs("Error: failed to open file to write DPCM data for blue!\n", stderr);
    (void)free(pDPCMData);
    (void)free(pPixelsYCbCrRcvr);
    (void)free(pPixelsYCbCrNech);
    (void)free(pPixelsYCbCrArith);
    (void)free(pPixelsYCbCr);
    (void)free(pPixelsRGB2);
    (void)free(pPixelsRGB);
    (void)fclose(pInFile);
    return EXIT_FAILURE;
  }

  for(i = 0; i < 255; i++){
    (void)fprintf(pOutFile, "%d\t%d\n", i, histogram[i][12]);
  }

  (void)fclose(pOutFile);

#ifndef HIDDENDAT
  pOutFile = fopen("DPCM_histogram_G_average.dat", "w");
#else
  pOutFile = fopen(".DPCM_histogram_G_average.dat", "w");
#endif
  
  if(!pOutFile){
    (void)fputs("Error: failed to open file to write DPCM data for green!\n", stderr);
    (void)free(pDPCMData);
    (void)free(pPixelsYCbCrRcvr);
    (void)free(pPixelsYCbCrNech);
    (void)free(pPixelsYCbCrArith);
    (void)free(pPixelsYCbCr);
    (void)free(pPixelsRGB2);
    (void)free(pPixelsRGB);
    (void)fclose(pInFile);
    return EXIT_FAILURE;
  }

  for(i = 0; i < 255; i++){
    (void)fprintf(pOutFile, "%d\t%d\n", i, histogram[i][13]);
  }

  (void)fclose(pOutFile);
  
#ifndef HIDDENDAT
  pOutFile = fopen("DPCM_histogram_R_average.dat", "w");
#else
  pOutFile = fopen(".DPCM_histogram_R_average.dat", "w");
#endif
  
  if(!pOutFile){
    (void)fputs("Error: failed to open file to write DPCM data for red!\n", stderr);
    (void)free(pDPCMData);
    (void)free(pPixelsYCbCrRcvr);
    (void)free(pPixelsYCbCrNech);
    (void)free(pPixelsYCbCrArith);
    (void)free(pPixelsYCbCr);
    (void)free(pPixelsRGB2);
    (void)free(pPixelsRGB);
    (void)fclose(pInFile);
    return EXIT_FAILURE;
  }

  for(i = 0; i < 255; i++){
    (void)fprintf(pOutFile, "%d\t%d\n", i, histogram[i][14]);
  }

  (void)fclose(pOutFile);

  for(j = 0; j < 3; j++){
    for(i = 1; i < iHead.height; i++){
      for(k = 1; k < iHead.width; k++){
        *(pDPCMData + (k - 1) + (i - 1)*(iHead.width - 1)) = abs((pPixelsYCbCr + k + i*iHead.width - 1)->pixel[j] - (pPixelsYCbCr + k + i*iHead.width)->pixel[j]);
      }
    }
    for(i = 0; i < (iHead.width - 1)*(iHead.height - 1); i++){
      histogram[(unsigned char)*(pDPCMData + i)][j+18]++;
    }
  }

#ifndef HIDDENDAT
  pOutFile = fopen("DPCM_histogram_Y.dat", "w");
#else
  pOutFile = fopen(".DPCM_histogram_Y.dat", "w");
#endif

  if(!pOutFile){
    (void)fputs("Error: failed to open file to write DPCM data for Y!\n", stderr);
    (void)free(pDPCMData);
    (void)free(pPixelsYCbCrRcvr);
    (void)free(pPixelsYCbCrNech);
    (void)free(pPixelsYCbCrArith);
    (void)free(pPixelsYCbCr);
    (void)free(pPixelsRGB2);
    (void)free(pPixelsRGB);
    (void)fclose(pInFile);
    return EXIT_FAILURE;
  }

  for(i = 0; i < 255; i++){
    (void)fprintf(pOutFile, "%d\t%d\n", i, histogram[i][18]);
  }

  (void)fclose(pOutFile);

#ifndef HIDDENDAT
  pOutFile = fopen("DPCM_histogram_Cb.dat", "w");
#else
  pOutFile = fopen(".DPCM_histogram_Cb.dat", "w");
#endif
  
  if(!pOutFile){
    (void)fputs("Error: failed to open file to write DPCM data for Cb!\n", stderr);
    (void)free(pDPCMData);
    (void)free(pPixelsYCbCrRcvr);
    (void)free(pPixelsYCbCrNech);
    (void)free(pPixelsYCbCrArith);
    (void)free(pPixelsYCbCr);
    (void)free(pPixelsRGB2);
    (void)free(pPixelsRGB);
    (void)fclose(pInFile);
    return EXIT_FAILURE;
  }

  for(i = 0; i < 255; i++){
    (void)fprintf(pOutFile, "%d\t%d\n", i, histogram[i][19]);
  }

  (void)fclose(pOutFile);
  
#ifndef HIDDENDAT
  pOutFile = fopen("DPCM_histogram_Cr.dat", "w");
#else
  pOutFile = fopen(".DPCM_histogram_Cr.dat", "w");
#endif
  
  if(!pOutFile){
    (void)fputs("Error: failed to open file to write DPCM data for Cr!\n", stderr);
    (void)free(pDPCMData);
    (void)free(pPixelsYCbCrRcvr);
    (void)free(pPixelsYCbCrNech);
    (void)free(pPixelsYCbCrArith);
    (void)free(pPixelsYCbCr);
    (void)free(pPixelsRGB2);
    (void)free(pPixelsRGB);
    (void)fclose(pInFile);
    return EXIT_FAILURE;
  }

  for(i = 0; i < 255; i++){
    (void)fprintf(pOutFile, "%d\t%d\n", i, histogram[i][20]);
  }
  
  (void)fclose(pOutFile);
  
for(j = 0; j < 3; j++){
    for(i = 1; i < iHead.height; i++){
      for(k = 1; k < iHead.width; k++){
        *(pDPCMData + (k - 1) + (i - 1)*(iHead.width - 1)) = abs((pPixelsYCbCr + k + (i - 1)*iHead.width)->pixel[j] - (pPixelsYCbCr + k + i*iHead.width)->pixel[j]);
      }
    }
    for(i = 0; i < (iHead.width - 1)*(iHead.height - 1); i++){
      histogram[(unsigned char)*(pDPCMData + i)][j+21]++;
    }
  }

#ifndef HIDDENDAT
  pOutFile = fopen("DPCM_histogram_Y_top.dat", "w");
#else
  pOutFile = fopen(".DPCM_histogram_Y_top.dat", "w");
#endif

  if(!pOutFile){
    (void)fputs("Error: failed to open file to write DPCM data for Y!\n", stderr);
    (void)free(pDPCMData);
    (void)free(pPixelsYCbCrRcvr);
    (void)free(pPixelsYCbCrNech);
    (void)free(pPixelsYCbCrArith);
    (void)free(pPixelsYCbCr);
    (void)free(pPixelsRGB2);
    (void)free(pPixelsRGB);
    (void)fclose(pInFile);
    return EXIT_FAILURE;
  }

  for(i = 0; i < 255; i++){
    (void)fprintf(pOutFile, "%d\t%d\n", i, histogram[i][21]);
  }

  (void)fclose(pOutFile);

#ifndef HIDDENDAT
  pOutFile = fopen("DPCM_histogram_Cb_top.dat", "w");
#else
  pOutFile = fopen(".DPCM_histogram_Cb_top.dat", "w");
#endif
  
  if(!pOutFile){
    (void)fputs("Error: failed to open file to write DPCM data for Cb!\n", stderr);
    (void)free(pDPCMData);
    (void)free(pPixelsYCbCrRcvr);
    (void)free(pPixelsYCbCrNech);
    (void)free(pPixelsYCbCrArith);
    (void)free(pPixelsYCbCr);
    (void)free(pPixelsRGB2);
    (void)free(pPixelsRGB);
    (void)fclose(pInFile);
    return EXIT_FAILURE;
  }

  for(i = 0; i < 255; i++){
    (void)fprintf(pOutFile, "%d\t%d\n", i, histogram[i][22]);
  }

  (void)fclose(pOutFile);
  
#ifndef HIDDENDAT
  pOutFile = fopen("DPCM_histogram_Cr_top.dat", "w");
#else
  pOutFile = fopen(".DPCM_histogram_Cr_top.dat", "w");
#endif
  
  if(!pOutFile){
    (void)fputs("Error: failed to open file to write DPCM data for Cr!\n", stderr);
    (void)free(pDPCMData);
    (void)free(pPixelsYCbCrRcvr);
    (void)free(pPixelsYCbCrNech);
    (void)free(pPixelsYCbCrArith);
    (void)free(pPixelsYCbCr);
    (void)free(pPixelsRGB2);
    (void)free(pPixelsRGB);
    (void)fclose(pInFile);
    return EXIT_FAILURE;
  }

  for(i = 0; i < 255; i++){
    (void)fprintf(pOutFile, "%d\t%d\n", i, histogram[i][23]);
  }
  
  (void)fclose(pOutFile);
  
  for(j = 0; j < 3; j++){
    for(i = 1; i < iHead.height; i++){
      for(k = 1; k < iHead.width; k++){
        *(pDPCMData + (k - 1) + (i - 1)*(iHead.width - 1)) = abs((pPixelsYCbCr + (k - 1) + (i - 1)*iHead.width)->pixel[j] - (pPixelsYCbCr + k + i*iHead.width)->pixel[j]);
      }
    }
    for(i = 0; i < (iHead.width - 1)*(iHead.height - 1); i++){
      histogram[(unsigned char)*(pDPCMData + i)][j+24]++;
    }
  }

#ifndef HIDDENDAT
  pOutFile = fopen("DPCM_histogram_Y_top_left.dat", "w");
#else
  pOutFile = fopen(".DPCM_histogram_Y_top_left.dat", "w");
#endif

  if(!pOutFile){
    (void)fputs("Error: failed to open file to write DPCM data for Y!\n", stderr);
    (void)free(pDPCMData);
    (void)free(pPixelsYCbCrRcvr);
    (void)free(pPixelsYCbCrNech);
    (void)free(pPixelsYCbCrArith);
    (void)free(pPixelsYCbCr);
    (void)free(pPixelsRGB2);
    (void)free(pPixelsRGB);
    (void)fclose(pInFile);
    return EXIT_FAILURE;
  }

  for(i = 0; i < 255; i++){
    (void)fprintf(pOutFile, "%d\t%d\n", i, histogram[i][24]);
  }

  (void)fclose(pOutFile);

#ifndef HIDDENDAT
  pOutFile = fopen("DPCM_histogram_Cb_top_left.dat", "w");
#else
  pOutFile = fopen(".DPCM_histogram_Cb_top_left.dat", "w");
#endif
  
  if(!pOutFile){
    (void)fputs("Error: failed to open file to write DPCM data for Cb!\n", stderr);
    (void)free(pDPCMData);
    (void)free(pPixelsYCbCrRcvr);
    (void)free(pPixelsYCbCrNech);
    (void)free(pPixelsYCbCrArith);
    (void)free(pPixelsYCbCr);
    (void)free(pPixelsRGB2);
    (void)free(pPixelsRGB);
    (void)fclose(pInFile);
    return EXIT_FAILURE;
  }

  for(i = 0; i < 255; i++){
    (void)fprintf(pOutFile, "%d\t%d\n", i, histogram[i][25]);
  }

  (void)fclose(pOutFile);
  
#ifndef HIDDENDAT
  pOutFile = fopen("DPCM_histogram_Cr_top_left.dat", "w");
#else
  pOutFile = fopen(".DPCM_histogram_Cr_top_left.dat", "w");
#endif
  
  if(!pOutFile){
    (void)fputs("Error: failed to open file to write DPCM data for Cr!\n", stderr);
    (void)free(pDPCMData);
    (void)free(pPixelsYCbCrRcvr);
    (void)free(pPixelsYCbCrNech);
    (void)free(pPixelsYCbCrArith);
    (void)free(pPixelsYCbCr);
    (void)free(pPixelsRGB2);
    (void)free(pPixelsRGB);
    (void)fclose(pInFile);
    return EXIT_FAILURE;
  }

  for(i = 0; i < 255; i++){
    (void)fprintf(pOutFile, "%d\t%d\n", i, histogram[i][26]);
  }

  (void)fclose(pOutFile);
  
  for(j = 0; j < 3; j++){
    for(i = 1; i < iHead.height; i++){
      for(k = 1; k < iHead.width; k++){
        *(pDPCMData + (k - 1) + (i - 1)*(iHead.width - 1)) = abs(((double)(pPixelsYCbCr + (k - 1) + (i - 1)*iHead.width)->pixel[j] + (pPixelsYCbCr + k + (i - 1)*iHead.width)->pixel[j] + (pPixelsYCbCr + (k - 1) + i*iHead.width)->pixel[j])/3.0 - (pPixelsYCbCr + k + i*iHead.width)->pixel[j]);
      }
    }
    for(i = 0; i < (iHead.width - 1)*(iHead.height - 1); i++){
      histogram[(unsigned char)*(pDPCMData + i)][j+27]++;
    }
  }

#ifndef HIDDENDAT
  pOutFile = fopen("DPCM_histogram_Y_average.dat", "w");
#else
  pOutFile = fopen(".DPCM_histogram_Y_average.dat", "w");
#endif

  if(!pOutFile){
    (void)fputs("Error: failed to open file to write DPCM data for Y!\n", stderr);
    (void)free(pDPCMData);
    (void)free(pPixelsYCbCrRcvr);
    (void)free(pPixelsYCbCrNech);
    (void)free(pPixelsYCbCrArith);
    (void)free(pPixelsYCbCr);
    (void)free(pPixelsRGB2);
    (void)free(pPixelsRGB);
    (void)fclose(pInFile);
    return EXIT_FAILURE;
  }

  for(i = 0; i < 255; i++){
    (void)fprintf(pOutFile, "%d\t%d\n", i, histogram[i][27]);
  }

  (void)fclose(pOutFile);

#ifndef HIDDENDAT
  pOutFile = fopen("DPCM_histogram_Cb_average.dat", "w");
#else
  pOutFile = fopen(".DPCM_histogram_Cb_average.dat", "w");
#endif
  
  if(!pOutFile){
    (void)fputs("Error: failed to open file to write DPCM data for Cb!\n", stderr);
    (void)free(pDPCMData);
    (void)free(pPixelsYCbCrRcvr);
    (void)free(pPixelsYCbCrNech);
    (void)free(pPixelsYCbCrArith);
    (void)free(pPixelsYCbCr);
    (void)free(pPixelsRGB2);
    (void)free(pPixelsRGB);
    (void)fclose(pInFile);
    return EXIT_FAILURE;
  }

  for(i = 0; i < 255; i++){
    (void)fprintf(pOutFile, "%d\t%d\n", i, histogram[i][28]);
  }

  (void)fclose(pOutFile);
  
#ifndef HIDDENDAT
  pOutFile = fopen("DPCM_histogram_Cr_average.dat", "w");
#else
  pOutFile = fopen(".DPCM_histogram_Cr_average.dat", "w");
#endif
  
  if(!pOutFile){
    (void)fputs("Error: failed to open file to write DPCM data for Cr!\n", stderr);
    (void)free(pDPCMData);
    (void)free(pPixelsYCbCrRcvr);
    (void)free(pPixelsYCbCrNech);
    (void)free(pPixelsYCbCrArith);
    (void)free(pPixelsYCbCr);
    (void)free(pPixelsRGB2);
    (void)free(pPixelsRGB);
    (void)fclose(pInFile);
    return EXIT_FAILURE;
  }

  for(i = 0; i < 255; i++){
    (void)fprintf(pOutFile, "%d\t%d\n", i, histogram[i][29]);
  }

  for(i = 0; i < 12; i++){
    for(j = 0; j < 255; j++){
      if(histogram[j][i + 3] != 0){
        H[i + 3] -= (((double)histogram[j][i + 3]/((double)iHead.width * iHead.height)) * log2((double)histogram[j][i + 3]/(double)(iHead.width * iHead.height)));
      }
    }
  }

  for(i = 0; i < 12; i++){
    for(j = 0; j < 255; j++){
      if(histogram[j][i + 18] != 0){
        H[i + 18] -= (((double)histogram[j][i + 18]/((double)iHead.width * iHead.height)) * log2((double)histogram[j][i + 18]/(double)(iHead.width * iHead.height)));
      }
    }
  }

  (void)printf("%-75s%f||\n","||Bits for compressing the DPCM for left\033[34m blue\033[37m component: ", H[3]);
  (void)printf("%-75s%f||\n","||Bits for compressing the DPCM for left\033[32m green\033[37m component: ", H[4]);
  (void)printf("%-75s%f||\n","||Bits for compressing the DPCM for left\033[31m red\033[37m component: ", H[5]);
  (void)printf("%-75s%f||\n","||Bits for compressing the DPCM for top\033[34m blue\033[37m component: ", H[6]);
  (void)printf("%-75s%f||\n","||Bits for compressing the DPCM for top\033[32m green\033[37m component: ", H[7]);
  (void)printf("%-75s%f||\n","||Bits for compressing the DPCM for top\033[31m red\033[37m component: ", H[8]);
  (void)printf("%-75s%f||\n","||Bits for compressing the DPCM for top left\033[34m blue\033[37m component: ", H[9]);
  (void)printf("%-75s%f||\n","||Bits for compressing the DPCM for top left\033[32m green\033[37m component: ", H[10]);
  (void)printf("%-75s%f||\n","||Bits for compressing the DPCM for top left\033[31m red\033[37m component: ", H[11]);
  (void)printf("%-75s%f||\n","||Bits for compressing the DPCM for average\033[34m blue\033[37m component: ", H[12]);
  (void)printf("%-75s%f||\n","||Bits for compressing the DPCM for average\033[32m green\033[37m component: ", H[13]);
  (void)printf("%-75s%f||\n","||Bits for compressing the DPCM for average\033[31m red\033[37m component: ", H[14]);

  (void)printf("%-65s%f||\n","||Bits for compressing the DPCM for left Y component: ", H[15]);
  (void)printf("%-65s%f||\n","||Bits for compressing the DPCM for left Cb component: ", H[16]);
  (void)printf("%-65s%f||\n","||Bits for compressing the DPCM for left Cr component: ", H[17]);
  (void)printf("%-65s%f||\n","||Bits for compressing the DPCM for top Y component: ", H[18]);
  (void)printf("%-65s%f||\n","||Bits for compressing the DPCM for top Cb component: ", H[19]);
  (void)printf("%-65s%f||\n","||Bits for compressing the DPCM for top Cr component: ", H[20]);
  (void)printf("%-65s%f||\n","||Bits for compressing the DPCM for top left Y component: ", H[21]);
  (void)printf("%-65s%f||\n","||Bits for compressing the DPCM for top left Cb component: ", H[22]);
  (void)printf("%-65s%f||\n","||Bits for compressing the DPCM for top left Cr component: ", H[23]);
  (void)printf("%-65s%f||\n","||Bits for compressing the DPCM for top left Y component: ", H[24]);
  (void)printf("%-65s%f||\n","||Bits for compressing the DPCM for top left Cb component: ", H[25]);
  (void)printf("%-65s%f||\n","||Bits for compressing the DPCM for top left Cr component: ", H[26]);
  (void)printf("%-65s%f||\n","||Bits for compressing the DPCM for top left Y component: ", H[27]);
  (void)printf("%-65s%f||\n","||Bits for compressing the DPCM for top left Cb component: ", H[28]);
  (void)printf("%-65s%f||\n","||Bits for compressing the DPCM for top left Cr component: ", H[29]);
  (void)puts("#=========================================================================#");

  (void)free(pDPCMData);
  (void)free(pPixelsYCbCrRcvr);
  (void)free(pPixelsYCbCrNech);
  (void)free(pPixelsYCbCrArith);
  (void)free(pPixelsYCbCr);
  (void)free(pPixelsRGB2);
  (void)free(pPixelsRGB);
  (void)fclose(pInFile);
  (void)fclose(pOutFile);
  return EXIT_SUCCESS;
}
