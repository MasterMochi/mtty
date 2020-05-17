/******************************************************************************/
/*                                                                            */
/* src/mtty/Buffer.h                                                          */
/*                                                                 2020/05/01 */
/* Copyright (C) 2020 Mochi.                                                  */
/*                                                                            */
/******************************************************************************/
#ifndef BUFFER_H
#define BUFFER_H
/******************************************************************************/
/* インクルード                                                               */
/******************************************************************************/
/* 標準ヘッダ */
#include <stdbool.h>
#include <stddef.h>

/* 共通ヘッダ */
#include "mtty.h"


/******************************************************************************/
/* グローバル関数宣言                                                         */
/******************************************************************************/
/* 書込み用バッファデータ有無判定 */
extern bool BufferCheckWrite( MttyDevId_t id );
/* バッファ管理初期化 */
extern void BufferInit( void );
/* 読込み用バッファ取出し */
extern size_t BufferPopForRead( MttyDevId_t id,
                                void        *pData,
                                size_t      size    );
/* 書込み用バッファ取出し */
extern size_t BufferPopForWrite( MttyDevId_t id,
                                 void        *pData,
                                 size_t      size    );
/* 読込み用バッファ追加 */
extern size_t BufferPushForRead( MttyDevId_t id,
                                 void        *pData,
                                 size_t      size    );
/* 書込み用バッファ追加 */
extern size_t BufferPushForWrite( MttyDevId_t id,
                                  void        *pData,
                                  size_t      size    );


/******************************************************************************/
#endif
