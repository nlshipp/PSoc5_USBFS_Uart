/* ========================================
 *
 * Copyright YOUR COMPANY, THE YEAR
 * All Rights Reserved
 * UNPUBLISHED, LICENSED SOFTWARE.
 *
 * CONFIDENTIAL AND PROPRIETARY INFORMATION
 * WHICH IS THE PROPERTY OF your company.
 *
 * ========================================
*/

#include "USBUART_pvt.h"
#include "cyapicallbacks.h"

/******************************************************************************
*  USB Microsoft OS String Descriptor
*  "MSFT" identifies a Microsoft host
*  "100" specifies version 1.00
*  USBUART_GET_EXTENDED_CONFIG_DESCRIPTOR becomes the bRequest value
*  in a host vendor device/class request
******************************************************************************/

const uint8 CYCODE USBUART_MSOS_DESCRIPTOR_EX[USBUART_MSOS_DESCRIPTOR_LENGTH] = {
/* Descriptor Length                       */   0x12u,
/* DescriptorType: STRING                  */   0x03u,
/* qwSignature - "MSFT100"                 */   (uint8)'M', 0u, (uint8)'S', 0u, (uint8)'F', 0u, (uint8)'T', 0u,
                                                (uint8)'1', 0u, (uint8)'0', 0u, (uint8)'0', 0u,
/* bMS_VendorCode:                         */   USBUART_GET_EXTENDED_CONFIG_DESCRIPTOR,
/* bPad                                    */   0x00u
};

/* Extended Configuration Descriptor */

const uint8 CYCODE USBUART_MSOS_CONFIGURATION_DESCR_EX[USBUART_MSOS_CONF_DESCR_LENGTH] = {
/*  Length of the descriptor 4 bytes       */   0x28u, 0x00u, 0x00u, 0x00u,
/*  Version of the descriptor 2 bytes      */   0x00u, 0x01u,
/*  wIndex - Fixed:INDEX_CONFIG_DESCRIPTOR */   0x04u, 0x00u,
/*  bCount - Count of device functions.    */   0x01u,
/*  Reserved : 7 bytes                     */   0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
/*  bFirstInterfaceNumber                  */   0x00u,
/*  Reserved                               */   0x01u,
/*  compatibleID    - "CYUSB\0\0"          */   (uint8)'W', (uint8)'I', (uint8)'N', (uint8)'U', (uint8)'S', (uint8)'B',
                                                0x00u, 0x00u,
/*  subcompatibleID - "00001\0\0"          */   (uint8)'0', (uint8)'0', (uint8)'0', (uint8)'0', (uint8)'1',
                                                0x00u, 0x00u, 0x00u,
/*  Reserved : 6 bytes                     */   0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u
};

/*******************************************************************************
* Function Name: USBUART_HandleVendorRqst
****************************************************************************//**
*
*  This routine provide users with a method to implement vendor specific
*  requests.
*
*  To implement vendor specific requests, add your code in this function to
*  decode and disposition the request.  If the request is handled, your code
*  must set the variable "requestHandled" to TRUE, indicating that the
*  request has been handled.
*
* \return
*  requestHandled.
*
* \reentrant
*  No.
*
*******************************************************************************/
uint8 USBUART_HandleVendorRqst_EX(void) 
{
    uint8 requestHandled = USBUART_FALSE;

    /* Check request direction: D2H or H2D. */
    if (0u != (USBUART_bmRequestTypeReg & USBUART_RQST_DIR_D2H))
    {
        /* Handle direction from device to host. */
        
        switch (USBUART_bRequestReg)
        {
            case USBUART_GET_EXTENDED_CONFIG_DESCRIPTOR:
            #if defined(USBUART_ENABLE_MSOS_STRING)
                USBUART_currentTD.pData = (volatile uint8 *) &USBUART_MSOS_CONFIGURATION_DESCR_EX[0u];
                USBUART_currentTD.count = USBUART_MSOS_CONFIGURATION_DESCR_EX[0u];
                requestHandled  = USBUART_InitControlRead();
            #endif /* (USBUART_ENABLE_MSOS_STRING) */
                break;
            
            default:
                break;
        }
    }

    /* `#START VENDOR_SPECIFIC_CODE` Place your vendor specific request here */

    /* `#END` */

#ifdef USBUART_HANDLE_VENDOR_RQST_CALLBACK
    if (USBUART_FALSE == requestHandled)
    {
        requestHandled = USBUART_HandleVendorRqst_Callback();
    }
#endif /* (USBUART_HANDLE_VENDOR_RQST_CALLBACK) */

    return (requestHandled);
}


/*******************************************************************************
* Function Name: USBUART_HandleStandardRqst
****************************************************************************//**
*
*  This Routine dispatches standard requests
*
*
* \return
*  TRUE if request handled.
*
* \reentrant
*  No.
*
*******************************************************************************/
uint8 USBUART_HandleStandardRqst_EX(void) 
{
    uint8 requestHandled = USBUART_FALSE;
    uint8 interfaceNumber;
    uint8 configurationN;
    uint8 bmRequestType = USBUART_bmRequestTypeReg;

#if defined(USBUART_ENABLE_STRINGS)
    volatile uint8 *pStr = 0u;
    #if defined(USBUART_ENABLE_DESCRIPTOR_STRINGS)
        uint8 nStr;
        uint8 descrLength;
    #endif /* (USBUART_ENABLE_DESCRIPTOR_STRINGS) */
#endif /* (USBUART_ENABLE_STRINGS) */
    
    static volatile uint8 USBUART_tBuffer[USBUART_STATUS_LENGTH_MAX];
    const T_USBUART_LUT CYCODE *pTmp;

    USBUART_currentTD.count = 0u;

    if (USBUART_RQST_DIR_D2H == (bmRequestType & USBUART_RQST_DIR_MASK))
    {
        /* Control Read */
        switch (USBUART_bRequestReg)
        {
            case USBUART_GET_DESCRIPTOR:
                if (USBUART_DESCR_DEVICE ==USBUART_wValueHiReg)
                {
                    pTmp = USBUART_GetDeviceTablePtr();
                    USBUART_currentTD.pData = (volatile uint8 *)pTmp->p_list;
                    USBUART_currentTD.count = USBUART_DEVICE_DESCR_LENGTH;
                    
                    requestHandled  = USBUART_InitControlRead();
                }
                else if (USBUART_DESCR_CONFIG == USBUART_wValueHiReg)
                {
                    pTmp = USBUART_GetConfigTablePtr((uint8) USBUART_wValueLoReg);
                    
                    /* Verify that requested descriptor exists */
                    if (pTmp != NULL)
                    {
                        USBUART_currentTD.pData = (volatile uint8 *)pTmp->p_list;
                        USBUART_currentTD.count = (uint16)((uint16)(USBUART_currentTD.pData)[USBUART_CONFIG_DESCR_TOTAL_LENGTH_HI] << 8u) | \
                                                                            (USBUART_currentTD.pData)[USBUART_CONFIG_DESCR_TOTAL_LENGTH_LOW];
                        requestHandled  = USBUART_InitControlRead();
                    }
                }
                
            #if(USBUART_BOS_ENABLE)
                else if (USBUART_DESCR_BOS == USBUART_wValueHiReg)
                {
                    pTmp = USBUART_GetBOSPtr();
                    
                    /* Verify that requested descriptor exists */
                    if (pTmp != NULL)
                    {
                        USBUART_currentTD.pData = (volatile uint8 *)pTmp;
                        USBUART_currentTD.count = ((uint16)((uint16)(USBUART_currentTD.pData)[USBUART_BOS_DESCR_TOTAL_LENGTH_HI] << 8u)) | \
                                                                             (USBUART_currentTD.pData)[USBUART_BOS_DESCR_TOTAL_LENGTH_LOW];
                        requestHandled  = USBUART_InitControlRead();
                    }
                }
            #endif /*(USBUART_BOS_ENABLE)*/
            
            #if defined(USBUART_ENABLE_STRINGS)
                else if (USBUART_DESCR_STRING == USBUART_wValueHiReg)
                {
                /* Descriptor Strings */
                #if defined(USBUART_ENABLE_DESCRIPTOR_STRINGS)
                    nStr = 0u;
                    pStr = (volatile uint8 *) &USBUART_STRING_DESCRIPTORS[0u];
                    
                    while ((USBUART_wValueLoReg > nStr) && (*pStr != 0u))
                    {
                        /* Read descriptor length from 1st byte */
                        descrLength = *pStr;
                        /* Move to next string descriptor */
                        pStr = &pStr[descrLength];
                        nStr++;
                    }
                #endif /* (USBUART_ENABLE_DESCRIPTOR_STRINGS) */
                
                /* Microsoft OS String */
                #if defined(USBUART_ENABLE_MSOS_STRING)
                    if (USBUART_STRING_MSOS == USBUART_wValueLoReg)
                    {
                        pStr = (volatile uint8 *)& USBUART_MSOS_DESCRIPTOR_EX[0u];
                    }
                #endif /* (USBUART_ENABLE_MSOS_STRING) */
                
                /* SN string */
                #if defined(USBUART_ENABLE_SN_STRING)
                    if ((USBUART_wValueLoReg != 0u) && 
                        (USBUART_wValueLoReg == USBUART_DEVICE0_DESCR[USBUART_DEVICE_DESCR_SN_SHIFT]))
                    {
                    #if defined(USBUART_ENABLE_IDSN_STRING)
                        /* Read DIE ID and generate string descriptor in RAM */
                        USBUART_ReadDieID(USBUART_idSerialNumberStringDescriptor);
                        pStr = USBUART_idSerialNumberStringDescriptor;
                    #elif defined(USBUART_ENABLE_FWSN_STRING)
                        
                        if(USBUART_snStringConfirm != USBUART_FALSE)
                        {
                            pStr = USBUART_fwSerialNumberStringDescriptor;
                        }
                        else
                        {
                            pStr = (volatile uint8 *)&USBUART_SN_STRING_DESCRIPTOR[0u];
                        }
                    #else
                        pStr = (volatile uint8 *)&USBUART_SN_STRING_DESCRIPTOR[0u];
                    #endif  /* (USBUART_ENABLE_IDSN_STRING) */
                    }
                #endif /* (USBUART_ENABLE_SN_STRING) */
                
                    if (*pStr != 0u)
                    {
                        USBUART_currentTD.count = *pStr;
                        USBUART_currentTD.pData = pStr;
                        requestHandled  = USBUART_InitControlRead();
                    }
                }
            #endif /*  USBUART_ENABLE_STRINGS */
                else
                {
                    requestHandled = USBUART_DispatchClassRqst();
                }
                break;
                
            case USBUART_GET_STATUS:
                switch (bmRequestType & USBUART_RQST_RCPT_MASK)
                {
                    case USBUART_RQST_RCPT_EP:
                        USBUART_currentTD.count = USBUART_EP_STATUS_LENGTH;
                        USBUART_tBuffer[0u]     = USBUART_EP[USBUART_wIndexLoReg & USBUART_DIR_UNUSED].hwEpState;
                        USBUART_tBuffer[1u]     = 0u;
                        USBUART_currentTD.pData = &USBUART_tBuffer[0u];
                        
                        requestHandled  = USBUART_InitControlRead();
                        break;
                    case USBUART_RQST_RCPT_DEV:
                        USBUART_currentTD.count = USBUART_DEVICE_STATUS_LENGTH;
                        USBUART_tBuffer[0u]     = USBUART_deviceStatus;
                        USBUART_tBuffer[1u]     = 0u;
                        USBUART_currentTD.pData = &USBUART_tBuffer[0u];
                        
                        requestHandled  = USBUART_InitControlRead();
                        break;
                    default:    /* requestHandled is initialized as FALSE by default */
                        break;
                }
                break;
                
            case USBUART_GET_CONFIGURATION:
                USBUART_currentTD.count = 1u;
                USBUART_currentTD.pData = (volatile uint8 *) &USBUART_configuration;
                requestHandled  = USBUART_InitControlRead();
                break;
                
            case USBUART_GET_INTERFACE:
                USBUART_currentTD.count = 1u;
                USBUART_currentTD.pData = (volatile uint8 *) &USBUART_interfaceSetting[USBUART_wIndexLoReg];
                requestHandled  = USBUART_InitControlRead();
                break;
                
            default: /* requestHandled is initialized as FALSE by default */
                break;
        }
    }
    else
    {
        /* Control Write */
        switch (USBUART_bRequestReg)
        {
            case USBUART_SET_ADDRESS:
                /* Store address to be set in USBUART_NoDataControlStatusStage(). */
                USBUART_deviceAddress = (uint8) USBUART_wValueLoReg;
                requestHandled = USBUART_InitNoDataControlTransfer();
                break;
                
            case USBUART_SET_CONFIGURATION:
                configurationN = USBUART_wValueLoReg;
                
                /* Verify that configuration descriptor exists */
                if(configurationN > 0u)
                {
                    pTmp = USBUART_GetConfigTablePtr((uint8) configurationN - 1u);
                }
                
                /* Responds with a Request Error when configuration number is invalid */
                if (((configurationN > 0u) && (pTmp != NULL)) || (configurationN == 0u))
                {
                    /* Set new configuration if it has been changed */
                    if(configurationN != USBUART_configuration)
                    {
                        USBUART_configuration = (uint8) configurationN;
                        USBUART_configurationChanged = USBUART_TRUE;
                        USBUART_Config(USBUART_TRUE);
                    }
                    requestHandled = USBUART_InitNoDataControlTransfer();
                }
                break;
                
            case USBUART_SET_INTERFACE:
                if (0u != USBUART_ValidateAlternateSetting())
                {
                    /* Get interface number from the request. */
                    interfaceNumber = USBUART_wIndexLoReg;
                    USBUART_interfaceNumber = (uint8) USBUART_wIndexLoReg;
                     
                    /* Check if alternate settings is changed for interface. */
                    if (USBUART_interfaceSettingLast[interfaceNumber] != USBUART_interfaceSetting[interfaceNumber])
                    {
                        USBUART_configurationChanged = USBUART_TRUE;
                    
                        /* Change alternate setting for the endpoints. */
                    #if (USBUART_EP_MANAGEMENT_MANUAL && USBUART_EP_ALLOC_DYNAMIC)
                        USBUART_Config(USBUART_FALSE);
                    #else
                        USBUART_ConfigAltChanged();
                    #endif /* (USBUART_EP_MANAGEMENT_MANUAL && USBUART_EP_ALLOC_DYNAMIC) */
                    }
                    
                    requestHandled = USBUART_InitNoDataControlTransfer();
                }
                break;
                
            case USBUART_CLEAR_FEATURE:
                switch (bmRequestType & USBUART_RQST_RCPT_MASK)
                {
                    case USBUART_RQST_RCPT_EP:
                        if (USBUART_wValueLoReg == USBUART_ENDPOINT_HALT)
                        {
                            requestHandled = USBUART_ClearEndpointHalt();
                        }
                        break;
                    case USBUART_RQST_RCPT_DEV:
                        /* Clear device REMOTE_WAKEUP */
                        if (USBUART_wValueLoReg == USBUART_DEVICE_REMOTE_WAKEUP)
                        {
                            USBUART_deviceStatus &= (uint8)~USBUART_DEVICE_STATUS_REMOTE_WAKEUP;
                            requestHandled = USBUART_InitNoDataControlTransfer();
                        }
                        break;
                    case USBUART_RQST_RCPT_IFC:
                        /* Validate interfaceNumber */
                        if (USBUART_wIndexLoReg < USBUART_MAX_INTERFACES_NUMBER)
                        {
                            USBUART_interfaceStatus[USBUART_wIndexLoReg] &= (uint8) ~USBUART_wValueLoReg;
                            requestHandled = USBUART_InitNoDataControlTransfer();
                        }
                        break;
                    default:    /* requestHandled is initialized as FALSE by default */
                        break;
                }
                break;
                
            case USBUART_SET_FEATURE:
                switch (bmRequestType & USBUART_RQST_RCPT_MASK)
                {
                    case USBUART_RQST_RCPT_EP:
                        if (USBUART_wValueLoReg == USBUART_ENDPOINT_HALT)
                        {
                            requestHandled = USBUART_SetEndpointHalt();
                        }
                        break;
                        
                    case USBUART_RQST_RCPT_DEV:
                        /* Set device REMOTE_WAKEUP */
                        if (USBUART_wValueLoReg == USBUART_DEVICE_REMOTE_WAKEUP)
                        {
                            USBUART_deviceStatus |= USBUART_DEVICE_STATUS_REMOTE_WAKEUP;
                            requestHandled = USBUART_InitNoDataControlTransfer();
                        }
                        break;
                        
                    case USBUART_RQST_RCPT_IFC:
                        /* Validate interfaceNumber */
                        if (USBUART_wIndexLoReg < USBUART_MAX_INTERFACES_NUMBER)
                        {
                            USBUART_interfaceStatus[USBUART_wIndexLoReg] &= (uint8) ~USBUART_wValueLoReg;
                            requestHandled = USBUART_InitNoDataControlTransfer();
                        }
                        break;
                    
                    default:    /* requestHandled is initialized as FALSE by default */
                        break;
                }
                break;
                
            default:    /* requestHandled is initialized as FALSE by default */
                break;
        }
    }
    
    return (requestHandled);
}


/*******************************************************************************
* Function Name: USBUART_HandleSetup
****************************************************************************//**
*
*  This Routine dispatches requests for the four USB request types
*
*
* \reentrant
*  No.
*
*******************************************************************************/
void USBUART_HandleSetup_EX(void) 
{
    uint8 requestHandled;
    
    /* Clear register lock by SIE (read register) and clear setup bit 
    * (write any value in register).
    */
    requestHandled = (uint8) USBUART_EP0_CR_REG;
    USBUART_EP0_CR_REG = (uint8) requestHandled;
    requestHandled = (uint8) USBUART_EP0_CR_REG;

    if ((requestHandled & USBUART_MODE_SETUP_RCVD) != 0u)
    {
        /* SETUP bit is set: exit without mode modification. */
        USBUART_ep0Mode = requestHandled;
    }
    else
    {
        /* In case the previous transfer did not complete, close it out */
        USBUART_UpdateStatusBlock(USBUART_XFER_PREMATURE);

        /* Check request type. */
        switch (USBUART_bmRequestTypeReg & USBUART_RQST_TYPE_MASK)
        {
            case USBUART_RQST_TYPE_STD:
                requestHandled = USBUART_HandleStandardRqst_EX();
                break;
                
            case USBUART_RQST_TYPE_CLS:
                requestHandled = USBUART_DispatchClassRqst();
                break;
                
            case USBUART_RQST_TYPE_VND:
                requestHandled = USBUART_HandleVendorRqst_EX();
                break;
                
            default:
                requestHandled = USBUART_FALSE;
                break;
        }
        
        /* If request is not recognized. Stall endpoint 0 IN and OUT. */
        if (requestHandled == USBUART_FALSE)
        {
            USBUART_ep0Mode = USBUART_MODE_STALL_IN_OUT;
        }
    }
}


/*******************************************************************************
* Function Name: USBUART_ep_0_Interrupt
****************************************************************************//**
*
*  This Interrupt Service Routine handles Endpoint 0 (Control Pipe) traffic.
*  It dispatches setup requests and handles the data and status stages.
*
*
*******************************************************************************/
CY_ISR(USBUART_EP_0_ISR_EX)
{
    uint8 tempReg;
    uint8 modifyReg;

#ifdef USBUART_EP_0_ISR_ENTRY_CALLBACK
    USBUART_EP_0_ISR_EntryCallback();
#endif /* (USBUART_EP_0_ISR_ENTRY_CALLBACK) */
    
    tempReg = USBUART_EP0_CR_REG;
    if ((tempReg & USBUART_MODE_ACKD) != 0u)
    {
        modifyReg = 1u;
        if ((tempReg & USBUART_MODE_SETUP_RCVD) != 0u)
        {
            if ((tempReg & USBUART_MODE_MASK) != USBUART_MODE_NAK_IN_OUT)
            {
                /* Mode not equal to NAK_IN_OUT: invalid setup */
                modifyReg = 0u;
            }
            else
            {
                USBUART_HandleSetup_EX();
                
                if ((USBUART_ep0Mode & USBUART_MODE_SETUP_RCVD) != 0u)
                {
                    /* SETUP bit set: exit without mode modificaiton */
                    modifyReg = 0u;
                }
            }
        }
        else if ((tempReg & USBUART_MODE_IN_RCVD) != 0u)
        {
            USBUART_HandleIN();
        }
        else if ((tempReg & USBUART_MODE_OUT_RCVD) != 0u)
        {
            USBUART_HandleOUT();
        }
        else
        {
            modifyReg = 0u;
        }
        
        /* Modify the EP0_CR register */
        if (modifyReg != 0u)
        {
            
            tempReg = USBUART_EP0_CR_REG;
            
            /* Make sure that SETUP bit is cleared before modification */
            if ((tempReg & USBUART_MODE_SETUP_RCVD) == 0u)
            {
                /* Update count register */
                tempReg = (uint8) USBUART_ep0Toggle | USBUART_ep0Count;
                USBUART_EP0_CNT_REG = tempReg;
               
                /* Make sure that previous write operaiton was successful */
                if (tempReg == USBUART_EP0_CNT_REG)
                {
                    /* Repeat until next successful write operation */
                    do
                    {
                        /* Init temporary variable */
                        modifyReg = USBUART_ep0Mode;
                        
                        /* Unlock register */
                        tempReg = (uint8) (USBUART_EP0_CR_REG & USBUART_MODE_SETUP_RCVD);
                        
                        /* Check if SETUP bit is not set */
                        if (0u == tempReg)
                        {
                            /* Set the Mode Register  */
                            USBUART_EP0_CR_REG = USBUART_ep0Mode;
                            
                            /* Writing check */
                            modifyReg = USBUART_EP0_CR_REG & USBUART_MODE_MASK;
                        }
                    }
                    while (modifyReg != USBUART_ep0Mode);
                }
            }
        }
    }

    USBUART_ClearSieInterruptSource(USBUART_INTR_SIE_EP0_INTR);
	
#ifdef USBUART_EP_0_ISR_EXIT_CALLBACK
    USBUART_EP_0_ISR_ExitCallback();
#endif /* (USBUART_EP_0_ISR_EXIT_CALLBACK) */
}


/* [] END OF FILE */
