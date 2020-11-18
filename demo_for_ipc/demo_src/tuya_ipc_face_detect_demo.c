/*********************************************************************************
  *Copyright(C),2015-2020, 
  *TUYA 
  *www.tuya.comm
  *FileName:    tuya_ipc_face_detect_demo
**********************************************************************************/

#include <stdio.h>
#include <dirent.h>
#include <unistd.h>
#include <stdlib.h>
#include "tuya_ipc_ai_face_detect.h"
#include "tuya_ipc_ai_face_db.h"

#define TYDEBUG printf
#define TYERROR printf

//According to different chip platforms, users need to implement the int erface of capture.
void gen_new_face_message(TUYA_AI_FACE_UPLOAD_PARA* p_new_face)
{
    // use demo file to simulate
    if(p_new_face == NULL)
    {
        return;
    }
    char snapfile[128];
    p_new_face->face_jpeg_data_size = 0;
    extern char s_raw_path[];
    printf("get new face snapshot\n");
    snprintf(snapfile,64,"%s/resource/media/demo_face_snapshot.jpg",s_raw_path);
    FILE*fp_face = fopen(snapfile,"r+");
    if(NULL == fp_face)
    {
        printf("fail to open snap.jpg\n");
        return;
    }
    fseek(fp_face,0,SEEK_END);
    p_new_face->face_jpeg_data_size = ftell(fp_face);
    if(p_new_face->face_jpeg_data_size < 100*1024)
    {
        fseek(fp_face,0,SEEK_SET);
        fread(p_new_face->face_jpeg_data,p_new_face->face_jpeg_data_size,1,fp_face);
    }
    fclose(fp_face);

    snprintf(snapfile,64,"%s/resource/media/demo_scence_snapshot.jpg",s_raw_path);
    FILE*fp_scence = fopen(snapfile,"r+");
    if(NULL == fp_scence)
    {
        printf("fail to open snap.jpg\n");
        return;
    }
    fseek(fp_scence,0,SEEK_END);
    p_new_face->face_jpeg_data_size = ftell(fp_scence);
    if(p_new_face->face_jpeg_data_size < 100*1024)
    {
        fseek(fp_scence,0,SEEK_SET);
        fread(p_new_face->face_jpeg_data,p_new_face->face_jpeg_data_size,1,fp_scence);
    }
    fclose(fp_scence);

    p_new_face->face_score = 80; //you should set the score
    return;
}

//According to different chip platforms, users need to implement the interface of capture.
void gen_signed_face_message(TUYA_AI_FACE_UPLOAD_PARA* p_signed_face)
{
    //we use file to simulate
    if(p_signed_face == NULL)
    {
        return;
    }
    char snapfile[128];
    p_signed_face->face_jpeg_data_size = 0;
    p_signed_face->scene_jpeg_data_size = 0;
    extern char s_raw_path[];
    printf("get signed face snapshot\n");
    snprintf(snapfile,64,"%s/resource/media/demo_signed_face.jpg",s_raw_path);
    FILE*fp_face = fopen(snapfile,"r+");
    if(NULL == fp_face)
    {
        printf("fail to open snap.jpg\n");
        return;
    }
    fseek(fp_face,0,SEEK_END);
    p_signed_face->face_jpeg_data_size = ftell(fp_face);
    if(p_signed_face->face_jpeg_data_size < 100*1024)
    {
        fseek(fp_face,0,SEEK_SET);
        fread(p_signed_face->face_jpeg_data,p_signed_face->face_jpeg_data_size,1,fp_face);
    }
    fclose(fp_face);

    snprintf(snapfile,64,"%s/resource/media/demo_signed_scence.jpg",s_raw_path);
    FILE*fp_scence = fopen(snapfile,"r+");
    if(NULL == fp_scence)
    {
        printf("fail to open snap.jpg\n");
        return;
    }
    fseek(fp_scence,0,SEEK_END);
    p_signed_face->scene_jpeg_data_size = ftell(fp_scence);
    if(p_signed_face->scene_jpeg_data_size < 100*1024)
    {
        fseek(fp_scence,0,SEEK_SET);
        fread(p_signed_face->scene_jpeg_data,p_signed_face->scene_jpeg_data_size,1,fp_scence);
    }
    fclose(fp_scence);

    p_signed_face->face_id = 10000;  //you should set your faceid
    return;
}

void tycam_ai_face_download_pic_cb(OUT PVOID_T callback_data)
{
    TUYA_AI_FACE_DATA *data= (TUYA_AI_FACE_DATA*)callback_data;
    TYDEBUG("face_id = %d \n" ,data->face_id);
    /*save or send it to AI detect base face_download_para*/
    /*
    FILE* pfile = fopen(base, "wb");
    if (!pfile) {
        PR_ERR("open audio file fail!\n");
    }
    if(pfile != NULL) {
        fwrite(face_download_para->pic_src, 1, face_download_para->pic_len, pfile);
        fflush(pfile);
        fclose(pfile);
        pfile = NULL;
    }
    */
}


/* face_id_list: [1,2,3,4,5] */
void tycam_ai_face_cfg_func(IN CONST CHAR_T* action, IN CONST CHAR_T* face_id_list)
{
    TYDEBUG("tycam_ai_face_cfg_func\n");
    if((action == NULL) || (face_id_list == NULL))
    {
        return ;
    }
    else
    {
        if(strcmp(action,"add") == 0)
        {
            tuya_ipc_ai_face_update_cloud_face();
        }
        else if(strcmp(action,"delete") == 0)
        {
            /* delete local face data within face_id_list*/
        }
        else if(strcmp(action,"update") == 0)
        {
            tuya_ipc_ai_face_update_cloud_face();
        }
    }
    return ;
}

int get_frame_yuv_data(char **fram_yuv)
{
    /*get frame yuv data*/
    return 0;
}

BOOL_T send_and_detect_face(char *yuv)
{   BOOL_T ret =0;
    /*if it has face set 1,nor set 0*/
    return ret;
}


BOOL_T compare_face_is_strange(char *yuv)
{   BOOL_T ret =0;
    /*if it is strange set 1,nor set 0*/
    return ret;
}

int save_data_base_id(unsigned int face_id)
{
    /*save the date base face id*/
    return 0;
}



/*init callback to TUYA SDK*/
void tycam_face_sdk_func_init(void)
{
    tuya_ipc_ai_face_detect_storage_init(tycam_ai_face_cfg_func,tycam_ai_face_download_pic_cb); 

    int db_len=513*4;
    tuya_ipc_ai_face_db_init("/home/canjian/face",db_len);

    tuya_ipc_ai_face_update_cloud_face();
    

    CHAR_T* pFace_id ="542779";
    CHAR_T* pData;
    pData=(CHAR_T*)malloc(db_len);


    char base[256]="/home/canjian/face1/542778";
    FILE *feature_file = fopen(base, "rb");
    fread(pData, 1, db_len, feature_file);
    fclose(feature_file);

    tuya_ipc_ai_face_db_add(pFace_id, (PVOID_T)pData);

    CHAR_T* pData1;
    pData1=(CHAR_T*)malloc(db_len);
    tuya_ipc_ai_face_db_read(pFace_id, (PVOID_T)pData1);

    char base1[256]="/home/canjian/face/5427710";
    FILE *feature_file1 = fopen(base1, "wb");
    fwrite(pData1, 1, db_len, feature_file1);
    fclose(feature_file1);

    tuya_ipc_ai_face_db_delete("5427710");

    return;
}

VOID *thread_face_dectect_proc(VOID *arg)
{
    BOOL_T is_strange = 0;
    char *yuv = NULL;
    TUYA_AI_FACE_UPLOAD_PARA new_up_load_buffer={0};
    TUYA_AI_FACE_UPLOAD_PARA signed_up_load_buffer={0};
    unsigned int face_id =0;
    tycam_face_sdk_func_init();

    while (1)
    {
        usleep(100*1000);
        get_frame_yuv_data(&yuv);
        BOOL_T has_face = send_and_detect_face(yuv);
        if(has_face)
        {
            is_strange = compare_face_is_strange(yuv);
            if(is_strange)
            {
                gen_new_face_message(&new_up_load_buffer);
                tuya_ipc_ai_face_new_report(&new_up_load_buffer,&face_id);
                save_data_base_id(face_id);
            }
            else
            {
                gen_signed_face_message(&signed_up_load_buffer);
                tuya_ipc_ai_face_signed_report(&signed_up_load_buffer);
            }

        }

    }
    return NULL;
}

