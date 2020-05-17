/******************************************************************************/
/*                                                                            */
/* src/mtty/Uevt.c                                                            */
/*                                                                 2020/05/09 */
/* Copyright (C) 2020 Mochi.                                                  */
/*                                                                            */
/******************************************************************************/
/******************************************************************************/
/* インクルード                                                               */
/******************************************************************************/
/* 標準ヘッダ */
#include <stdlib.h>
#include <string.h>

/* ライブラリヘッダ */
#include <libmk.h>
#include <libmvfs.h>

/* 共通ヘッダ */
#include "mtty.h"
#include "Debug.h"
#include "Sess.h"
#include "Tty.h"
#include "Uevt.h"


/******************************************************************************/
/* 定義                                                                       */
/******************************************************************************/
/* スタックサイズ */
#define STACK_SIZE ( 8192 )


/******************************************************************************/
/* ローカル関数宣言                                                           */
/******************************************************************************/
/* スケジューリング(スレッドエントリ関数 */
static void SchedThread( void *pArg );


/******************************************************************************/
/* 静的グローバル変数定義                                                     */
/******************************************************************************/
/* スレッドタスクID */
static MkTaskId_t gTaskId;


/******************************************************************************/
/* グローバル関数定義                                                         */
/******************************************************************************/
/******************************************************************************/
/**
 * @brief       ユーザイベント初期化
 * @details     tty制御スレッドを作成する。
 */
/******************************************************************************/
void UevtInit( void )
{
    void    *pStack;    /* スタック        */
    MkErr_t errMk;      /* LibMkエラー要因 */
    MkRet_t retMk;      /* LibMk戻り値     */

    /* 初期化 */
    pStack = NULL;
    errMk  = MK_ERR_NONE;
    retMk  = MK_RET_FAILURE;

    /* スタック領域割り当て */
    pStack = malloc( STACK_SIZE );

    /* スタック領域初期化 */
    memset( pStack, 0, STACK_SIZE );

    /* スレッド生成 */
    retMk = LibMkThreadCreate( &SchedThread,     /* スレッド関数     */
                               NULL,             /* スレッド関数引数 */
                               pStack,           /* スタック         */
                               STACK_SIZE,       /* スタックサイズ   */
                               &gTaskId,         /* タスクID         */
                               &errMk        );  /* エラー要因       */

    /* 生成結果判定 */
    if ( retMk != MK_RET_SUCCESS ) {
        /* 失敗 */

        DEBUG_LOG_ERR(
            "LibMkThreadCreate(): ret=%d, err=%x",
            retMk,
            errMk
        );
    }

    return;
}


/******************************************************************************/
/* ローカル関数定義                                                           */
/******************************************************************************/
/******************************************************************************/
/**
 * @brief       スケジューリング(スレッドエントリ関数)
 * @details     ttyファイルを作成し、ttyファイルへの操作要求を待ち受ける。
 *
 * @param[in]   *pArg 未使用
 */
/******************************************************************************/
static void SchedThread( void *pArg )
{
    LibMvfsErr_t       errLibMvfs;  /* LibMvfs関数エラー要因 */
    LibMvfsRet_t       retLibMvfs;  /* LibMvfs関数戻り値     */
    LibMvfsSchedInfo_t schedInfo;   /* スケジュール情報      */

    /* 初期化 */
    errLibMvfs = LIBMVFS_ERR_NONE;
    retLibMvfs = LIBMVFS_RET_FAILURE;
    memset( &schedInfo, 0, sizeof ( LibMvfsSchedInfo_t ) );

    /* ttyS1ファイル作成 */
    retLibMvfs = LibMvfsMount( "/ttyS1", &errLibMvfs );

    /* 作成結果判定 */
    if ( retLibMvfs != LIBMVFS_RET_SUCCESS ) {
        /* 失敗 */

        DEBUG_LOG_ERR(
            "LibMvfsMount(): ret=%u, err=%x",
            retLibMvfs,
            errLibMvfs
        );
    }

    /* ttyS2ファイル作成 */
    retLibMvfs = LibMvfsMount( "/ttyS2", &errLibMvfs );

    /* 作成結果判定 */
    if ( retLibMvfs != LIBMVFS_RET_SUCCESS ) {
        /* 失敗 */

        DEBUG_LOG_ERR(
            "LibMvfsMount(): ret=%u, err=%x",
            retLibMvfs,
            errLibMvfs
        );
    }

    /* スケジュール情報設定 */
    schedInfo.callBack.pVfsClose = &SessDoVfsClose;
    schedInfo.callBack.pVfsOpen  = &SessDoVfsOpen;
    schedInfo.callBack.pVfsRead  = &TtyDoVfsRead;
    schedInfo.callBack.pVfsWrite = &TtyDoVfsWrite;
    schedInfo.callBack.pOther    = NULL;

    /* スケジュール開始 */
    retLibMvfs = LibMvfsSchedStart( &schedInfo, &errLibMvfs );

    /* 結果判定 */
    if ( retLibMvfs != LIBMVFS_RET_SUCCESS ) {
        /* 失敗 */

        DEBUG_LOG_ERR(
            "LibMvfsSchedStart(): ret=%d, err=%x",
            retLibMvfs,
            errLibMvfs
        );
    }

    /* TODO: アボート */
    DEBUG_ABORT();

    return;
}


/******************************************************************************/
