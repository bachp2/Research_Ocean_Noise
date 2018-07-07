#include <stdio.h>
#include <windows.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <stdint.h>
#include "mat.h"
#define R 1000
#define C 5000

FILE *fp;
int total_lines_count = 0;
size_t linelen = 100;
int hcounter;
int rcounter_row;
int rcounter_col;
int cgml = 0;
int groupPerShots;
double* vesselX;
double* vesselY;
double* depth;
int* julianDOY;
int* time;
double* sourceX;
double* sourceY;
double* tailX;
double* tailY;
double* receiverX[R];
double* receiverY[R];
enum processing{PREAMBLE, HEADER, RECEIVER_COL, RECEIVER_ROW};

void malloc_data_chunks(){
    vesselX = (double*)malloc(C*sizeof(double));
    vesselY = (double*)malloc(C*sizeof(double));
    depth = (double*)malloc(C*sizeof(double));

    julianDOY = (int*)malloc(C*sizeof(int));
    time = (int*)malloc(C*sizeof(int));

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
    free(time);
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
            rcounter_row++;
            rcounter_col = 0;
            break;
        case RECEIVER_ROW:
            //something's seriously wrong with my logic
            break;
        case RECEIVER_COL:
            rcounter_col++;
            break;
    }
}

void substr(char* str, char* sub , int start, int len){
    memcpy(sub, &str[start], len);
    sub[len] = '\0';
}

void print_values_at_current_hcounter(){
    printf("day:%d, timeGMT:%d",julianDOY[hcounter],time[hcounter]);
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
    time[hcounter] = atoi(sstr);

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
    receiverX[rcounter_col][rcounter_row] = atof(sstr);
    substr(linebuff, sstr, 14, 9);
    receiverY[rcounter_col][rcounter_row] = atof(sstr);
    rx = receiverX[rcounter_col][rcounter_row];
    ry = receiverY[rcounter_col][rcounter_row];
    //printf("\nreceiver loc x:%f, y:%f",rx,ry);
    increase_counter(RECEIVER_COL);
    
    substr(linebuff, sstr, 31, 9);
    receiverX[rcounter_col][rcounter_row] = atof(sstr);
    substr(linebuff, sstr, 40, 9);
    receiverY[rcounter_col][rcounter_row] = atof(sstr);
    rx = receiverX[rcounter_col][rcounter_row];
    ry = receiverY[rcounter_col][rcounter_row];
    //printf("\nreceiver loc x:%f, y:%f",rx,ry);
    increase_counter(RECEIVER_COL);
    

    substr(linebuff, sstr, 57, 9);
    receiverX[rcounter_col][rcounter_row] = atof(sstr);
    substr(linebuff, sstr, 66, 9);
    receiverY[rcounter_col][rcounter_row] = atof(sstr);
    rx = receiverX[rcounter_col][rcounter_row];
    ry = receiverY[rcounter_col][rcounter_row];
    //printf("\nreceiver loc x:%f, y:%f",rx,ry);
    increase_counter(RECEIVER_COL);
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
        increase_counter(RECEIVER_ROW);
    }
}
MATFile write_to_mat(double** dataptr){

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
    //digest last line !!BUG!! last line is EOF
    //digesting the buffer will lead to null pointer access violation
    //digest_line(linebuff);

    printf("\nSUCCESS!!\nRead a total of %d lines\n", total_lines_count);
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
    printf("-----------------------------------------------");
    return 0;
}
