/******************************************************************************/
/*                                                                            */
/* src/mtty/Dctrl.c                                                           */
/*                                                                 2020/05/06 */
/* Copyright (C) 2020 Mochi.                                                  */
/*                                                                            */
/******************************************************************************/
/******************************************************************************/
/* インクルード                                                               */
/******************************************************************************/
/* 標準ヘッダ */
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

/* ライブラリヘッダ */
#include <libmvfs.h>

/* 共通ヘッダ */
#include "Buffer.h"
#include "Dctrl.h"
#include "Debug.h"
#include "Echo.h"


/******************************************************************************/
/* 定義                                                                       */
/******************************************************************************/
/** 読込みサイズ */
#define READ_SIZE ( 512 )

/** 書込みサイズ */
#define WRITE_SIZE ( 512 )


/******************************************************************************/
/* グローバル関数定義                                                         */
/******************************************************************************/
/******************************************************************************/
/**
 * @brief       デバイスファイル読込み
 * @details     デバイスからデータを読込み、バッファにデータを追加する。
 *
 * @param[in]   fd ファイルディスクリプタ
 * @param[in]   id デバイスID
 *                  - MTTY_DEVID_SERIAL1 シリアルポート1
 *                  - MTTY_DEVID_SERIAL2 シリアルポート2
 */
/******************************************************************************/
void DctrlDoRead( uint32_t    fd,
                  MttyDevId_t id  )
{
    char         buffer[ READ_SIZE ];   /* バッファ              */
    size_t       size;                  /* サイズ                */
    LibMvfsErr_t errLibMvfs;            /* LibMvfs関数エラー要因 */
    LibMvfsRet_t retLibMvfs;            /* LibMvfs関数戻り値     */

    /* 初期化 */
    memset( buffer, 0, READ_SIZE );
    size       = 0;
    errLibMvfs = LIBMVFS_ERR_NONE;
    retLibMvfs = LIBMVFS_RET_FAILURE;

    while ( true ) {
        /* デバイスファイル読込 */
        retLibMvfs = LibMvfsRead( fd, buffer, READ_SIZE, &size, &errLibMvfs );

        /* 読込みサイズ判定 */
        if ( size == 0 ) {
            /* 0 */

            break;
        }

        /* エコー */
        EchoDo( fd, id, buffer, size );

        /* バッファ追加 */
        BufferPushForRead( id, &buffer, size );
    }

    return;
}


/******************************************************************************/
/**
 * @brief       デバイスファイル書込み
 * @details     バッファからデータを取り出し、デバイスにデータを書き込む。
 *
 * @param[in]   fd ファイルディスクリプタ
 * @param[in]   id デバイスID
 *                  - MTTY_DEVID_SERIAL1 シリアルポート1
 *                  - MTTY_DEVID_SERIAL2 シリアルポート2
 */
/******************************************************************************/
void DctrlDoWrite( uint32_t    fd,
                   MttyDevId_t id  )
{
    char         buffer[ WRITE_SIZE ];  /* 書込みバッファ        */
    size_t       writeSize;             /* 書込みサイズ          */
    size_t       retSize;               /* 書込み結果サイズ      */
    LibMvfsErr_t errLibMvfs;            /* LibMvfs関数エラー要因 */
    LibMvfsRet_t retLibMvfs;            /* LibMvfs関数戻り値     */

    /* 初期化 */
    memset( buffer, 0, WRITE_SIZE );
    writeSize = 0;
    retSize   = 0;
    errLibMvfs = LIBMVFS_ERR_NONE;
    retLibMvfs = LIBMVFS_RET_FAILURE;

    /* バッファ取出し */
    writeSize = BufferPopForWrite( id, buffer, WRITE_SIZE );

    /* 取出し結果判定 */
    if ( writeSize != 0 ) {
        /* 取出し成功 */

        retLibMvfs = LibMvfsWrite( fd,
                                   buffer,
                                   writeSize,
                                   &retSize,
                                   &errLibMvfs );

        /* 書込み結果 */
        if ( ( retLibMvfs != LIBMVFS_RET_SUCCESS ) ||
             ( writeSize  != retSize             )    ) {
            /* 失敗 */

            DEBUG_LOG_ERR(
                "LibMvfsWrite(): wsize=%u, ret=%u, err=%x, rsize=%u",
                writeSize,
                retLibMvfs,
                errLibMvfs,
                retSize
            );
        }
    }

    return;
}


/******************************************************************************/
