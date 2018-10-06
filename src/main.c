/******************************************************************************/
/* src/main.c                                                                 */
/*                                                                 2018/10/05 */
/* Copyright (C) 2018 Mochi.                                                  */
/******************************************************************************/
/******************************************************************************/
/* インクルード                                                               */
/******************************************************************************/
/* 共通ヘッダ */
#include <stdbool.h>
#include <stddef.h>
#include <kernel/library.h>
#include <mterm.h>

/* モジュールヘッダ */


/******************************************************************************/
/* ローカル関数定義                                                           */
/******************************************************************************/
void main( void )
{
    char   buffer[ MK_MSG_SIZE_MAX + 1 ];
    size_t size;
    
    char buffer2[ 50 ];
    MtermMsgOutput_t *pMsg = (MtermMsgOutput_t*)buffer2;
    uint32_t index;
    uint32_t index2;
    
    /* メインループ */
    while ( true ) {
        /* メッセージ受信 */
        size = MkMsgReceive( MK_CONFIG_TASKID_NULL,     /* タスクID           */
                             buffer,                    /* メッセージバッファ */
                             MK_MSG_SIZE_MAX,           /* バッファサイズ     */
                             NULL                   );  /* エラー番号         */
        
        /* 受信結果判定 */
        if ( size == MK_MSG_RET_FAILURE ) {
            /* 失敗 */
            
            continue;
        }
        
        for ( index = 0, index2 = 0; index < size; index++ ) {
            if ( buffer[ index ] == '\n' ) {
                pMsg->data[ index2 ] = buffer[ index ];
                index2++;
            } else if ( 0 <= buffer[ index ] && buffer[ index ] <= 0x1F ) {
                pMsg->data[ index2     ] = '^';
                pMsg->data[ index2 + 1 ] = buffer[ index ] + '@';
                index2 += 2;
            } else {
                pMsg->data[ index2 ] = buffer[ index ];
                index2++;
            }
        }
        pMsg->header.funcId = MTERM_FUNC_OUTPUT;
        pMsg->header.length = index2;
        MkMsgSend( 3, pMsg, sizeof ( MtermMsgHdr_t ) + index2, NULL );
    }
}
/******************************************************************************/
