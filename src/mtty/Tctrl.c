/******************************************************************************/
/*                                                                            */
/* src/mtty/Tctrl.c                                                           */
/*                                                                 2020/08/11 */
/* Copyright (C) 2020 Mochi.                                                  */
/*                                                                            */
/******************************************************************************/
/******************************************************************************/
/* インクルード                                                               */
/******************************************************************************/
/* 標準ヘッダ */
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

/* ライブラリヘッダ */
#include <libmvfs.h>
#include <MLib/MLibSpin.h>

/* 共通ヘッダ */
#include "Bufmng.h"
#include "Dctrl.h"
#include "Debug.h"
#include "Sessmng.h"
#include "Tctrl.h"


/******************************************************************************/
/* ローカル関数宣言                                                           */
/******************************************************************************/
/* ターミナルファイル読込み(排他制御処理) */
static void DoVfsRead( MttyDevID_t id,
                       uint32_t    globalFD,
                       uint64_t    readIdx,
                       size_t      size      );
/* VfsRead応答送信 */
static void SendVfsReadResp( uint32_t globalFD,
                             uint32_t result,
                             uint32_t ready,
                             void     *pBuffer,
                             size_t   size      );
/* VfsReady通知送信 */
static void SendVfsReadyNtc( MttyDevID_t id,
                             uint32_t    ready );
/* VfsWrite応答送信 */
static void SendVfsWriteResp( uint32_t globalFD,
                              uint32_t result,
                              uint32_t ready,
                              size_t   size      );


/******************************************************************************/
/* 静的グローバル変数                                                         */
/******************************************************************************/
/* デバイスID毎ターミナルファイルパス */
const static char *gpPath[ MTTY_DEVID_NUM ] =
    { MTTY_TTYPATH_SERIAL1,
      MTTY_TTYPATH_SERIAL2  };

/* スピンロック */
static MLibSpin_t gLock[ MTTY_DEVID_NUM ];

/* レディ通知状態 */
static uint32_t gReady[ MTTY_DEVID_NUM ];


/******************************************************************************/
/* グローバル関数定義                                                         */
/******************************************************************************/
/******************************************************************************/
/**
 * @brief       ターミナルファイル読込み
 * @details     引数globalFDに対応する読込み用バッファからデータを取り出して
 *              VfsRead要求の応答を行う。
 *
 * @param[in]   globalFD グローバルファイルディスクリプタ
 * @param[in]   readIdx  読込みインデックス(未使用)
 * @param[in]   size     読込みサイズ
 */
/******************************************************************************/
void TctrlDoVfsRead( uint32_t globalFD,
                     uint64_t readIdx,
                     size_t   size      )
{
    MttyDevID_t id;         /* デバイスID   */

    /* 初期化 */
    id = MTTY_DEVID_NULL;

    /* デバイスID変換 */
    id = SessmngConvertToDevID( globalFD );

    /* 変換結果判定 */
    if ( id == MTTY_DEVID_NULL ) {
        /* 無効 */

        DEBUG_LOG_ERR( "Invalid globalFD: fd=%u", globalFD );

        /* VfsRead応答(失敗) */
        SendVfsReadResp( globalFD, LIBMVFS_RET_FAILURE, 0, NULL, 0 );

        return;
    }

    /* スピンロック */
    ( void ) MLibSpinLock( &( gLock[ id ] ), NULL );

    /* ターミナルファイル読込み */
    DoVfsRead( id, globalFD, readIdx, size );

    /* スピンアンロック */
    ( void ) MLibSpinUnlock( &( gLock[ id ] ), NULL );

    return;
}


/******************************************************************************/
/**
 * @brief       ターミナルファイル書込み
 * @details     引数globalFDに対応するデバイスファイルにデータを書込み、
 *              VfsWrite要求の応答を行う。
 *
 * @param[in]   globalFD グローバルファイルディスクリプタ
 * @param[in]   writeIdx 書込みインデックス(未使用)
 * @param[in]   *pBuffer 書込みデータ
 * @param[in]   size     書込みサイズ
 */
/******************************************************************************/
void TctrlDoVfsWrite( uint32_t globalFD,
                      uint64_t writeIdx,
                      void     *pBuffer,
                      size_t   size      )
{
    size_t      writeSize;  /* 書込みサイズ */
    MttyDevID_t id;         /* デバイスID   */

    /* 初期化 */
    writeSize = 0;
    id        = MTTY_DEVID_NULL;

    /* デバイスID変換 */
    id = SessmngConvertToDevID( globalFD );

    /* 変換結果判定 */
    if ( id == MTTY_DEVID_NULL ) {
        /* 無効 */

        DEBUG_LOG_ERR( "Invalid globalFD: fd=%u", globalFD );

        /* VfsWrite応答(失敗) */
        SendVfsWriteResp( globalFD,
                          LIBMVFS_RET_FAILURE,
                          MVFS_READY_WRITE,
                          0                    );

        return;
    }

    /* デバイスファイル書込み */
    writeSize = DctrlWrite( id, pBuffer, size );

    /* 追加サイズ判定 */
    if ( writeSize == size ) {
        /* 書込みサイズと一致 */

        /* VfsWrite応答(成功) */
        SendVfsWriteResp( globalFD,
                          LIBMVFS_RET_SUCCESS,
                          MVFS_READY_WRITE,
                          writeSize            );

    } else {
        /* 書込みサイズと不一致 */

        /* VfsWrite応答(成功) */
        SendVfsWriteResp( globalFD,
                          LIBMVFS_RET_SUCCESS,
                          MVFS_READY_WRITE,
                          writeSize            );
    }

    return;
}


/******************************************************************************/
/**
 * @brief       ターミナル制御初期化
 * @details     スピンロックと読込みレディ通知状態を初期化する。
 */
/******************************************************************************/
void TctrlInit( void )
{
    /* スピンロック初期化 */
    MLibSpinInit( &( gLock[ MTTY_DEVID_SERIAL1 ] ), NULL );
    MLibSpinInit( &( gLock[ MTTY_DEVID_SERIAL2 ] ), NULL );

    /* 読込みレディ通知状態初期化 */
    gReady[ MTTY_DEVID_SERIAL1 ] = MVFS_READY_WRITE;
    gReady[ MTTY_DEVID_SERIAL2 ] = MVFS_READY_WRITE;

    return;
}


/******************************************************************************/
/**
 * @brief       読込みレディ
 * @details     読込みレディ通知状態が未通知の場合は読込みレディ状態を通知する。
 *
 * @param[in]   id デバイスID
 */
/******************************************************************************/
void TctrlReadyRead( MttyDevID_t id )
{
    /* スピンロック */
    ( void ) MLibSpinLock( &( gLock[ id ] ), NULL );

    /* 読込みレディ通知状態判定 */
    if ( ( gReady[ id ] & MVFS_READY_READ ) == 0 ) {
        /* 未通知 */

        /* レディ通知状態設定 */
        gReady[ id ] |= MVFS_READY_READ;

        /* VfsReady通知 */
        SendVfsReadyNtc( id, gReady[ id ] );
    }

    /* スピンアンロック */
    ( void ) MLibSpinUnlock( &( gLock[ id ] ), NULL );

    return;
}


/******************************************************************************/
/* ローカル関数定義                                                           */
/******************************************************************************/
/******************************************************************************/
/**
 * @brief       ターミナルファイル読込み(排他制御処理)
 * @details     バッファからデータを取り出し、VfsRead応答を行う。
 *
 * @param[in]   id       デバイスID
 * @param[in]   globalFD グローバルファイルディスクリプタ
 * @param[in]   readIdx  読込みインデックス(未使用)
 * @param[in]   size     読込みサイズ
 */
/******************************************************************************/
static void DoVfsRead( MttyDevID_t id,
                       uint32_t    globalFD,
                       uint64_t    readIdx,
                       size_t      size      )
{
    char   *pData;  /* データ       */
    size_t popSize; /* 取出しサイズ */

    /* 初期化 */
    pData   = NULL;
    popSize = 0;

    /* データ領域割当て */
    pData = malloc( size );

    /* 割当て結果判定 */
    if ( pData == NULL ) {
        /* 失敗 */

        DEBUG_LOG_ERR( "malloc(): fd=%u, size=%u", globalFD, size );

        /* VfsRead応答(失敗) */
        SendVfsReadResp( globalFD, LIBMVFS_RET_FAILURE, 0, NULL, 0 );

        /* 読込みレディ通知状態更新 */
        gReady[ id ] &= ~MVFS_READY_READ;

        return;
    }

    /* バッファ取出し */
    popSize = BufmngPopForRead( id, pData, size );

    /* 取出しサイズ判定 */
    if ( popSize == size ) {
        /* 読込みサイズ */

        /* VfsRead応答(成功) */
        SendVfsReadResp( globalFD,
                         LIBMVFS_RET_SUCCESS,
                         MVFS_READY_READ,
                         pData,
                         popSize              );

        /* 読込みレディ通知状態更新 */
        gReady[ id ] |= MVFS_READY_READ;

    } else if ( popSize != 0 ) {
        /* 1以上 */

        /* VfsRead応答(成功) */
        SendVfsReadResp( globalFD, LIBMVFS_RET_SUCCESS, 0, pData, popSize );

        /* 読込みレディ通知状態更新 */
        gReady[ id ] &= ~MVFS_READY_READ;

    } else {
        /* 0 */

        /* VfsRead応答(失敗) */
        SendVfsReadResp( globalFD, LIBMVFS_RET_FAILURE, 0, NULL, 0 );

        /* 読込みレディ通知状態更新 */
        gReady[ id ] &= ~MVFS_READY_READ;

    }

    /* データ領域解放 */
    free( pData );

    return;
}


/******************************************************************************/
/**
 * @brief       VfsRead応答送信
 * @details     VfsRead応答を送信する。
 *
 * @param[in]   globalFD グローバルファイルディスクリプタ
 * @param[in]   result   処理結果
 *                  - LIBMVFS_RET_SUCCESS 処理成功
 *                  - LIBMVFS_RET_FAILURE 処理失敗
 * @param[in]   ready    読込みレディ状態
 *                  - 0                  非レディ
 *                  - LIBMVFS_READY_READ レディ
 * @param[in]   *pBuffer 読込みバッファ
 * @param[in]   size     読込みサイズ
 */
/******************************************************************************/
static void SendVfsReadResp( uint32_t globalFD,
                             uint32_t result,
                             uint32_t ready,
                             void     *pBuffer,
                             size_t   size      )
{
    LibMvfsErr_t errLibMvfs;    /* LibMvfs関数エラー要因 */
    LibMvfsRet_t retLibMvfs;    /* LibMvfs関数戻り値     */

    /* 初期化 */
    errLibMvfs = LIBMVFS_ERR_NONE;
    retLibMvfs = LIBMVFS_RET_FAILURE;

    /* VfsRead応答送信 */
    retLibMvfs = LibMvfsSendVfsReadResp( globalFD,
                                         result,
                                         ready,
                                         pBuffer,
                                         size,
                                         &errLibMvfs );

    /* 送信結果判定 */
    if ( retLibMvfs != LIBMVFS_RET_SUCCESS ) {
        /* 失敗 */

        DEBUG_LOG_ERR(
            "LibMvfsSendVfsReadResp(): ret=%u, err=%x",
            retLibMvfs,
            errLibMvfs
        );
    }

    return;
}


/******************************************************************************/
/**
 * @brief       VfsReady通知送信
 * @details     VfsReady通知を送信する。
 *
 * @param[in]   id デバイスID
 *                  - MTTY_DEVID_SERIAL1 シリアルポート1
 *                  - MTTY_DEVID_SERIAL2 シリアルポート2
 * @param[in]   ready レディ状態
 *                  - MVFS_READY_READ  読込みレディ
 *                  - MVFS_READY_WRITE 書込みレディ
 */
/******************************************************************************/
static void SendVfsReadyNtc( MttyDevID_t id,
                             uint32_t    ready )
{
    LibMvfsErr_t errLibMvfs;    /* LibMvfs関数エラー要因 */
    LibMvfsRet_t retLibMvfs;    /* LibMvfs関数戻り値     */

    /* 初期化 */
    errLibMvfs = LIBMVFS_ERR_NONE;
    retLibMvfs = LIBMVFS_RET_FAILURE;

    /* VfsReady通知 */
    retLibMvfs = LibMvfsSendVfsReadyNtc( gpPath[ id ], ready, &errLibMvfs );

    /* 通知結果判定 */
    if ( retLibMvfs != LIBMVFS_RET_SUCCESS ) {
        /* 失敗 */

        DEBUG_LOG_ERR(
            "LibMvfsSendReadyNtc(): ret=%u, err=%x",
            retLibMvfs,
            errLibMvfs
        );
    }

    return;
}


/******************************************************************************/
/**
 * @brief       VfsWrite応答送信
 * @details     VfsWrite応答を送信する。
 *
 * @param[in]   globalFD グローバルファイルディスクリプタ
 * @param[in]   result   処理結果
 *                  - LIBMVFS_RET_SUCCESS 処理成功
 *                  - LIBMVFS_RET_FAILURE 処理失敗
 * @param[in]   ready    書込みレディ状態
 *                  - 0                   非レディ
 *                  - LIBMVFS_READY_WRITE レディ
 * @param[in]   size     書込みサイズ
 */
/******************************************************************************/
static void SendVfsWriteResp( uint32_t globalFD,
                              uint32_t result,
                              uint32_t ready,
                              size_t   size      )
{
    LibMvfsErr_t errLibMvfs;    /* LibMvfs関数エラー要因 */
    LibMvfsRet_t retLibMvfs;    /* LibMvfs関数戻り値     */

    /* 初期化 */
    errLibMvfs = LIBMVFS_ERR_NONE;
    retLibMvfs = LIBMVFS_RET_FAILURE;

    /* VfsWrite応答送信 */
    retLibMvfs = LibMvfsSendVfsWriteResp( globalFD,
                                          result,
                                          ready,
                                          size,
                                          &errLibMvfs );

    /* 送信結果判定 */
    if ( retLibMvfs != LIBMVFS_RET_SUCCESS ) {
        /* 失敗 */

        DEBUG_LOG_ERR(
            "LibMvfsSendVfsWriteResp(): ret=%u, err=%x",
            retLibMvfs,
            errLibMvfs
        );
    }

    return;
}


/******************************************************************************/
