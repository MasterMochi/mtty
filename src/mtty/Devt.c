/******************************************************************************/
/*                                                                            */
/* src/mtty/Devt.c                                                            */
/*                                                                 2020/05/04 */
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
#include "Buffer.h"
#include "Dctrl.h"
#include "Debug.h"
#include "Devt.h"


/******************************************************************************/
/* ローカル関数宣言                                                           */
/******************************************************************************/
/* 書込み監視FDリスト設定 */
static void SetWriteFds( LibMvfsFds_t *pList,
                         MttyDevId_t  id      );


/******************************************************************************/
/* 静的グローバル変数                                                         */
/******************************************************************************/
/* デバイスファイルディスクリプタ */
static uint32_t gDevFd[ MTTY_DEVID_NUM ];


/******************************************************************************/
/* グローバル関数定義                                                         */
/******************************************************************************/
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
    retLibMvfs = LibMvfsOpen( &gDevFd[ MTTY_DEVID_SERIAL1 ],
                              "/serial1",
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
    retLibMvfs = LibMvfsOpen( &gDevFd[ MTTY_DEVID_SERIAL2 ],
                              "/serial2",
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
    LibMvfsFds_t writeFds;      /* 書込みFDリスト        */

    /* 初期化 */
    id         = 0;
    errLibMvfs = LIBMVFS_ERR_NONE;
    retLibMvfs = LIBMVFS_RET_FAILURE;

    while ( true ) {
        /* 初期化 */
        memset( &readFds,  0, sizeof ( LibMvfsFds_t ) );
        memset( &writeFds, 0, sizeof ( LibMvfsFds_t ) );

        /* 読込みFDリスト設定 */
        LIBMVFS_FDS_SET( &readFds, gDevFd[ MTTY_DEVID_SERIAL1 ] );
        LIBMVFS_FDS_SET( &readFds, gDevFd[ MTTY_DEVID_SERIAL2 ] );

        /* 書込みFDリスト設定 */
        SetWriteFds( &writeFds, MTTY_DEVID_SERIAL1 );
        SetWriteFds( &writeFds, MTTY_DEVID_SERIAL2 );

        /* FD監視 */
        retLibMvfs = LibMvfsSelect( &readFds,       /* 読込みFDリスト   */
                                    &writeFds,      /* 書込みFDリスト   */
                                    1000000,        /* タイムアウト時間 */
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
            if ( LIBMVFS_FDS_CHECK( &readFds, gDevFd[ id ] ) ) {
                /* 可 */

                /* デバイスファイル読込 */
                DctrlDoRead( gDevFd[ id ], id );
            }

            /* 書込み可判定 */
            if ( LIBMVFS_FDS_CHECK( &writeFds, gDevFd[ id ] ) ) {
                /* 可 */

                /* デバイスファイル書込み */
                DctrlDoWrite( gDevFd[ id ], id );
            }
        }
    }

    return;
}


/******************************************************************************/
/* ローカル関数定義                                                           */
/******************************************************************************/
/******************************************************************************/
/**
 * @brief       書込みFDリスト設定
 * @details     引数idに対応する書込み用バッファにデータが有る場合は、FDリスト
 *              にファイルディスクリプタを設定する。
 *
 * @param[in]   *pList 書込みFDリスト
 * @param[in]   id     デバイスID
 *                  - MTTY_DEVID_SERIAL1 シリアルポート1
 *                  - MTTY_DEVID_SERIAL2 シリアルポート2
 */
/******************************************************************************/
static void SetWriteFds( LibMvfsFds_t *pList,
                         MttyDevId_t  id      )
{
    bool ret;   /* データ有無 */

    /* 初期化 */
    ret = false;

    /* 書込み用バッファデータ有無判定 */
    ret = BufferCheckWrite( id );

    /* 結果判定 */
    if ( ret != false ) {
        /* データ有り */

        /* 書込みFDリスト設定 */
        LIBMVFS_FDS_SET( pList, gDevFd[ id ] );
    }

    return;
}


/******************************************************************************/
