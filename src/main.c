/******************************************************************************/
/* src/main.c                                                                 */
/*                                                                 2018/10/10 */
/* Copyright (C) 2018 Mochi.                                                  */
/******************************************************************************/
/******************************************************************************/
/* インクルード                                                               */
/******************************************************************************/
/* 共通ヘッダ */
#include <mterm.h>
#include "mtty.h"
#include <stdbool.h>
#include <stddef.h>
#include <kernel/library.h>

/* モジュールヘッダ */
#include "Ldisc.h"
#include "Sess.h"


/******************************************************************************/
/* ローカル関数定義                                                           */
/******************************************************************************/
void main( void )
{
    char         buffer[ MK_MSG_SIZE_MAX + 1 ];
    size_t       size;
    MttyMsgHdr_t *pMsg;
    
    /* 初期化 */
    pMsg = ( MttyMsgHdr_t * ) buffer;
    
    /* メインループ */
    while ( true ) {
        /* メッセージ受信 */
        size = MkMsgReceive( MK_CONFIG_TASKID_NULL,     /* タスクID           */
                             pMsg,                      /* メッセージバッファ */
                             MK_MSG_SIZE_MAX,           /* バッファサイズ     */
                             NULL                   );  /* エラー番号         */
        
        /* 受信結果判定 */
        if ( size == MK_MSG_RET_FAILURE ) {
            /* 失敗 */
            
            continue;
        }
        
        /* 機能ID判定 */
        if ( pMsg->funcId == MTTY_FUNC_INPUT ) {
            /* 端末データ入力 */
            
            SessInput( ( MttyMsgInput_t * ) pMsg );
            
        } else if ( pMsg->funcId == MTTY_FUNC_READ ) {
            /* 端末データ読込 */
            
            SessDoRead( ( MttyMsgRead_t * ) pMsg );
            
        } else if ( pMsg->funcId == MTTY_FUNC_WRITE ) {
            /* 端末データ書込 */
            
            SessDoWrite( ( MttyMsgWrite_t * ) pMsg );
        }
    }
}
/******************************************************************************/
