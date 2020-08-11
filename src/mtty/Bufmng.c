/******************************************************************************/
/*                                                                            */
/* src/mtty/Bufmng.c                                                          */
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

/* ライブラリヘッダ */
#include <MLib/MLibRingBuffer.h>

/* 共通ヘッダ */
#include "config.h"
#include "Bufmng.h"
#include "Debug.h"


/******************************************************************************/
/* 静的グローバル変数定義                                                     */
/******************************************************************************/
/** 読込み用バッファハンドル */
static MLibRingBuffer_t gReadBuffer[ MTTY_DEVID_NUM ];


/******************************************************************************/
/* グローバル関数定義                                                         */
/******************************************************************************/
/******************************************************************************/
/**
 * @brief       バッファ管理初期化
 * @details     バッファを初期化する。
 */
/******************************************************************************/
void BufmngInit( void )
{
    MLibErr_t errMLib;  /* MLIBエラー要因 */
    MLibRet_t retMLib;  /* MLib戻り値     */

    /* 初期化 */
    errMLib = MLIB_ERR_NONE;
    retMLib = MLIB_RET_FAILURE;

    /* シリアルポート1読込み用バッファ初期化 */
    retMLib =
        MLibRingBufferInit(
            &gReadBuffer[ MTTY_DEVID_SERIAL1 ],     /* ハンドル       */
            1,                                      /* エントリサイズ */
            CONFIG_BUFSIZE_R_SERIAL1,               /* エントリ最大数 */
            &errMLib                                /* エラー要因     */
        );

    /* 初期化結果判定 */
    if ( retMLib != MLIB_RET_SUCCESS ) {
        /* 失敗 */

        DEBUG_LOG_ERR(
            "MLibRingBufferInit(): ret=%u, err=%x",
            retMLib,
            errMLib
        );
    }

    /* シリアルポート2読込み用バッファ初期化 */
    retMLib =
        MLibRingBufferInit(
            &gReadBuffer[ MTTY_DEVID_SERIAL2 ],     /* ハンドル       */
            1,                                      /* エントリサイズ */
            CONFIG_BUFSIZE_R_SERIAL2,               /* エントリ最大数 */
            &errMLib                                /* エラー要因     */
        );

    /* 初期化結果判定 */
    if ( retMLib != MLIB_RET_SUCCESS ) {
        /* 失敗 */

        DEBUG_LOG_ERR(
            "MLibRingBufferInit(): ret=%u, err=%x",
            retMLib,
            errMLib
        );
    }

    return;
}


/******************************************************************************/
/**
 * @brief       読込み用バッファ取出し
 * @details     引数idに対応する読込み用バッファからデータを取り出す。バッファ
 *              が空となりデータが取り出せなくなった場合は中断する。
 *
 * @param[in]   id     デバイスID
 *                  - MTTY_DEVID_SERIAL1 シリアルポート1
 *                  - MTTY_DEVID_SERIAL2 シリアルポート2
 * @param[in]   *pData 取り出したデータの格納先ポインタ
 * @param[in]   size   取り出すデータのサイズ
 *
 * @return      取り出したデータのサイズを返す。
 */
/******************************************************************************/
size_t BufmngPopForRead( MttyDevID_t id,
                         void        *pData,
                         size_t      size    )
{
    uint32_t  idx;      /* 取り出しデータインデックス */
    MLibErr_t errMLib;  /* MLib関数エラー要因         */
    MLibRet_t retMLib;  /* MLib関数戻り値             */

    /* 初期化 */
    idx     = 0;
    errMLib = MLIB_ERR_NONE;
    retMLib = MLIB_RET_FAILURE;

    /* 1byte毎にサイズ分繰り返し */
    for ( idx = 0; idx < size; idx++ ) {
        /* バッファからのデータ取り出し */
        retMLib = MLibRingBufferPop( &gReadBuffer[ id ],
                                     &( ( char * ) pData )[ idx ],
                                     &errMLib                      );

        /* 取り出し結果判定 */
        if ( retMLib != MLIB_RET_SUCCESS ) {
            /* 失敗 */

            break;
        }
    }

    return idx;
}


/******************************************************************************/
/**
 * @brief       読込み用バッファ追加
 * @details     引数idに対応する読込み用バッファにデータを追加する。バッファが
 *              フルとなり追加できなかったデータは破棄する。
 *
 * @param[in]   id     デバイスID
 *                  - MTTY_DEVID_SERIAL1 シリアルポート1
 *                  - MTTY_DEVID_SERIAL2 シリアルポート2
 * @param[in]   *pData 追加するデータへのポインタ
 * @param[in]   size   追加するデータのサイズ
 *
 * @return      バッファに追加したデータのサイズを返す。
 */
/******************************************************************************/
size_t BufmngPushForRead( MttyDevID_t id,
                          void        *pData,
                          size_t      size    )
{
    uint32_t  idx;      /* 書込みデータインデックス */
    MLibErr_t errMLib;  /* MLib関数エラー関数       */
    MLibRet_t retMLib;  /* MLib関数戻り値           */

    /* 初期化 */
    idx     = 0;
    errMLib = MLIB_ERR_NONE;
    retMLib = MLIB_RET_FAILURE;

    /* 1byte毎にサイズ分繰り返す */
    for ( idx = 0; idx < size; idx++ ) {
        /* バッファへのデータ追加 */
        retMLib = MLibRingBufferPush( &gReadBuffer[ id ],
                                      &( ( char * ) pData )[ idx ],
                                      &errMLib                      );

        /* 追加結果判定 */
        if ( retMLib != MLIB_RET_SUCCESS ) {
            /* 失敗 */

            DEBUG_LOG_ERR(
                "MLibRingBufferPush(): id=%u, ret=%u, err=%x",
                id,
                retMLib,
                errMLib
            );

            break;
        }
    }

    return idx;
}


/******************************************************************************/
