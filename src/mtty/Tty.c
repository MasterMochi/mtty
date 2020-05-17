/******************************************************************************/
/*                                                                            */
/* src/mtty/Tty.c                                                             */
/*                                                                 2020/05/08 */
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

/* 共通ヘッダ */
#include "Buffer.h"
#include "Debug.h"
#include "Sess.h"
#include "Tty.h"


/******************************************************************************/
/* ローカル関数宣言                                                           */
/******************************************************************************/
/* VfsRead応答送信 */
static void SendVfsReadResp( uint32_t globalFd,
                             uint32_t result,
                             uint32_t ready,
                             void     *pBuffer,
                             size_t   size      );
/* VfsWrite応答送信 */
static void SendVfsWriteResp( uint32_t globalFd,
                              uint32_t result,
                              uint32_t ready,
                              size_t   size      );


/******************************************************************************/
/* グローバル関数定義                                                         */
/******************************************************************************/
/******************************************************************************/
/**
 * @brief       ttyファイル読込み
 * @details     引数globalFdに対応する読込み用バッファからデータを取り出して
 *              VfsRead要求の応答を行う。
 *
 * @param[in]   globalFd グローバルファイルディスクリプタ
 * @param[in]   readIdx  読込みインデックス(未使用)
 * @param[in]   size     読込みサイズ
 */
/******************************************************************************/
void TtyDoVfsRead( uint32_t globalFd,
                   uint64_t readIdx,
                   size_t   size      )
{
    char        *pData;     /* データ       */
    size_t      popSize;    /* 取出しサイズ */
    MttyDevId_t id;         /* デバイスID   */

    /* 初期化 */
    pData   = NULL;
    popSize = 0;
    id      = MTTY_DEVID_NULL;

    /* デバイスID変換 */
    id = SessConvertFdToId( globalFd );

    /* 変換結果判定 */
    if ( id == MTTY_DEVID_NULL ) {
        /* 無効 */

        DEBUG_LOG_ERR( "Invalid globalFd: fd=%u", globalFd );

        /* VfsRead応答(失敗) */
        SendVfsReadResp( globalFd, LIBMVFS_RET_FAILURE, 0, NULL, 0 );

        return;
    }

    /* データ領域割当て */
    pData = malloc( size );

    /* 割当て結果判定 */
    if ( pData == NULL ) {
        /* 失敗 */

        DEBUG_LOG_ERR( "malloc(): fd=%u, size=%u", globalFd, size );

        return;
    }

    /* バッファ取出し */
    popSize = BufferPopForRead( id, pData, size );

    /* 取出しサイズ判定 */
    if ( popSize == size ) {
        /* 読込みサイズ */

        /* VfsRead応答(成功) */
        SendVfsReadResp( globalFd,
                         LIBMVFS_RET_SUCCESS,
                         MVFS_READY_READ,
                         pData,
                         popSize              );

    } else if ( popSize != 0 ) {
        /* 1以上 */

        /* VfsRead応答(成功) */
        SendVfsReadResp( globalFd, LIBMVFS_RET_SUCCESS, 0, pData, popSize );

    } else {
        /* 0 */

        /* VfsRead応答(失敗) */
        SendVfsReadResp( globalFd, LIBMVFS_RET_FAILURE, 0, NULL, 0 );
    }

    /* データ領域解放 */
    free( pData );

    return;
}


/******************************************************************************/
/**
 * @brief       ttyファイル書込み
 * @details     引数globalFdに対応する書込み用バッファにデータを追加して
 *              VfsWrite要求の応答を行う。
 *
 * @param[in]   globalFd グローバルファイルディスクリプタ
 * @param[in]   writeIdx 書込みインデックス(未使用)
 * @param[in]   *pBuffer 書込みデータ
 * @param[in]   size     書込みサイズ
 */
/******************************************************************************/
void TtyDoVfsWrite( uint32_t globalFd,
                    uint64_t writeIdx,
                    void     *pBuffer,
                    size_t   size      )
{
    size_t      pushSize;   /* バッファ追加サイズ */
    MttyDevId_t id;         /* デバイスID         */

    /* 初期化 */
    pushSize = 0;
    id       = MTTY_DEVID_NULL;

    /* デバイスID変換 */
    id = SessConvertFdToId( globalFd );

    /* 変換結果判定 */
    if ( id == MTTY_DEVID_NULL ) {
        /* 無効 */

        DEBUG_LOG_ERR( "Invalid globalFd: fd=%u", globalFd );

        /* VfsWrite応答(失敗) */
        SendVfsWriteResp( globalFd, LIBMVFS_RET_FAILURE, 0, 0 );
    }

    /* 書込み用バッファ追加 */
    pushSize = BufferPushForWrite( id, pBuffer, size );

    /* 追加サイズ判定 */
    if ( pushSize == size ) {
        /* 書込みサイズと一致 */

        /* VfsWrite応答(成功) */
        SendVfsWriteResp( globalFd,
                          LIBMVFS_RET_SUCCESS,
                          MVFS_READY_WRITE,
                          pushSize             );

    } else {
        /* 書込みサイズと不一致 */

        /* VfsWrite応答(成功) */
        SendVfsWriteResp( globalFd, LIBMVFS_RET_FAILURE, 0, pushSize );

    }

    return;
}


/******************************************************************************/
/* ローカル関数定義                                                           */
/******************************************************************************/
/******************************************************************************/
/**
 * @brief       VfsRead応答送信
 * @details     VfsRead応答を送信する。
 *
 * @param[in]   globalFd グローバルファイルディスクリプタ
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
static void SendVfsReadResp( uint32_t globalFd,
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
    retLibMvfs = LibMvfsSendVfsReadResp( globalFd,
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
 * @brief       VfsWrite応答送信
 * @details     VfsWrite応答を送信する。
 *
 * @param[in]   globalFd グローバルファイルディスクリプタ
 * @param[in]   result   処理結果
 *                  - LIBMVFS_RET_SUCCESS 処理成功
 *                  - LIBMVFS_RET_FAILURE 処理失敗
 * @param[in]   ready    書込みレディ状態
 *                  - 0                   非レディ
 *                  - LIBMVFS_READY_WRITE レディ
 * @param[in]   size     書込みサイズ
 */
/******************************************************************************/
static void SendVfsWriteResp( uint32_t globalFd,
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
    retLibMvfs = LibMvfsSendVfsWriteResp( globalFd,
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
