/******************************************************************************/
/*                                                                            */
/* src/mtty/Dctrl.c                                                           */
/*                                                                 2020/08/11 */
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
#include "config.h"
#include "Bufmng.h"
#include "Dctrl.h"
#include "Devt.h"
#include "Debug.h"
#include "Echo.h"
#include "Tctrl.h"


/******************************************************************************/
/* グローバル関数定義                                                         */
/******************************************************************************/
/******************************************************************************/
/**
 * @brief       デバイスファイル読込み
 * @details     デバイスからデータを読込み、バッファにデータを追加する。また、
 *              読込みレディとエコーを行う。
 *
 * @param[in]   fd ファイルディスクリプタ
 * @param[in]   id デバイスID
 *                  - MTTY_DEVID_SERIAL1 シリアルポート1
 *                  - MTTY_DEVID_SERIAL2 シリアルポート2
 */
/******************************************************************************/
void DctrlRead( uint32_t    fd,
                MttyDevID_t id  )
{
    char         buffer[ CONFIG_SIZE_READ ];    /* バッファ              */
    size_t       size;                          /* サイズ                */
    LibMvfsErr_t errLibMvfs;                    /* LibMvfs関数エラー要因 */
    LibMvfsRet_t retLibMvfs;                    /* LibMvfs関数戻り値     */

    /* 初期化 */
    memset( buffer, 0, CONFIG_SIZE_READ );
    size       = 0;
    errLibMvfs = LIBMVFS_ERR_NONE;
    retLibMvfs = LIBMVFS_RET_FAILURE;

    while ( true ) {
        /* デバイスファイル読込 */
        retLibMvfs =
            LibMvfsRead( fd,                    /* ファイルディスクリプタ */
                         buffer,                /* 読込みバッファ         */
                         CONFIG_SIZE_READ,      /* 読込みバッファサイズ   */
                         &size,                 /* 読込みサイズ           */
                         &errLibMvfs       );   /* エラー要因             */

        /* 読込みサイズ判定 */
        if ( size == 0 ) {
            /* 0 */

            break;
        }

        /* バッファ追加 */
        BufmngPushForRead( id, &buffer, size );

        /* 読込みレディ */
        TctrlReadyRead( id );

        /* エコー */
        EchoDo( fd, id, buffer, size );
    }

    return;
}


/******************************************************************************/
/**
 * @brief       デバイスファイル書込み
 * @details     バッファからデータを取り出し、デバイスにデータを書き込む。
 *
 * @param[in]   id     デバイスID
 *                  - MTTY_DEVID_SERIAL1 シリアルポート1
 *                  - MTTY_DEVID_SERIAL2 シリアルポート2
 * @param[in]   *pData データ
 * @param[in]   size   サイズ
 */
/******************************************************************************/
size_t DctrlWrite( MttyDevID_t id,
                   void        *pData,
                   size_t      size    )
{
    size_t       retSize;               /* 書込み結果サイズ       */
    uint32_t     fd;                    /* ファイルディスクリプタ */
    LibMvfsErr_t errLibMvfs;            /* LibMvfs関数エラー要因  */
    LibMvfsRet_t retLibMvfs;            /* LibMvfs関数戻り値      */

    /* 初期化 */
    retSize    = 0;
    fd         = 0;
    errLibMvfs = LIBMVFS_ERR_NONE;
    retLibMvfs = LIBMVFS_RET_FAILURE;

    /* デバイスファイルFD変換 */
    fd = DevtConvertToFD( id );

    /* 書込みサイズ判定 */
    if ( size != 0 ) {

        /* 書込み */
        retLibMvfs =
            LibMvfsWrite( fd,               /* ファイルディスクリプタ */
                          pData,            /* 書込みデータ           */
                          size,             /* 書込みデータサイズ     */
                          &retSize,         /* 書込みサイズ           */
                          &errLibMvfs );    /* エラー要因             */

        /* 書込み結果 */
        if ( ( retLibMvfs != LIBMVFS_RET_SUCCESS ) ||
             ( retSize    != size                )    ) {
            /* 失敗 */

            DEBUG_LOG_ERR(
                "LibMvfsWrite(): wsize=%u, ret=%u, err=%x, rsize=%u",
                size,
                retLibMvfs,
                errLibMvfs,
                retSize
            );
        }
    }

    return retSize;
}


/******************************************************************************/
