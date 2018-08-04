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
#define ROWS 1000
#define COLS 5000
#define PRINT_LIMIT 100
#define TOTAL_CELLS ROWS*COLS
#define INPUT_DIR "C:\\Users\\bachp2\\Documents\\MATLAB\\MGDS_Download\\MGL1212\\"
#define OUTPUT_DIR "MATFILES\\"
#ifdef _WIN32
    #define WINPAUSE system("pause")
#endif

typedef enum {cINT, cDOUBLE, cSTRING} dtype;
typedef struct{
    char* d;
    char* m;
    char* s;
}DMS;

FILE *fp;
int printline = 0;
int total_lines_count = 0;
size_t linelen = 100;
int hcounter = 0;
int record_rows, record_cols;
int rcounter_row = 0;
int rcounter_col = 0;
int cgml = 0;
int groupPerShots;
double* vesselX;
double* vesselY;
double* lat;
double* lon;
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
    vesselX = (double*)malloc(COLS*sizeof(double));
    vesselY = (double*)malloc(COLS*sizeof(double));
    depth = (double*)malloc(COLS*sizeof(double));

    lat = (double*)malloc(COLS*sizeof(double));
    lon = (double*)malloc(COLS*sizeof(double));

    julianDOY = (int*)malloc(COLS*sizeof(int));
    timeHMS = (int*)malloc(COLS*sizeof(int));

    sourceX = (double*)malloc(COLS*sizeof(double));
    sourceY = (double*)malloc(COLS*sizeof(double));

    tailX = (double*)malloc(COLS*sizeof(double));
    tailY = (double*)malloc(COLS*sizeof(double));

    receiverX = (double*)malloc(TOTAL_CELLS*sizeof(double));
    receiverY = (double*)malloc(TOTAL_CELLS*sizeof(double));

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
    free(receiverX);
    free(receiverY);
    free(lat);
    free(lon);
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
            break;
        case RECEIVER_ROW:
            rcounter_row++;
            break;
        case RECEIVER_COL:
            record_rows = rcounter_row;
            record_cols = rcounter_col;
            rcounter_row = 0;
            rcounter_col++;
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
double dms2decimaldegrees(char* d, char* m, char* s)
{
    double mdeg = atof(d);
    double mmin = atof(m);
    double msec = atof(s);
    return mdeg + mmin/60 + msec/3600;
}

void processHeaderLines(char* linebuff){
    char sstr[15];
    char d[5],m[5],s[10];
    substr(linebuff, d, 25, 2);
    //printf(d);
    substr(linebuff, m, 27, 2);
    substr(linebuff, s, 29, 6);
    lat[hcounter] = dms2decimaldegrees(d,m,s);
    substr(linebuff, d, 35, 3);
    substr(linebuff, m, 38, 2);
    substr(linebuff, s, 40, 6);
    lon[hcounter] = -1*dms2decimaldegrees(d,m,s);

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
    substr(linebuff, sstr, 5, 9);
    //receiverX[rcounter_row][rcounter_col] = atof(sstr);
    *(receiverX + rcounter_col*ROWS + rcounter_row) = atof(sstr);
    substr(linebuff, sstr, 14, 9);
    *(receiverY + rcounter_col*ROWS + rcounter_row) = atof(sstr);
    increase_counter(RECEIVER_ROW);
    
    substr(linebuff, sstr, 31, 9);
    *(receiverX + rcounter_col*ROWS + rcounter_row) = atof(sstr);
    substr(linebuff, sstr, 40, 9);
    *(receiverY + rcounter_col*ROWS + rcounter_row) = atof(sstr);
    increase_counter(RECEIVER_ROW);
    
    substr(linebuff, sstr, 57, 9);
    *(receiverX + rcounter_col*ROWS + rcounter_row) = atof(sstr);
    substr(linebuff, sstr, 66, 9);
    *(receiverY + rcounter_col*ROWS + rcounter_row) = atof(sstr);
    increase_counter(RECEIVER_ROW);
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
        if(rcounter_row == groupPerShots) increase_counter(RECEIVER_COL);
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
        for(size_t i = 0; i < m; ++i){
            memcpy( ( (double *) mxGetPr(pa1) ) + i*n,  ( (double *)*dataptr ) + i*COLS, n*sizeof(double));
        }
        break;
    case cINT:
        for(size_t i = 0; i < m; ++i){
            memcpy( ( (int *) mxGetPr(pa1) ) + i*n,  ( (int *)*dataptr ) + i*COLS, n*sizeof(int));
        }
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

int write_data_chunks_to_mat_file(char *file){
    MATFile *pmat;
    //char *file = "MATFILES\\airguns_data.mat";
    
    printf("Creating mat file %s...\n\n", file);
    pmat = matOpen(file, "w");
    if (pmat == NULL) {
        printf("Error creating file %s\n", file);
        printf("(Do you have write permission in this directory?)\n");
        return(EXIT_FAILURE);
    }

    char *dataname[] = {"vesselX", "vesselY", "depth", "DOY", "HMS", "sourceX", "sourceY", "tailX", "tailY", "receiverX", "receiverY","lat", "lon"};
    write_to_mat(pmat, &vesselX, cDOUBLE, 1, hcounter, dataname[0]);
    write_to_mat(pmat, &vesselY, cDOUBLE, 1, hcounter, dataname[1]);
    write_to_mat(pmat, &depth,   cDOUBLE, 1, hcounter, dataname[2]);
    write_to_mat(pmat, &julianDOY, cINT, 1, hcounter, dataname[3]);
    write_to_mat(pmat, &timeHMS,   cINT, 1, hcounter, dataname[4]);
    write_to_mat(pmat, &sourceX, cDOUBLE, 1, hcounter,  dataname[5]);
    write_to_mat(pmat, &sourceY, cDOUBLE, 1, hcounter, dataname[6]);
    write_to_mat(pmat, &tailX,  cDOUBLE, 1, hcounter, dataname[7]);
    write_to_mat(pmat, &tailY, cDOUBLE, 1, hcounter, dataname[8]);
    write_to_mat(pmat, &lat,  cDOUBLE, 1, hcounter, dataname[11]);
    write_to_mat(pmat, &lon, cDOUBLE, 1, hcounter, dataname[12]);
    //printf("%d\n",record_cols);
    write_to_mat(pmat, &receiverX, cDOUBLE, ROWS, COLS, dataname[9]);
    write_to_mat(pmat, &receiverY, cDOUBLE, ROWS, COLS, dataname[10]);
    printf("Written successfully to file!\n\n");
    
    //close file
    if (matClose(pmat) != 0) {
        printf("Error closing file %s\n",file);
        return(EXIT_FAILURE);
    }
    return(EXIT_SUCCESS);
}

char** str_split(char* a_str, const char a_delim)
{
    char** result    = 0;
    size_t count     = 0;
    char* tmp        = a_str;
    char* last_comma = 0;
    char delim[2];
    delim[0] = a_delim;
    delim[1] = 0;

    /* Count how many elements will be extracted. */
    while (*tmp)
    {
        if (a_delim == *tmp)
        {
            count++;
            last_comma = tmp;
        }
        tmp++;
    }

    /* Add space for trailing token. */
    count += last_comma < (a_str + strlen(a_str) - 1);

    /* Add space for terminating null string so caller
       knows where the list of returned strings ends. */
    count++;

    result = malloc(sizeof(char*) * count);

    if (result)
    {
        size_t idx  = 0;
        char* token = strtok(a_str, delim);

        while (token)
        {
            assert(idx < count);
            *(result + idx++) = strdup(token);
            token = strtok(0, delim);
        }
        assert(idx == count - 1);
        *(result + idx) = 0;
    }

    return result;
}

int files_num = 16;
static const char* files_out[16] = {"MGL1212MCS1.mat",
                                "MGL1212MCS2.mat",
                                "MGL1212MCS3.mat",
                                "MGL1212MCS4.mat",
                                "MGL1212MCS5.mat",
                                "MGL1212MCS6.mat",
                                "MGL1212MCS7.mat",
                                "MGL1212MCS8.mat",
                                "MGL1212MCS9.mat",
                                "MGL1212MCS10.mat",
                                "MGL1212MCS11.mat",
                                "MGL1212MCS12.mat",
                                "MGL1212MCS13.mat",
                                "MGL1212MCS14.mat",
                                "MGL1212MCS15.mat",
                                "MGL1212MCS16.mat"};

static const char* files_in[16] = {"MGL1212MCS1.p190",
                                "MGL1212MCS2.p190",
                                "MGL1212MCS3.p190",
                                "MGL1212MCS4.p190",
                                "MGL1212MCS5.p190",
                                "MGL1212MCS6.p190",
                                "MGL1212MCS7.p190",
                                "MGL1212MCS8.p190",
                                "MGL1212MCS9.p190",
                                "MGL1212MCS10.p190",
                                "MGL1212MCS11.p190",
                                "MGL1212MCS12.p190",
                                "MGL1212MCS13.p190",
                                "MGL1212MCS14.p190",
                                "MGL1212MCS15.p190",
                                "MGL1212MCS16.p190"};
void reset_vars()
{
    total_lines_count = 0;
    linelen = 100;
    hcounter = 0;
    record_rows, record_cols;
    rcounter_row = 0;
    rcounter_col = 0;
    cgml = 0;
    groupPerShots = 0;
}

int main() {

    LARGE_INTEGER freq;
    LARGE_INTEGER t0, tF, tDiff;
    double elapsedTime;
    double resolution;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&t0);
    char fnbuf[256];
    for(int i = 0; i < files_num; ++i)
    {
        snprintf(fnbuf, sizeof fnbuf, "%s%s", INPUT_DIR, files_in[i]);
        char* linebuff;
        linebuff = (char *)malloc(linelen * sizeof(char));
        malloc_data_chunks();
        fp = fopen(fnbuf, "r+");
        int c;
        
        while((c = getline(&linebuff, linelen, &fp))!= EOF || c != -1){
            digest_line(linebuff);
        }
        record_cols++;//to fix off by one errror
        printf("\nRead a total of %d lines\n", total_lines_count);

        snprintf(fnbuf, sizeof fnbuf, "%s%s", OUTPUT_DIR, files_out[i]);
        write_data_chunks_to_mat_file(fnbuf);
        printf("Writing to text file...\n");
        fclose(fp);
        free(linebuff);
        free_data_chunks();
        reset_vars();
    }
    
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
