//
// Compiler Includes
//
#include "stdio.h"
#include "string.h"
#include "stdint.h"

//
// HW Includes
//
#include "stm32f4xx.h"
#include "stm32f4xx_usart.h"

//
// OS Includes
//
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

//
// Common Includes
//
#include "syscalls.h"
#include "leds.h"

//
// Application
//
#include "communication.h"

//
// OS variables
//
xQueueHandle xCommunication_Queue;

//
// File Variables
//
static vCommunicationFunctionPointer communication_packet_list[COMMUNICATION_MAX_PACKET_TYPE];

//
// Packet Handler Functions
//
static void prvPingPacketHandler(xPacket_Type *pkt);
static xCommunication_Error_Type prvAddCommunicationPacketHandler(vCommunicationFunctionPointer handler, uint8_t index);
static void prvSendPacket(xPacket_Type *packet);

/*******************************************************************************
 *******************************************************************************/
static void prvPingPacketHandler(xPacket_Type *pkt)
{
    LEDS_Toggle(GREEN2);
    
    return;    
}

/*******************************************************************************
 *******************************************************************************/
static xCommunication_Error_Type
prvAddCommunicationPacketHandler(vCommunicationFunctionPointer handler, uint8_t index)
{
    if (index > COMMUNICATION_MAX_PACKET_TYPE){
	return ERROR_INDEX;     
    }else{
	communication_packet_list[index] = handler;         
    }
    
    return ERROR_NONE;    
}

/*******************************************************************************
 *******************************************************************************/
void vPacket_Init(xPacket_Type *packet)
{
    uint32_t i;
    
    packet->type = 0;
    packet->size = 0;
    for (i=0; i<COMMUNICATION_MAX_PACKET_SIZE; i++){
	packet->data[i] =0;     
    }    
    packet->crc =0;
    
    return;    
}

/*******************************************************************************
 *******************************************************************************/
static void prvSendPingPacket(void)
{
    xPacket_Type packet;
    vPacket_Init(&packet);
    prvSendPacket(&packet);
    
    return;    
}

/*******************************************************************************
 *******************************************************************************/
static void prvSendPacket(xPacket_Type *packet)
{
    uint8_t i;
    
    uartPutch(packet->type);
    uartPutch(packet->size);
    for (i=0; i< packet->size; i++){
	uartPutch(packet->data[i]);
    }    
    uartPutch(packet->type);

    uartPutch( (packet->crc & 0x000000FF) );    
    uartPutch( (packet->crc & 0x0000FF00) >> 8 );    
    uartPutch( (packet->crc & 0x00FF0000) >> 16 );    
    uartPutch( (packet->crc & 0xFF000000) >> 24);
    
    return;    
}

uint8_t charGetByte(void)
{
    uint8_t retval;
    
    //
    // Wait for character to arrive
    //
    while (USART_GetFlagStatus(USART2,  USART_FLAG_RXNE) != SET);

    retval = USART_ReceiveData(USART2) & 0x00FF;    
    
    return retval;
    
}


/*******************************************************************************
 *******************************************************************************/
void vCommunication_Task(void * pvParameters)
{
    uint32_t i;    
    xCommunication_Message xMessage;
    xPacket_Type xPacket;
    xCommunication_Error_Type error;
    uint16_t data[8];
   
    //
    // Get rid of compiler warnings
    //
    UNUSED(pvParameters);

    //
    // Initialize our packet to a good default value
    // 
    vPacket_Init(&xPacket);
    
    //
    // Clear all packet handlers in case nothing gets installed
    //
    for (i=0; i< COMMUNICATION_MAX_PACKET_TYPE; i++){
	communication_packet_list[i] = NULL;    
    }
        
    //
    // Install our packet handlers
    //
    prvAddCommunicationPacketHandler(prvPingPacketHandler, COMMUNICATION_PACKET_PING);       
    
//
// Our main task loop
//
    for (;;){
//
// Initialize our message packet for this loop
//
	vPacket_Init(&xMessage.packet);    

//
// Wait for the message to indicate we need to act
//
	while( xQueueReceive( xCommunication_Queue, &xMessage, portMAX_DELAY ) != pdPASS );

	if (xMessage.tx_or_rx == TRANSMIT_DATA){
//
// Transmit the packet
//
	}else{
//
// Copy the buffer with the packet
//  
	    ( void ) memcpy( ( void * ) &xPacket , ( void * ) &xMessage.packet, ( unsigned ) sizeof(xPacket) );

//
// TODO: Check the CRC
//
            
//
// Check if handler is installed and call it
//
	    if ((xPacket.type < COMMUNICATION_MAX_PACKET_TYPE)  &&
		(communication_packet_list[xPacket.type] != (NULL)))
	    {

		//
		// Call Handler!
		//
		(communication_packet_list[xPacket.type])(&xPacket);       

	    }else{
		
	    }            
	}
        
    }    

    return;    
}
