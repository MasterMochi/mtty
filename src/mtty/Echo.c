/******************************************************************************/
/*                                                                            */
/* src/mtty/Echo.c                                                            */
/*                                                                 2020/05/24 */
/* Copyright (C) 2020 Mochi.                                                  */
/*                                                                            */
/******************************************************************************/
/******************************************************************************/
/* インクルード                                                               */
/******************************************************************************/
/* 標準ヘッダ */
#include <stddef.h>
#include <stdint.h>
#include <string.h>

/* ライブラリヘッダ */
#include <libmvfs.h>

/* 共通ヘッダ */
#include "Debug.h"
#include "Echo.h"


/******************************************************************************/
/* 静的ローカル変数定義                                                       */
/******************************************************************************/
/** 変換テーブル */
static const char *pgConvertTbl[] =
    { "^@",   "^A",   "^B",   "^C", "^D",  "^E",   "^F",   "^G",
      "\x08", "\x09", "\x0A", "^K", "^L",  "\x0D", "^N",   "^O",
      "^P",   "^Q",   "^R",   "^S", "^T",  "^U",   "^V",   "^W",
      "^X",   "^Y",   "^Z",   "^[", "^\\", "^]",   "\x1E", "^_"  };


/******************************************************************************/
/* グローバル関数定義                                                         */
/******************************************************************************/
/******************************************************************************/
/**
 * @brief       エコー
 * @details     データをデバイスファイルに書き込む。データが制御コードの場合は
 *              文字変換を行った後に書き込む。
 *
 * @param[in]   fd    ファイルディスクリプタ
 * @param[in]   id    デバイスID
 *                  - MTTY_DEVID_SERIAL1 シリアルポート1
 *                  - MTTY_DEVID_SERIAL2 シリアルポート2
 * @param[in]   *pData データ
 * @param[in]   size   データサイズ
 */
/******************************************************************************/
void EchoDo( uint32_t    fd,
             MttyDevId_t id,
             char        *pData,
             size_t      size    )
{
    char         *pWrite;       /* 書込みデータ          */
    size_t       writeSize;     /* 書込みサイズ          */
    uint32_t     idx;           /* インデックス          */
    LibMvfsErr_t errLibMvfs;    /* LibMvfs関数エラー要因 */
    LibMvfsRet_t retLibMvfs;    /* LibMvfs関数戻り値     */

    /* 初期化 */
    pWrite     = NULL;
    writeSize  = 0;
    idx        = 0;
    errLibMvfs = LIBMVFS_ERR_NONE;
    retLibMvfs = LIBMVFS_RET_FAILURE;

    /* 1バイト毎にサイズ分繰り返す */
    for ( idx = 0; idx < size; idx++ ) {
        /* 制御コード判定 */
        if ( ( 0 <= pData[ idx ] ) && ( pData[ idx ] <= 0x1F ) ) {
            /* 制御コード */

            pWrite    = ( char * ) pgConvertTbl[ pData[ idx ] ];
            writeSize = strlen( pWrite );

        } else {
            /* 非制御コード */

            pWrite    = &( pData[ idx ] );
            writeSize = 1;
        }

        /* デバイスファイル書込み */
        retLibMvfs = LibMvfsWrite( fd, pWrite, writeSize, NULL, &errLibMvfs );

        /* 書込み結果判定 */
        if ( retLibMvfs != LIBMVFS_RET_SUCCESS ) {
            /* 失敗 */

            DEBUG_LOG_ERR(
                "LibMvfsWrite(): ret=%u, err=%x",
                retLibMvfs,
                errLibMvfs
            );
        }
    }

    return;
}


/******************************************************************************/
