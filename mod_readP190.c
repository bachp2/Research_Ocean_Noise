#include <stdio.h>
#include <windows.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <stdint.h>
#include "mat.h"
#include "matrix.h"

//compile in matlab
//mex -v -client engine mod_readP190.c libmat.lib libmx.lib
#define R 1000
#define C 5000
#ifdef _WIN32
    #define WINPAUSE system("pause")
#endif

typedef enum {cINT, cDOUBLE, cSTRING} dtype;

FILE *fp;
int total_lines_count = 0;
size_t linelen = 100;
int hcounter = 0;
int rcounter_row = -1;
int rcounter_col = 0;
int cgml = 0;
int groupPerShots;
double* vesselX;
double* vesselY;
double* depth;
int* julianDOY;
int* timeHMS;
double* sourceX;
double* sourceY;
double* tailX;
double* tailY;
double* receiverX;
double* receiverY;
enum processing{PREAMBLE, HEADER, RECEIVER_COL, RECEIVER_ROW};

void malloc_data_chunks(){
    vesselX = (double*)malloc(C*sizeof(double));
    vesselY = (double*)malloc(C*sizeof(double));
    depth = (double*)malloc(C*sizeof(double));

    julianDOY = (int*)malloc(C*sizeof(int));
    timeHMS = (int*)malloc(C*sizeof(int));

    sourceX = (double*)malloc(C*sizeof(double));
    sourceY = (double*)malloc(C*sizeof(double));

    tailX = (double*)malloc(C*sizeof(double));
    tailY = (double*)malloc(C*sizeof(double));

    for(size_t i = 0; i < R; ++i){
        receiverX[i] = (double*)malloc(C*sizeof(double));
        receiverY[i] = (double*)malloc(C*sizeof(double));
    }
}

void free_data_chunks(){
    free(vesselX);
    free(vesselY);
    free(depth);
    free(timeHMS);
    free(julianDOY);
    free(sourceX);
    free(sourceY);
    free(tailX);
    free(tailY);
    for(size_t i = 0; i < R; ++i){
        free(receiverX[i]);
        free(receiverY[i]);
    }
}

int getline(char** buff, size_t llen, FILE** fp){
    assert(fp != NULL);
    size_t i = 0;
    int c;
    while((c = fgetc(*fp)) != EOF){
        if(i == llen) {
            i--;
            printf("buffer overflow");
            break;
        }
        (*buff)[i++] = (char) c;
        if(c == '\n') break;
    }
    buff[i] = '\0';
    total_lines_count++;
    return c;
}

void increase_counter(enum processing p){
    switch(p){
        case PREAMBLE:
            break;
        case HEADER:
            hcounter++;
            rcounter_row = 0;
            rcounter_col++;
            break;
        case RECEIVER_ROW:
            rcounter_row++;
            break;
        case RECEIVER_COL:
            break;
    }
}

void substr(char* str, char* sub , int start, int len){
    memcpy(sub, &str[start], len);
    sub[len] = '\0';
}

void print_values_at_current_hcounter(){
    printf("day:%d, timeGMT:%d",julianDOY[hcounter],timeHMS[hcounter]);
    printf("\nvessel loc x:%f, y:%f, z:%f",vesselX[hcounter],vesselY[hcounter],depth[hcounter]);
    printf("\nsource loc x:%f, y:%f",sourceX[hcounter],sourceY[hcounter]);
    printf("\ntail loc x:%f, y:%f",tailX[hcounter],tailY[hcounter]);
}

void processHeaderLines(char* linebuff){
    char sstr[10];
    substr(linebuff, sstr, 46, 9);
    vesselX[hcounter] = atof(sstr);
    substr(linebuff, sstr, 55, 9);
    vesselY[hcounter] = atof(sstr);
    substr(linebuff, sstr, 64, 6);
    
    depth[hcounter] = atof(sstr);
    substr(linebuff, sstr, 70, 3);
    julianDOY[hcounter] = atoi(sstr);
    substr(linebuff, sstr, 73, 6);
    timeHMS[hcounter] = atoi(sstr);

    getline(&linebuff, linelen, &fp);
    substr(linebuff, sstr, 46, 9);
    sourceX[hcounter] = atof(sstr);
    substr(linebuff, sstr, 55, 9);
    sourceY[hcounter] = atof(sstr);

    if(cgml) getline(&linebuff, linelen, &fp);
    
    getline(&linebuff, linelen, &fp);
    substr(linebuff, sstr, 46, 9);
    tailX[hcounter] = atof(sstr);
    substr(linebuff, sstr, 55, 9);
    tailY[hcounter] = atof(sstr);
    //print_values_at_current_hcounter();
}

void processReceiverLine(char* linebuff){
    char sstr[10];
    double rx,ry;
    substr(linebuff, sstr, 5, 9);
    receiverX[rcounter_row][rcounter_col] = atof(sstr);
    substr(linebuff, sstr, 14, 9);
    receiverY[rcounter_row][rcounter_col] = atof(sstr);
    increase_counter(RECEIVER_ROW);
    
    substr(linebuff, sstr, 31, 9);
    receiverX[rcounter_row][rcounter_col] = atof(sstr);
    substr(linebuff, sstr, 40, 9);
    receiverY[rcounter_row][rcounter_col] = atof(sstr);
    increase_counter(RECEIVER_ROW);
    

    substr(linebuff, sstr, 57, 9);
    receiverX[rcounter_row][rcounter_col] = atof(sstr);
    substr(linebuff, sstr, 66, 9);
    receiverY[rcounter_row][rcounter_col] = atof(sstr);
    increase_counter(RECEIVER_ROW);
    //printf("\n");
}

void digest_line(char* linebuff){
    if(strstr(linebuff, "H1100") != NULL){
        //get number of groups per shot
        char groupPerShotStr[5];
        substr(linebuff, groupPerShotStr, 32,4);
        groupPerShots = atoi(groupPerShotStr);
    }
    else if (strstr(linebuff, "CMGL") != NULL) {
        cgml = 1;
    }
    else if(linebuff[0] == 'V'){
        //get Vessel information
        processHeaderLines(linebuff);
        increase_counter(HEADER);
    }
    else if(linebuff[0] == 'R'){
        //get Receiver streamers info
        processReceiverLine(linebuff);
    }
}

int write_to_mat(MATFile *pmat, void** dataptr, dtype dt, int m, int n, const char *dataname) {
    mxArray *pa1;
    int status;
    switch(dt){
    case cDOUBLE:
        pa1 = mxCreateDoubleMatrix(m,n,mxREAL);
        break;
    case cINT:
        pa1 = mxCreateNumericMatrix(m,n, mxINT32_CLASS, mxREAL);
        break;
    }

    if (pa1 == NULL) {
        printf("%s : Out of memory on line %d\n", __FILE__, __LINE__); 
        printf("Unable to create mxArray.\n");
        return(EXIT_FAILURE);
    }
    
    switch(dt){
    case cDOUBLE:
        memcpy( mxGetPr(pa1), (*dataptr), m*n*sizeof(double));
        break;
    case cINT:
        memcpy( mxGetPr(pa1), (*dataptr), m*n*sizeof(int));
        break;
    }
        
    status = matPutVariable(pmat, dataname, pa1);
    if (status != 0) {
        printf("Error using matPutVariable\n");
        return(EXIT_FAILURE);
    } 
    /* clean up before exit */
    mxDestroyArray(pa1);
    return(EXIT_SUCCESS);
}

int write_data_chunks_to_mat_file(){
    MATFile *pmat;
    char *file = "airguns_data.mat";
    
    printf("Creating mat file %s...\n\n", file);
    pmat = matOpen(file, "w");
    if (pmat == NULL) {
        printf("Error creating file %s\n", file);
        printf("(Do you have write permission in this directory?)\n");
        return(EXIT_FAILURE);
    }

    char *dataname[] = {"vesselX", "vesselY", "depth", "DOY", "HMS", "sourceX", "sourceY", "tailX", "tailY", "receiverX", "receiverY"};
    write_to_mat(pmat, &vesselX, cDOUBLE, 1, hcounter, dataname[0]);
    write_to_mat(pmat, &vesselY, cDOUBLE, 1, hcounter, dataname[1]);
    write_to_mat(pmat, &depth,   cDOUBLE, 1, hcounter, dataname[2]);
    write_to_mat(pmat, &julianDOY, cINT, 1, hcounter, dataname[3]);
    write_to_mat(pmat, &timeHMS,   cINT, 1, hcounter, dataname[4]);
    write_to_mat(pmat, &sourceX, cDOUBLE, 1, hcounter,  dataname[5]);
    write_to_mat(pmat, &sourceY, cDOUBLE, 1, hcounter, dataname[6]);
    write_to_mat(pmat, &tailX,  cDOUBLE, 1, hcounter, dataname[7]);
    write_to_mat(pmat, &tailY, cDOUBLE, 1, hcounter, dataname[8]);
    write_to_mat(pmat, &receiverX, cDOUBLE, rcounter_row, rcounter_col, dataname[9]);
    write_to_mat(pmat, &receiverY, cDOUBLE, rcounter_row, rcounter_col, dataname[10]);
    printf("Written successfully to file!\n\n");
    
    //close file
    if (matClose(pmat) != 0) {
        printf("Error closing file %s\n",file);
        return(EXIT_FAILURE);
    }
    return(EXIT_SUCCESS);
}

int main() {

    LARGE_INTEGER freq;
    LARGE_INTEGER t0, tF, tDiff;
    double elapsedTime;
    double resolution;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&t0);

    char* linebuff;
    linebuff = (char *)malloc(linelen * sizeof(char));
    malloc_data_chunks();
    fp = fopen("MGL1212MCS5.p190", "r+");
    int c;
    
    while((c = getline(&linebuff, linelen, &fp))!= EOF || c != -1){
        digest_line(linebuff);
    }

    printf("\nSUCCESS!!\nRead a total of %d lines\n", total_lines_count);
    printf("-----------------------------------------------\n");
    for(int i = 0; i < rcounter_row; ++i){
        for(int j = 0; j < rcounter_col; ++j){
            printf("%.3f ", receiverX[i][j]);
        }
        printf("\n");
    }
    //write_data_chunks_to_mat_file();
    
    fclose(fp);
    free(linebuff);
    free_data_chunks();

    QueryPerformanceCounter(&tF);
    tDiff.QuadPart = tF.QuadPart - t0.QuadPart;
    elapsedTime = tDiff.QuadPart / (double) freq.QuadPart;
    resolution = 1.0 / (double) freq.QuadPart;
    printf("Your performance counter ticks %I64u times per second\n", freq.QuadPart);
    printf("Resolution is %lf nanoseconds\n", resolution*1e9);
    printf("Code under test took %lf sec\n", elapsedTime);
    WINPAUSE;
    return 0;
}
