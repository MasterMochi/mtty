/******************************************************************************/
/*                                                                            */
/* src/mtty/Sessmng.c                                                         */
/*                                                                 2020/08/11 */
/* Copyright (C) 2018-2020 Mochi.                                             */
/*                                                                            */
/******************************************************************************/
/******************************************************************************/
/* インクルード                                                               */
/******************************************************************************/
/* 標準ヘッダ */
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* ライブラリヘッダ */
#include <libmvfs.h>
#include <kernel/types.h>
#include <MLib/MLibList.h>

/* 共通ヘッダ */
#include "mtty.h"
#include "Debug.h"
#include "Sessmng.h"


/******************************************************************************/
/* 定義                                                                       */
/******************************************************************************/
/** セッション情報 */
typedef struct {
    MLibListNode_t node;        /**< リストノード情報                 */
    uint32_t       globalFD;    /**< グローバルファイルディスクリプタ */
    MttyDevID_t    devID;       /**< デバイスID                       */
    MkPid_t        pid;         /**< プロセスID                       */
} sessInfo_t;


/******************************************************************************/
/* ローカル関数宣言                                                           */
/******************************************************************************/
/* グローバルFD比較 */
static bool CompareGlobalFD( MLibListNode_t *pNode,
                             void           *pParam );


/******************************************************************************/
/* 静的グローバル変数定義                                                     */
/******************************************************************************/
/** セッション情報管理リスト */
static MLibList_t gSessInfoList;


/******************************************************************************/
/* グローバル関数定義                                                         */
/******************************************************************************/
/******************************************************************************/
/**
 * @brief       デバイスID変換
 * @details     オープンされているターミナルファイルのグローバルファイルディス
 *              クリプタをデバイスIDに変換する。
 *
 * @param[in]   globalFD グローバルファイルディスクリプタ
 *
 * @return      デバイスIDを返す。
 * @retval      MTTY_DEVID_NULL    無効デバイスID
 * @retval      MTTY_DEVID_SERIAL1 シリアルポート1
 * @retval      MTTY_DEVID_SERIAL2 シリアルポート2
 */
/******************************************************************************/
MttyDevID_t SessmngConvertToDevID( uint32_t globalFD )
{
    sessInfo_t *pSessInfo;  /* セッション情報 */

    /* 初期化 */
    pSessInfo = NULL;

    /* セッション情報取得 */
    pSessInfo =
        ( sessInfo_t * ) MLibListSearchHead( &gSessInfoList,
                                             &CompareGlobalFD,
                                             &globalFD,
                                             MLIB_LIST_GET     );

    /* 取得結果判定 */
    if ( pSessInfo == NULL ) {
        /* 失敗 */

        return MTTY_DEVID_NULL;
    }

    return pSessInfo->devID;
}


/******************************************************************************/
/**
 * @brief       セッション切断
 * @details     セッション情報を削除してVfsClose応答を行う。
 *
 * @param[in]   globalFD グローバルファイルディスクリプタ
 */
/******************************************************************************/
void SessmngDoVfsClose( uint32_t globalFD )
{
    sessInfo_t   *pSessInfo;    /* セッション情報        */
    LibMvfsErr_t errLibMvfs;    /* LibMvfs関数エラー要因 */
    LibMvfsRet_t retLibMvfs;    /* LibMvfs関数戻り値     */

    /* 初期化 */
    pSessInfo  = NULL;
    errLibMvfs = LIBMVFS_ERR_NONE;
    retLibMvfs = LIBMVFS_RET_FAILURE;

    /* セッション情報削除 */
    pSessInfo =
        ( sessInfo_t * ) MLibListSearchHead( &gSessInfoList,
                                             &CompareGlobalFD,
                                             &globalFD,
                                             MLIB_LIST_REMOVE  );

    /* 削除結果判定 */
    if ( pSessInfo == NULL ) {
        /* 失敗 */

        /* VfsClose応答(失敗) */
        retLibMvfs = LibMvfsSendVfsCloseResp( globalFD,
                                              LIBMVFS_RET_FAILURE,
                                              &errLibMvfs          );

        DEBUG_LOG_ERR(
            "LibMvfsSendVfsCloseResp(): fd=%u, ret=%u, err=%x",
            globalFD,
            retLibMvfs,
            errLibMvfs
        );

        return;
    }

    /* セッション情報解放 */
    free( pSessInfo );

    /* VfsClose応答(成功) */
    retLibMvfs = LibMvfsSendVfsCloseResp( globalFD,
                                          LIBMVFS_RET_SUCCESS,
                                          &errLibMvfs          );

    /* 応答結果判定 */
    if ( retLibMvfs != LIBMVFS_RET_SUCCESS ) {
        /* 失敗 */

        DEBUG_LOG_ERR(
            "LibMvfsSendVfsCloseResp(): fd=%u, ret=%u, err=%x",
            globalFD,
            retLibMvfs,
            errLibMvfs
        );
    }

    return;
}


/******************************************************************************/
/**
 * @brief       セッション接続
 * @details     セッション情報を作成し、VfsOpen応答を行う。
 *
 * @param[in]   pid       要求元プロセスID
 * @param[in]   globalFD  グローバルファイルディスクリプタ
 * @param[in]   *pPath    ファイルパス
 */
/******************************************************************************/
void SessmngDoVfsOpen( MkPid_t    pid,
                       uint32_t   globalFD,
                       const char *pPath    )
{
    sessInfo_t   *pSessInfo;    /* セッション情報        */
    MttyDevID_t  id;            /* デバイスID            */
    LibMvfsErr_t errLibMvfs;    /* LibMvfs関数エラー要因 */
    LibMvfsRet_t retLibMvfs;    /* LibMvfs関数戻り値     */

    /* 初期化 */
    pSessInfo  = NULL;
    errLibMvfs = LIBMVFS_ERR_NONE;
    retLibMvfs = LIBMVFS_RET_FAILURE;

    /* パスチェック */
    if ( strcmp( MTTY_TTYPATH_SERIAL1, pPath ) == 0 ) {
        /* シリアルポート1 */

        /* デバイスID設定 */
        id = MTTY_DEVID_SERIAL1;

    } else if ( strcmp( MTTY_TTYPATH_SERIAL2, pPath ) == 0 ) {
        /* シリアルポート2 */

        /* デバイスID設定 */
        id = MTTY_DEVID_SERIAL2;

    } else {
        /* 不正 */

        /* VfsOpen応答(失敗) */
        retLibMvfs = LibMvfsSendVfsOpenResp( globalFD,
                                             LIBMVFS_RET_FAILURE,
                                             &errLibMvfs          );

        DEBUG_LOG_ERR(
            "LibMvfsSendVfsOpenResp(): path=%s, ret=%u, err=%x",
            pPath,
            retLibMvfs,
            errLibMvfs
        );

        return;
    }

    /* セッション情報割り当て */
    pSessInfo = malloc( sizeof ( sessInfo_t ) );

    /* 割り当て結果判定 */
    if ( pSessInfo == NULL ) {
        /* 失敗 */

        /* VfsOpen応答(失敗) */
        retLibMvfs = LibMvfsSendVfsOpenResp( globalFD,
                                             LIBMVFS_RET_FAILURE,
                                             &errLibMvfs          );

        DEBUG_LOG_ERR(
            "LibMvfsSendVfsOpenResp(): path=%s, ret=%u, err=%x",
            pPath,
            retLibMvfs,
            errLibMvfs
        );

        return;
    }

    /* セッション情報設定 */
    memset( pSessInfo, 0, sizeof ( sessInfo_t ) );
    pSessInfo->globalFD = globalFD;
    pSessInfo->devID    = id;
    pSessInfo->pid      = pid;

    /* セッション情報登録 */
    ( void ) MLibListInsertHead( &gSessInfoList,
                                 ( MLibListNode_t * ) pSessInfo );

    /* VfsOpen応答(成功) */
    retLibMvfs = LibMvfsSendVfsOpenResp( globalFD,
                                         LIBMVFS_RET_SUCCESS,
                                         &errLibMvfs          );

    /* 応答結果判定 */
    if ( retLibMvfs != LIBMVFS_RET_SUCCESS ) {
        /* 失敗 */

        DEBUG_LOG_ERR(
            "LibMvfsSendVfsOpenResp(): ret=%u, err=%x",
            retLibMvfs,
            errLibMvfs
        );
    }

    return;
}


/******************************************************************************/
/**
 * @brief       セッション管理初期化
 * @details     セッション情報管理リストを初期化する。
 */
/******************************************************************************/
void SessmngInit( void )
{
    MLibRet_t retMLib;  /* MLib関数戻り値 */

    /* 初期化 */
    retMLib = MLIB_RET_FAILURE;

    /* セッション情報管理リスト初期化 */
    retMLib = MLibListInit( &gSessInfoList );

    /* 初期化結果判定 */
    if ( retMLib != MLIB_RET_SUCCESS ) {
        /* 失敗 */

        DEBUG_LOG_ERR( "MLibListInit(): ret=%d", retMLib );
    }

    return;
}


/******************************************************************************/
/* ローカル関数定義                                                           */
/******************************************************************************/
/******************************************************************************/
/**
 * @brief       グローバルFD比較
 * @details     セッション情報のグローバルFDを比較する。
 *
 * @param[in]   *pNode  セッション情報
 * @param[in]   *pParam 比較グローバルFD
 *
 * @return      比較結果を返す。
 * @retval      true  一致
 * @retval      false 不一致
 */
/******************************************************************************/
static bool CompareGlobalFD( MLibListNode_t *pNode,
                             void           *pParam )
{
    uint32_t   *pGlobalFD;  /* グローバルFD   */
    sessInfo_t *pSessInfo;  /* セッション情報 */

    /* 初期化 */
    pGlobalFD = ( uint32_t   * ) pParam;
    pSessInfo = ( sessInfo_t * ) pNode;

    /* 比較 */
    if ( pSessInfo->globalFD != *pGlobalFD ) {
        /* 不一致 */

        return false;
    }

    return true;
}


/******************************************************************************/
