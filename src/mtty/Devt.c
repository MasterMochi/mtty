/******************************************************************************/
/*                                                                            */
/* src/mtty/Devt.c                                                            */
/*                                                                 2020/08/11 */
/* Copyright (C) 2020 Mochi.                                                  */
/*                                                                            */
/******************************************************************************/
/******************************************************************************/
/* インクルード                                                               */
/******************************************************************************/
/* 標準ヘッダ */
#include <stdbool.h>
#include <string.h>

/* ライブラリヘッダ */
#include <libmvfs.h>

/* 共通ヘッダ */
#include "mtty.h"
#include "Bufmng.h"
#include "Dctrl.h"
#include "Debug.h"
#include "Devt.h"


/******************************************************************************/
/* 静的グローバル変数                                                         */
/******************************************************************************/
/* デバイスファイルディスクリプタ */
uint32_t gDevFD[ MTTY_DEVID_NUM ];


/******************************************************************************/
/* グローバル関数定義                                                         */
/******************************************************************************/
/******************************************************************************/
/**
 * @brief       デバイスファイルディスクリプタ変換
 * @details     デバイスIDからデバイスファイルディスクリプタに変換する。
 *
 * @param[in]   id デバイスID
 *                  - MTTY_DEVID_SERIAL1 シリアルポート1
 *                  - MTTY_DEVID_SERIAL2 シリアルポート2
 *
 * @return      デバイスファイルディスクリプタを返す。
 */
/******************************************************************************/
uint32_t DevtConvertToFD( MttyDevID_t id )
{
    return gDevFD[ id ];
}


/******************************************************************************/
/**
 * @brief       デバイスイベント初期化
 * @details     デバイスファイルを開く。
 */
/******************************************************************************/
void DevtInit( void )
{
    LibMvfsErr_t errLibMvfs;    /* LibMvfs関数エラー要因 */
    LibMvfsRet_t retLibMvfs;    /* LibMvfs関数戻り値     */

    /* 初期化 */
    errLibMvfs = LIBMVFS_ERR_NONE;
    retLibMvfs = LIBMVFS_RET_FAILURE;

    /* シリアルポート1オープン */
    retLibMvfs = LibMvfsOpen( &gDevFD[ MTTY_DEVID_SERIAL1 ],
                              MTTY_DEVPATH_SERIAL1,
                              &errLibMvfs                    );

    /* オープン結果判定 */
    if ( retLibMvfs != LIBMVFS_RET_SUCCESS ) {
        /* 失敗 */

        DEBUG_LOG_ERR(
            "LibMvfsOpen(): ret=%d, err=%x",
            retLibMvfs,
            errLibMvfs
        );
    }

    /* シリアルポート2オープン */
    retLibMvfs = LibMvfsOpen( &gDevFD[ MTTY_DEVID_SERIAL2 ],
                              MTTY_DEVPATH_SERIAL2,
                              &errLibMvfs                    );

    /* オープン結果判定 */
    if ( retLibMvfs != LIBMVFS_RET_SUCCESS ) {
        /* 失敗 */

        DEBUG_LOG_ERR(
            "LibMvfsOpen(): ret=%d, err=%x",
            retLibMvfs,
            errLibMvfs
        );
    }

    return;
}


/******************************************************************************/
/**
 * @brief       制御開始
 * @details     デバイスファイルを監視し、イベントを発行する。
 */
/******************************************************************************/
void DevtStart( void )
{
    uint32_t     id;            /* デバイスID            */
    LibMvfsErr_t errLibMvfs;    /* LibMvfs関数エラー要因 */
    LibMvfsRet_t retLibMvfs;    /* LibMvfs関数戻り値     */
    LibMvfsFds_t readFds;       /* 読込みFDリスト        */

    /* 初期化 */
    id         = 0;
    errLibMvfs = LIBMVFS_ERR_NONE;
    retLibMvfs = LIBMVFS_RET_FAILURE;

    while ( true ) {
        /* 初期化 */
        memset( &readFds,  0, sizeof ( LibMvfsFds_t ) );

        /* 読込みFDリスト設定 */
        LIBMVFS_FDS_SET( &readFds, gDevFD[ MTTY_DEVID_SERIAL1 ] );
        LIBMVFS_FDS_SET( &readFds, gDevFD[ MTTY_DEVID_SERIAL2 ] );

        /* FD監視 */
        retLibMvfs = LibMvfsSelect( &readFds,       /* 読込みFDリスト   */
                                    NULL,           /* 書込みFDリスト   */
                                    0,              /* タイムアウト時間 */
                                    &errLibMvfs );  /* エラー要因       */

        /* 監視結果判定 */
        if ( retLibMvfs != LIBMVFS_RET_SUCCESS ) {
            /* 失敗 */

            /* エラー要因判定 */
            if ( errLibMvfs != LIBMVFS_ERR_TIMEOUT ) {
                /* タイムアウト以外 */

                DEBUG_LOG_ERR(
                    "LibMvfsSelect(): ret=%u, err=%x",
                    retLibMvfs,
                    errLibMvfs
                );
            }

            continue;
        }

        for ( id = 0; id < MTTY_DEVID_NUM; id++ ) {
            /* 読込み可判定 */
            if ( LIBMVFS_FDS_CHECK( &readFds, gDevFD[ id ] ) ) {
                /* 可 */

                /* デバイスファイル読込 */
                DctrlRead( gDevFD[ id ], id );
            }
        }
    }

    return;
}


/******************************************************************************/
