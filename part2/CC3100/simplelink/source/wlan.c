/*
 * wlan.c - CC31xx/CC32xx Host Driver Implementation
 *
 * Copyright (C) 2014 Texas Instruments Incorporated - http://www.ti.com/ 
 * 
 * 
 *  Redistribution and use in source and binary forms, with or without 
 *  modification, are permitted provided that the following conditions 
 *  are met:
 *
 *    Redistributions of source code must retain the above copyright 
 *    notice, this list of conditions and the following disclaimer.
 *
 *    Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the 
 *    documentation and/or other materials provided with the   
 *    distribution.
 *
 *    Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
*/

/*****************************************************************************

    API Prototypes

 *****************************************************************************/
#include "datatypes.h"
#include "simplelink.h"
#include "protocol.h"
#include "driver.h"

/*****************************************************************************

*****************************************************************************/

#define MAX_SSID_LEN        32
#define MAX_KEY_LEN         63
#define MAX_USER_LEN        32
#define MAX_ANON_USER_LEN   32

#define MAX_SMART_CONFIG_KEY    16

typedef struct 
{
    _WlanConnectEapCommand_t    Args;
    char                        Strings[MAX_SSID_LEN + MAX_KEY_LEN + MAX_USER_LEN + MAX_ANON_USER_LEN];
}_WlanConnectCmd_t;

typedef union
{
	_WlanConnectCmd_t   Cmd;
	_BasicResponse_t	Rsp;
}_SlWlanConnectMsg_u;

/*****************************************************************************
  sl_WlanConnect
*****************************************************************************/
#if _SL_INCLUDE_FUNC(sl_WlanConnect)
int sl_WlanConnect(char* pName, int NameLen, unsigned char *pMacAddr, SlSecParams_t* pSecParams , SlSecParamsExt_t* pSecExtParams)
{
    _SlWlanConnectMsg_u    Msg = {0};
    _SlCmdCtrl_t           CmdCtrl = {0};

    CmdCtrl.TxDescLen = 0;/* init */
    CmdCtrl.RxDescLen = sizeof(_BasicResponse_t);

    /* verify SSID length */
    VERIFY_PROTOCOL(NameLen <= MAX_SSID_LEN);
    /* update SSID length */
    Msg.Cmd.Args.Common.SsidLen = NameLen;

    /* Profile with no security */
    /* Enterprise security profile */
    if (NULL != pSecExtParams)
    {
        /* Update command opcode */
        CmdCtrl.Opcode = SL_OPCODE_WLAN_WLANCONNECTEAPCOMMAND;
        CmdCtrl.TxDescLen += sizeof(_WlanConnectEapCommand_t);
        /* copy SSID */
        sl_Memcpy(EAP_SSID_STRING(&Msg), pName, NameLen);
        CmdCtrl.TxDescLen += NameLen;
        /* Copy password if supplied */
        if ((NULL != pSecParams) && (pSecParams->KeyLen > 0))
    {
            /* update security type */
		Msg.Cmd.Args.Common.SecType = pSecParams->Type;
            /* verify key length */
			if (pSecParams->KeyLen > MAX_KEY_LEN)
			{
				return SL_INVALPARAM;
			}
            /* update key length */
		Msg.Cmd.Args.Common.PasswordLen = pSecParams->KeyLen;
			ARG_CHECK_PTR(pSecParams->Key);
            /* copy key		 */
			sl_Memcpy(EAP_PASSWORD_STRING(&Msg), pSecParams->Key, pSecParams->KeyLen);
			CmdCtrl.TxDescLen += pSecParams->KeyLen;
		}
		else
		{
            Msg.Cmd.Args.Common.PasswordLen = 0;
    }

        ARG_CHECK_PTR(pSecExtParams);
        /* Update Eap bitmask */
        Msg.Cmd.Args.EapBitmask = pSecExtParams->EapMethod;
        /* Update Certificate file ID index - currently not supported */
        Msg.Cmd.Args.CertIndex = pSecExtParams->CertIndex;
        /* verify user length */
		if (pSecExtParams->UserLen > MAX_USER_LEN)
		{
			return SL_INVALPARAM;
		}
        Msg.Cmd.Args.UserLen = pSecExtParams->UserLen;
        /* copy user name (identity) */
        if(pSecExtParams->UserLen > 0)
        {
            sl_Memcpy(EAP_USER_STRING(&Msg), pSecExtParams->User, pSecExtParams->UserLen);
            CmdCtrl.TxDescLen += pSecExtParams->UserLen;
        }
        /* verify Anonymous user length  */
		if (pSecExtParams->AnonUserLen > MAX_ANON_USER_LEN)
		{
			return SL_INVALPARAM;
		}
        Msg.Cmd.Args.AnonUserLen = pSecExtParams->AnonUserLen;
        /* copy Anonymous user */
        if(pSecExtParams->AnonUserLen > 0)
        {
            sl_Memcpy(EAP_ANON_USER_STRING(&Msg), pSecExtParams->AnonUser, pSecExtParams->AnonUserLen);
            CmdCtrl.TxDescLen += pSecExtParams->AnonUserLen;
        }
        
    }

	/* Regular or open security profile */
    else
    {
        /* Update command opcode */
        CmdCtrl.Opcode = SL_OPCODE_WLAN_WLANCONNECTCOMMAND;
        CmdCtrl.TxDescLen += sizeof(_WlanConnectCommon_t);
        /* copy SSID */
        memcpy(SSID_STRING(&Msg), pName, NameLen);	
        CmdCtrl.TxDescLen += NameLen;
        /* Copy password if supplied */
        if( NULL != pSecParams )
        {
            /* update security type */
            Msg.Cmd.Args.Common.SecType = pSecParams->Type;
            /* verify key length is valid */
            if (pSecParams->KeyLen > MAX_KEY_LEN)
			{
				return SL_INVALPARAM;
			}
            /* update key length */
            Msg.Cmd.Args.Common.PasswordLen = pSecParams->KeyLen;
            CmdCtrl.TxDescLen += pSecParams->KeyLen;
            /* copy key (could be no key in case of WPS pin) */
            if( NULL != pSecParams->Key )
            {
            sl_Memcpy(PASSWORD_STRING(&Msg), pSecParams->Key, pSecParams->KeyLen);
        }
        }
		/* Profile with no security */
        else
        {
            Msg.Cmd.Args.Common.PasswordLen = 0;
			Msg.Cmd.Args.Common.SecType = SL_SEC_TYPE_OPEN;
        }	
    }
    /* If BSSID is not null, copy to buffer, otherwise set to 0 */
    if(NULL != pMacAddr)
    {
        sl_Memcpy(Msg.Cmd.Args.Common.Bssid, pMacAddr, sizeof(Msg.Cmd.Args.Common.Bssid));
    }
    else
    {
        sl_Memset(Msg.Cmd.Args.Common.Bssid, 0, sizeof(Msg.Cmd.Args.Common.Bssid));
    }


    VERIFY_RET_OK ( _SlDrvCmdOp(&CmdCtrl, &Msg, NULL));

    return (int)Msg.Rsp.status;
}
#endif

/*******************************************************************************/
/*   sl_Disconnect  */
/* ******************************************************************************/
#if _SL_INCLUDE_FUNC(sl_WlanDisconnect)
int sl_WlanDisconnect(void)
{
    return _SlDrvBasicCmd(SL_OPCODE_WLAN_WLANDISCONNECTCOMMAND);
}
#endif

/******************************************************************************/
/*  sl_PolicySet  */
/******************************************************************************/
typedef union
{
    _WlanPoliciySetGet_t    Cmd;
	_BasicResponse_t	    Rsp;
}_SlPolicyMsg_u;

const _SlCmdCtrl_t _SlPolicySetCmdCtrl =
{
    SL_OPCODE_WLAN_POLICYSETCOMMAND,
    sizeof(_WlanPoliciySetGet_t),
    sizeof(_BasicResponse_t)
};

#if _SL_INCLUDE_FUNC(sl_WlanPolicySet)
int sl_WlanPolicySet(unsigned char Type , const unsigned char Policy, unsigned char *pVal,unsigned char ValLen)
{
    _SlPolicyMsg_u         Msg;
    _SlCmdExt_t            CmdExt;

    CmdExt.TxPayloadLen = ValLen;
    CmdExt.RxPayloadLen = 0;
    CmdExt.pTxPayload = (UINT8 *)pVal;
    CmdExt.pRxPayload = NULL;


    Msg.Cmd.PolicyType        = Type;
    Msg.Cmd.PolicyOption      = Policy;
    Msg.Cmd.PolicyOptionLen   = ValLen;

    VERIFY_RET_OK(_SlDrvCmdOp((_SlCmdCtrl_t *)&_SlPolicySetCmdCtrl, &Msg, &CmdExt));

    return (int)Msg.Rsp.status;
}
#endif


/******************************************************************************/
/*  sl_PolicyGet  */
/******************************************************************************/
typedef union
{
	_WlanPoliciySetGet_t	    Cmd;
	_WlanPoliciySetGet_t	    Rsp;
}_SlPolicyGetMsg_u;

const _SlCmdCtrl_t _SlPolicyGetCmdCtrl =
{
    SL_OPCODE_WLAN_POLICYGETCOMMAND,
    sizeof(_WlanPoliciySetGet_t),
    sizeof(_WlanPoliciySetGet_t)
};

#if _SL_INCLUDE_FUNC(sl_WlanPolicyGet)
int sl_WlanPolicyGet(unsigned char Type , unsigned char Policy,unsigned char *pVal,unsigned char *pValLen)
{
    _SlPolicyGetMsg_u      Msg;
    _SlCmdExt_t            CmdExt;

	if (*pValLen == 0)
	{
		return SL_EZEROLEN;
	}
    CmdExt.TxPayloadLen = 0;
    CmdExt.RxPayloadLen = *pValLen;
    CmdExt.pTxPayload = NULL;
    CmdExt.pRxPayload = pVal;
	CmdExt.ActualRxPayloadLen = 0;


    Msg.Cmd.PolicyType = Type;
    Msg.Cmd.PolicyOption = Policy;
    VERIFY_RET_OK(_SlDrvCmdOp((_SlCmdCtrl_t *)&_SlPolicyGetCmdCtrl, &Msg, &CmdExt));

    
	if (CmdExt.RxPayloadLen < CmdExt.ActualRxPayloadLen) 
	{
		*pValLen = Msg.Rsp.PolicyOptionLen;
		return SL_ESMALLBUF;
	}
	else
	{
		/*  no pointer valus, fill the results into char */
		*pValLen = (unsigned char)CmdExt.ActualRxPayloadLen;
		if( 0 == CmdExt.ActualRxPayloadLen )
		{
			*pValLen = 1;
			 pVal[0] = Msg.Rsp.PolicyOption;
		}
	}

    
	return (int)SL_OS_RET_CODE_OK;
}
#endif


/*******************************************************************************/
/*  sl_ProfileAdd  */
/*******************************************************************************/
typedef struct
{
	_WlanAddGetEapProfile_t	Args;
    char                    Strings[MAX_SSID_LEN + MAX_KEY_LEN + MAX_USER_LEN + MAX_ANON_USER_LEN];
}_SlProfileParams_t;

typedef union
{
	_SlProfileParams_t	    Cmd;
	_BasicResponse_t	    Rsp;
}_SlProfileAddMsg_u;



#if _SL_INCLUDE_FUNC(sl_WlanProfileAdd)
int sl_WlanProfileAdd(char* pName, int NameLen, unsigned char *pMacAddr, SlSecParams_t* pSecParams , SlSecParamsExt_t* pSecExtParams, unsigned long  Priority, unsigned long  Options)
{
    _SlProfileAddMsg_u      Msg;
	_SlCmdCtrl_t           CmdCtrl = {0};
	CmdCtrl.TxDescLen = 0;/* init */
	CmdCtrl.RxDescLen = sizeof(_BasicResponse_t);

	/* update priority */
	Msg.Cmd.Args.Common.Priority = (UINT8)Priority; 
	/* verify SSID length */
	VERIFY_PROTOCOL(NameLen <= MAX_SSID_LEN);
	/* update SSID length */
	Msg.Cmd.Args.Common.SsidLen = (UINT8)NameLen;


	/* Enterprise security profile */
        if  (NULL != pSecExtParams)
	{
		/* Update command opcode */
		CmdCtrl.Opcode = SL_OPCODE_WLAN_EAP_PROFILEADDCOMMAND;
		CmdCtrl.TxDescLen += sizeof(_WlanAddGetEapProfile_t);

		/* copy SSID */
		memcpy(EAP_PROFILE_SSID_STRING(&Msg), pName, NameLen);	
		CmdCtrl.TxDescLen += NameLen;

		/* Copy password if supplied */
		if ((NULL != pSecParams) && (pSecParams->KeyLen > 0))
		{
			/* update security type */
			Msg.Cmd.Args.Common.SecType = pSecParams->Type;

			if( SL_SEC_TYPE_WEP == Msg.Cmd.Args.Common.SecType )
			{
			    Msg.Cmd.Args.Common.WepKeyId = 0;
			}

			/* verify key length */
			if (pSecParams->KeyLen > MAX_KEY_LEN)
			{
				return SL_INVALPARAM;
			}
			VERIFY_PROTOCOL(pSecParams->KeyLen <= MAX_KEY_LEN);
			/* update key length */
			Msg.Cmd.Args.Common.PasswordLen = pSecParams->KeyLen;	
			CmdCtrl.TxDescLen += pSecParams->KeyLen;
			ARG_CHECK_PTR(pSecParams->Key);
			/* copy key  */
			sl_Memcpy(EAP_PROFILE_PASSWORD_STRING(&Msg), pSecParams->Key, pSecParams->KeyLen);
		}
		else
		{
			Msg.Cmd.Args.Common.PasswordLen = 0;
		}

		ARG_CHECK_PTR(pSecExtParams);
		/* Update Eap bitmask */
		Msg.Cmd.Args.EapBitmask = pSecExtParams->EapMethod;
		/* Update Certificate file ID index - currently not supported */
		Msg.Cmd.Args.CertIndex = pSecExtParams->CertIndex;
		/* verify user length */
		if (pSecExtParams->UserLen > MAX_USER_LEN)
		{
			return SL_INVALPARAM;
		}
		Msg.Cmd.Args.UserLen = pSecExtParams->UserLen;
		/* copy user name (identity) */
		if(pSecExtParams->UserLen > 0)
		{
			sl_Memcpy(EAP_PROFILE_USER_STRING(&Msg), pSecExtParams->User, pSecExtParams->UserLen);
			CmdCtrl.TxDescLen += pSecExtParams->UserLen;
		}

		/* verify Anonymous user length (for tunneled) */
		if (pSecExtParams->AnonUserLen > MAX_ANON_USER_LEN)
		{
			return SL_INVALPARAM;
		}
		Msg.Cmd.Args.AnonUserLen = pSecExtParams->AnonUserLen;

		/* copy Anonymous user */
		if(pSecExtParams->AnonUserLen > 0)
		{
			sl_Memcpy(EAP_PROFILE_ANON_USER_STRING(&Msg), pSecExtParams->AnonUser, pSecExtParams->AnonUserLen);
			CmdCtrl.TxDescLen += pSecExtParams->AnonUserLen;
		}

	}
	/* Regular or open security profile */
	else
	{
		/* Update command opcode */
		CmdCtrl.Opcode = SL_OPCODE_WLAN_PROFILEADDCOMMAND;
		/* update commnad length */
		CmdCtrl.TxDescLen += sizeof(_WlanAddGetProfile_t);

		if (NULL != pName)
		{
			/* copy SSID */
            memcpy(PROFILE_SSID_STRING(&Msg), pName, NameLen);
			CmdCtrl.TxDescLen += NameLen;
		}

		/* Copy password if supplied */
		if( NULL != pSecParams )
		{
			/* update security type */
			Msg.Cmd.Args.Common.SecType = pSecParams->Type;

			if( SL_SEC_TYPE_WEP == Msg.Cmd.Args.Common.SecType )
			{
				 Msg.Cmd.Args.Common.WepKeyId = 0;
			}

			/* verify key length */
			if (pSecParams->KeyLen > MAX_KEY_LEN)
			{
				return SL_INVALPARAM;
			}
			/* update key length */
			Msg.Cmd.Args.Common.PasswordLen = pSecParams->KeyLen;
			CmdCtrl.TxDescLen += pSecParams->KeyLen;
            /* copy key (could be no key in case of WPS pin) */
                        if( NULL != pSecParams->Key )
                        {
			sl_Memcpy(PROFILE_PASSWORD_STRING(&Msg), pSecParams->Key, pSecParams->KeyLen);
		}
		}
		else
		{
			Msg.Cmd.Args.Common.SecType = SL_SEC_TYPE_OPEN;
			Msg.Cmd.Args.Common.PasswordLen = 0;
		}

	}


	/* If BSSID is not null, copy to buffer, otherwise set to 0  */
	if(NULL != pMacAddr)
	{
		sl_Memcpy(Msg.Cmd.Args.Common.Bssid, pMacAddr, sizeof(Msg.Cmd.Args.Common.Bssid));
	}
	else
	{
		sl_Memset(Msg.Cmd.Args.Common.Bssid, 0, sizeof(Msg.Cmd.Args.Common.Bssid));
	}

	VERIFY_RET_OK(_SlDrvCmdOp(&CmdCtrl, &Msg, NULL));

    return (int)Msg.Rsp.status;
}
#endif
/*******************************************************************************/
/*   sl_ProfileGet */
/*******************************************************************************/
typedef union
{
    _WlanProfileDelGetCommand_t Cmd;
	_SlProfileParams_t	        Rsp;
}_SlProfileGetMsg_u;

const _SlCmdCtrl_t _SlProfileGetCmdCtrl =
{
    SL_OPCODE_WLAN_PROFILEGETCOMMAND,
    sizeof(_WlanProfileDelGetCommand_t),
    sizeof(_SlProfileParams_t)
};
#if _SL_INCLUDE_FUNC(sl_WlanProfileGet)
int sl_WlanProfileGet(int Index,char* pName, int *pNameLen, unsigned char *pMacAddr, SlSecParams_t* pSecParams, SlGetSecParamsExt_t* pEntParams, unsigned long *pPriority)
{
    _SlProfileGetMsg_u      Msg;
    Msg.Cmd.index = (UINT8)Index;

    VERIFY_RET_OK(_SlDrvCmdOp((_SlCmdCtrl_t *)&_SlProfileGetCmdCtrl, &Msg, NULL));

	pSecParams->Type = Msg.Rsp.Args.Common.SecType;
	/* since password is not transferred in getprofile, password length should always be zero */
	pSecParams->KeyLen = Msg.Rsp.Args.Common.PasswordLen;
	if (NULL != pEntParams)
	{
	pEntParams->UserLen = Msg.Rsp.Args.UserLen;
	    /* copy user name */
	if (pEntParams->UserLen > 0)
	{	 
		memcpy(pEntParams->User, EAP_PROFILE_USER_STRING(&Msg), pEntParams->UserLen);
	}
	pEntParams->AnonUserLen = Msg.Rsp.Args.AnonUserLen;
	    /* copy anonymous user name */
	if (pEntParams->AnonUserLen > 0)
	{
		memcpy(pEntParams->AnonUser, EAP_PROFILE_ANON_USER_STRING(&Msg), pEntParams->AnonUserLen);
	}
        }
     
    *pNameLen  = Msg.Rsp.Args.Common.SsidLen;      
    *pPriority = Msg.Rsp.Args.Common.Priority;       

				//	if (NULL != Msg.Rsp.Args.Common.Bssid) comment out for the following compilation warning:
				// ../cc3100/simplelink/source/wlan.c(527): warning: comparison of array 'Msg.Rsp.Args.Common.Bssid' not equal to a null pointer is always true [-Wtautological-pointer-compare]
	{
		memcpy(pMacAddr, Msg.Rsp.Args.Common.Bssid, sizeof(Msg.Rsp.Args.Common.Bssid));
	}

    memcpy(pName, EAP_PROFILE_SSID_STRING(&Msg), *pNameLen);

    return (int)Msg.Rsp.Args.Common.SecType;

}
#endif
/*******************************************************************************/
/*   sl_ProfileDel  */
/*******************************************************************************/
typedef union
{
	_WlanProfileDelGetCommand_t	    Cmd;
	_BasicResponse_t	            Rsp;
}_SlProfileDelMsg_u;

const _SlCmdCtrl_t _SlProfileDelCmdCtrl =
{
    SL_OPCODE_WLAN_PROFILEDELCOMMAND,
    sizeof(_WlanProfileDelGetCommand_t),
    sizeof(_BasicResponse_t)
};

#if _SL_INCLUDE_FUNC(sl_WlanProfileDel)
int sl_WlanProfileDel(int Index)
{
    _SlProfileDelMsg_u Msg;

    Msg.Cmd.index = (UINT8)Index;

    VERIFY_RET_OK(_SlDrvCmdOp((_SlCmdCtrl_t *)&_SlProfileDelCmdCtrl, &Msg, NULL));

    return (int)Msg.Rsp.status;
}
#endif


/******************************************************************************/
/*  sl_WlanGetNetworkList  */
/******************************************************************************/
typedef union
{
	_WlanGetNetworkListCommand_t    Cmd;
	_WlanGetNetworkListResponse_t   Rsp;
}_SlWlanGetNetworkListMsg_u;

const _SlCmdCtrl_t _SlWlanGetNetworkListCtrl =
{
    SL_OPCODE_WLAN_SCANRESULTSGETCOMMAND,
    sizeof(_WlanGetNetworkListCommand_t),
    sizeof(_WlanGetNetworkListResponse_t)
};


#if _SL_INCLUDE_FUNC(sl_WlanGetNetworkList)
int sl_WlanGetNetworkList(unsigned char Index, unsigned char Count, Sl_WlanNetworkEntry_t *pEntries)
{
    int retVal = 0;
    _SlWlanGetNetworkListMsg_u Msg;
    _SlCmdExt_t    CmdExt;

	if (Count == 0)
	{
		return SL_EZEROLEN;
	}
    CmdExt.TxPayloadLen = 0;
    CmdExt.RxPayloadLen = sizeof(Sl_WlanNetworkEntry_t)*(Count);
    CmdExt.pTxPayload = NULL;
    CmdExt.pRxPayload = (UINT8 *)pEntries; 

    Msg.Cmd.index = Index;
    Msg.Cmd.count = Count;

    VERIFY_RET_OK(_SlDrvCmdOp((_SlCmdCtrl_t *)&_SlWlanGetNetworkListCtrl, &Msg, &CmdExt));
    retVal = Msg.Rsp.status;

    return (int)retVal;
}
#endif





/******************************************************************************/
/*     RX filters message command response structures  */
/******************************************************************************/

/* Set command */
typedef union
{
	_WlanRxFilterAddCommand_t	          Cmd;
	_WlanRxFilterAddCommandReponse_t      Rsp;
}_SlrxFilterAddMsg_u;

const _SlCmdCtrl_t _SlRxFilterAddtCmdCtrl =
{
	SL_OPCODE_WLAN_WLANRXFILTERADDCOMMAND,
    sizeof(_WlanRxFilterAddCommand_t),
    sizeof(_WlanRxFilterAddCommandReponse_t)
};


/* Set command */
typedef union _SlRxFilterSetMsg_u
{
	_WlanRxFilterSetCommand_t	            Cmd;
	_WlanRxFilterSetCommandReponse_t        Rsp;
}_SlRxFilterSetMsg_u;


const _SlCmdCtrl_t _SlRxFilterSetCmdCtrl =
{
	SL_OPCODE_WLAN_WLANRXFILTERSETCOMMAND,
    sizeof(_WlanRxFilterSetCommand_t),
    sizeof(_WlanRxFilterSetCommandReponse_t)
};

/* Get command */
typedef union _SlRxFilterGetMsg_u
{
	_WlanRxFilterGetCommand_t	            Cmd;
	_WlanRxFilterGetCommandReponse_t        Rsp;
}_SlRxFilterGetMsg_u;


const _SlCmdCtrl_t _SlRxFilterGetCmdCtrl =
{
	SL_OPCODE_WLAN_WLANRXFILTERGETCOMMAND,
    sizeof(_WlanRxFilterGetCommand_t),
    sizeof(_WlanRxFilterGetCommandReponse_t)
};


/*******************************************************************************/
/*     RX filters  */
/*******************************************************************************/

#if _SL_INCLUDE_FUNC(sl_WlanRxFilterAdd)
SlrxFilterID_t sl_WlanRxFilterAdd(	SlrxFilterRuleType_t 				RuleType,
									SlrxFilterFlags_t 					FilterFlags,
									const SlrxFilterRule_t* const 		Rule,
									const SlrxFilterTrigger_t* const 	Trigger,
									const SlrxFilterAction_t* const 	Action,
									SlrxFilterID_t*                     pFilterId)

{


	_SlrxFilterAddMsg_u Msg;
	Msg.Cmd.RuleType = RuleType;
	/* filterId is zero */
	Msg.Cmd.FilterId = 0;
	Msg.Cmd.FilterFlags = FilterFlags;
	sl_Memcpy( &(Msg.Cmd.Rule), Rule, sizeof(SlrxFilterRule_t) );
	sl_Memcpy( &(Msg.Cmd.Trigger), Trigger, sizeof(SlrxFilterTrigger_t) );
	sl_Memcpy( &(Msg.Cmd.Action), Action, sizeof(SlrxFilterAction_t) );
	VERIFY_RET_OK(_SlDrvCmdOp((_SlCmdCtrl_t *)&_SlRxFilterAddtCmdCtrl, &Msg, NULL) );
	*pFilterId = Msg.Rsp.FilterId;
	return (int)Msg.Rsp.Status;

}
#endif



/*******************************************************************************/
/*     RX filters    */
/*******************************************************************************/
#if _SL_INCLUDE_FUNC(sl_WlanRxFilterSet)
int sl_WlanRxFilterSet(const SLrxFilterOperation_t RxFilterOperation,
		                           const unsigned char* const pInputBuffer,
		               UINT16 InputbufferLength)
{
    _SlRxFilterSetMsg_u   Msg;
    _SlCmdExt_t           CmdExt;

    CmdExt.TxPayloadLen = InputbufferLength;
    CmdExt.pTxPayload   = (UINT8 *)pInputBuffer;
    CmdExt.RxPayloadLen = 0;
    CmdExt.pRxPayload   = (UINT8 *)NULL;

	Msg.Cmd.RxFilterOperation = RxFilterOperation;
	Msg.Cmd.InputBufferLength = InputbufferLength;


	VERIFY_RET_OK(_SlDrvCmdOp((_SlCmdCtrl_t *)&_SlRxFilterSetCmdCtrl, &Msg, &CmdExt) );


	return (int)Msg.Rsp.Status;
}
#endif

/******************************************************************************/
/*     RX filters  */
/******************************************************************************/
#if _SL_INCLUDE_FUNC(sl_WlanRxFilterGet)
int sl_WlanRxFilterGet(const SLrxFilterOperation_t RxFilterOperation,
								 unsigned char* pOutputBuffer,
		               UINT16 OutputbufferLength)
{
    _SlRxFilterGetMsg_u   Msg;
    _SlCmdExt_t           CmdExt;

	if (OutputbufferLength == 0)
	{
		return SL_EZEROLEN;
	}
    CmdExt.TxPayloadLen = 0;
    CmdExt.pTxPayload   = NULL;
    CmdExt.RxPayloadLen = OutputbufferLength;
    CmdExt.pRxPayload   = (UINT8 *)pOutputBuffer;
	CmdExt.ActualRxPayloadLen = 0;

	Msg.Cmd.RxFilterOperation = RxFilterOperation;
	Msg.Cmd.OutputBufferLength = OutputbufferLength;


	VERIFY_RET_OK(_SlDrvCmdOp((_SlCmdCtrl_t *)&_SlRxFilterGetCmdCtrl, &Msg, &CmdExt) );

	if (CmdExt.RxPayloadLen < CmdExt.ActualRxPayloadLen) 
	{
		return SL_ESMALLBUF;
	}

	return (int)Msg.Rsp.Status;
}
#endif

/*******************************************************************************/
/*             sl_WlanRxStatStart                                              */
/*******************************************************************************/
#if _SL_INCLUDE_FUNC(sl_WlanRxStatStart)
int sl_WlanRxStatStart(void)
{
    return _SlDrvBasicCmd(SL_OPCODE_WLAN_STARTRXSTATCOMMAND);
}
#endif

#if _SL_INCLUDE_FUNC(sl_WlanRxStatStop)
int sl_WlanRxStatStop(void)
{
    return _SlDrvBasicCmd(SL_OPCODE_WLAN_STOPRXSTATCOMMAND);
}
#endif

#if _SL_INCLUDE_FUNC(sl_WlanRxStatGet)
int sl_WlanRxStatGet(SlGetRxStatResponse_t *pRxStat,unsigned long Flags)
{
    _SlCmdCtrl_t            CmdCtrl = {SL_OPCODE_WLAN_GETRXSTATCOMMAND, 0, sizeof(SlGetRxStatResponse_t)};
    memset(pRxStat,0,sizeof(SlGetRxStatResponse_t));
    VERIFY_RET_OK(_SlDrvCmdOp((_SlCmdCtrl_t *)&CmdCtrl, pRxStat, NULL)); 

    return 0;
}
#endif



/******************************************************************************/
/*   sl_WlanSmartConfigStop                                                   */
/******************************************************************************/
#if _SL_INCLUDE_FUNC(sl_WlanSmartConfigStop)
int sl_WlanSmartConfigStop(void)
{
    return _SlDrvBasicCmd(SL_OPCODE_WLAN_SMART_CONFIG_STOP_COMMAND);
}
#endif


/******************************************************************************/
/*   sl_WlanSmartConfigStart                                                  */
/******************************************************************************/


typedef struct
{
	_WlanSmartConfigStartCommand_t	Args;
    char                            Strings[3 * MAX_SMART_CONFIG_KEY]; /* public key + groupId1 key + groupId2 key */
}_SlSmartConfigStart_t;

typedef union
{
	_SlSmartConfigStart_t	    Cmd;
	_BasicResponse_t	        Rsp;
}_SlSmartConfigStartMsg_u;

const _SlCmdCtrl_t _SlSmartConfigStartCmdCtrl =
{
    SL_OPCODE_WLAN_SMART_CONFIG_START_COMMAND,
    sizeof(_SlSmartConfigStart_t),
    sizeof(_BasicResponse_t)
};

#if _SL_INCLUDE_FUNC(sl_WlanSmartConfigStart)
int sl_WlanSmartConfigStart( const unsigned long    groupIdBitmask,
                             const unsigned char    cipher,
                             const unsigned char    publicKeyLen,
                             const unsigned char    group1KeyLen,
                             const unsigned char    group2KeyLen,
                             const unsigned char*   pPublicKey,
                             const unsigned char*   pGroup1Key,
                             const unsigned char*   pGroup2Key)
{
    _SlSmartConfigStartMsg_u      Msg;

    Msg.Cmd.Args.groupIdBitmask = (UINT8)groupIdBitmask;
    Msg.Cmd.Args.cipher         = (UINT8)cipher;
    Msg.Cmd.Args.publicKeyLen   = (UINT8)publicKeyLen;
    Msg.Cmd.Args.group1KeyLen   = (UINT8)group1KeyLen;
    Msg.Cmd.Args.group2KeyLen   = (UINT8)group2KeyLen;

    /* copy keys (if exist) after command (one after another) */
    memcpy(SMART_CONFIG_START_PUBLIC_KEY_STRING(&Msg), pPublicKey, publicKeyLen);
    memcpy(SMART_CONFIG_START_GROUP1_KEY_STRING(&Msg), pGroup1Key, group1KeyLen);
    memcpy(SMART_CONFIG_START_GROUP2_KEY_STRING(&Msg), pGroup2Key, group2KeyLen);

    VERIFY_RET_OK(_SlDrvCmdOp((_SlCmdCtrl_t *)&_SlSmartConfigStartCmdCtrl , &Msg, NULL));

    return (int)Msg.Rsp.status;


}
#endif


/*******************************************************************************/
/*   sl_WlanSetMode  */
/*******************************************************************************/
typedef union
{
	_WlanSetMode_t		    Cmd;
	_BasicResponse_t	    Rsp;
}_SlwlanSetModeMsg_u;

const _SlCmdCtrl_t _SlWlanSetModeCmdCtrl =
{
    SL_OPCODE_WLAN_SET_MODE,
    sizeof(_WlanSetMode_t),
    sizeof(_BasicResponse_t)
};

/* possible values are: 
     WLAN_SET_STA_MODE   =   1
     WLAN_SET_AP_MODE    =   2
     WLAN_SET_P2P_MODE   =   3  */

#if _SL_INCLUDE_FUNC(sl_WlanSetMode)
int sl_WlanSetMode(const unsigned char    mode)
{
    _SlwlanSetModeMsg_u      Msg;

	Msg.Cmd.mode  = mode;

    VERIFY_RET_OK(_SlDrvCmdOp((_SlCmdCtrl_t *)&_SlWlanSetModeCmdCtrl , &Msg, NULL));

    return (int)Msg.Rsp.status;

}
#endif




/*******************************************************************************/
/*   sl_WlanSet  */
/* ******************************************************************************/
typedef union
{
	_WlanCfgSetGet_t	    Cmd;
	_BasicResponse_t	    Rsp;
}_SlWlanCfgSetMsg_u;

const _SlCmdCtrl_t _SlWlanCfgSetCmdCtrl =
{
    SL_OPCODE_WLAN_CFG_SET,
    sizeof(_WlanCfgSetGet_t),
    sizeof(_BasicResponse_t)
};


#if _SL_INCLUDE_FUNC(sl_WlanSet)
int sl_WlanSet(unsigned short ConfigId ,unsigned short ConfigOpt,unsigned short ConfigLen, unsigned char *pValues)
{
    _SlWlanCfgSetMsg_u         Msg;
    _SlCmdExt_t                CmdExt;

	CmdExt.TxPayloadLen = (ConfigLen+3) & (~3);
    CmdExt.RxPayloadLen = 0;
    CmdExt.pTxPayload = (UINT8 *)pValues;
    CmdExt.pRxPayload = NULL;


    Msg.Cmd.ConfigId    = ConfigId;
    Msg.Cmd.ConfigLen   = ConfigLen;
	Msg.Cmd.ConfigOpt   = ConfigOpt;

    VERIFY_RET_OK(_SlDrvCmdOp((_SlCmdCtrl_t *)&_SlWlanCfgSetCmdCtrl, &Msg, &CmdExt));

    return (int)Msg.Rsp.status;
}
#endif


/******************************************************************************/
/*  sl_WlanGet  */
/******************************************************************************/
typedef union
{
	_WlanCfgSetGet_t	    Cmd;
	_WlanCfgSetGet_t	    Rsp;
}_SlWlanCfgMsgGet_u;

const _SlCmdCtrl_t _SlWlanCfgGetCmdCtrl =
{
    SL_OPCODE_WLAN_CFG_GET,
    sizeof(_WlanCfgSetGet_t),
    sizeof(_WlanCfgSetGet_t)
};

#if _SL_INCLUDE_FUNC(sl_WlanGet)
int sl_WlanGet(unsigned short ConfigId, unsigned short *pConfigOpt,unsigned short *pConfigLen, unsigned char *pValues)
{
    _SlWlanCfgMsgGet_u        Msg;
    _SlCmdExt_t               CmdExt;

	if (*pConfigLen == 0)
	{
		return SL_EZEROLEN;
	}
    CmdExt.TxPayloadLen = 0;
    CmdExt.RxPayloadLen = *pConfigLen;
    CmdExt.pTxPayload = NULL;
    CmdExt.pRxPayload = (UINT8 *)pValues;
	CmdExt.ActualRxPayloadLen = 0;

    Msg.Cmd.ConfigId    = ConfigId;
    if( pConfigOpt )
    {
       Msg.Cmd.ConfigOpt   = (UINT16)*pConfigOpt;
    }
    VERIFY_RET_OK(_SlDrvCmdOp((_SlCmdCtrl_t *)&_SlWlanCfgGetCmdCtrl, &Msg, &CmdExt));
    
     if( pConfigOpt )
     {
       *pConfigOpt = (UINT8)Msg.Rsp.ConfigOpt;
     }
	 if (CmdExt.RxPayloadLen < CmdExt.ActualRxPayloadLen) 
	 {
		*pConfigLen = (UINT8)CmdExt.RxPayloadLen;
		return SL_ESMALLBUF;
	 }
	else
    {
		*pConfigLen = (UINT8)CmdExt.ActualRxPayloadLen;
	}
	 

    return (int)Msg.Rsp.Status;
}
#endif
