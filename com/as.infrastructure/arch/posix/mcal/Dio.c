/**
 * AS - the open source Automotive Software on https://github.com/parai
 *
 * Copyright (C) 2015  AS <parai@foxmail.com>
 *
 * This source code is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published by the
 * Free Software Foundation; See <http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt>.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 */
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include "Std_Types.h"
#include "Dio.h"
#include "Port.h"
#if defined(USE_DET)
#include "Det.h"
#endif



#define DIO_GET_PORT_FROM_CHANNEL_ID(_channelId) (_channelId / 8)
#define DIO_GET_BIT_FROM_CHANNEL_ID(_channelId) (1 << (_channelId % 8))
#define CHANNEL_PTR		(&DioChannelConfigData)
#define CHANNEL_GRP_PTR	(&DioConfigData)
#define PORT_PTR		(&DioPortConfigData)

// ================= Types

typedef struct
{
	uint8_t Level[DIO_PORT_NUM];
	uint8_t Direction[DIO_PORT_NUM];
}DioReg_Type;

#if ( DIO_DEV_ERROR_DETECT == STD_ON )
static int Channel_Config_Contains(Dio_ChannelType channelId)
{
	Dio_ChannelType* ch_ptr=(Dio_ChannelType*)CHANNEL_PTR;
	int rv=0;
	while ((Dio_ChannelType)DIO_END_OF_LIST!=*ch_ptr)
	{
		if (*ch_ptr==channelId)
		{
			rv=1;
			break;
		}
		ch_ptr++;
	}
	return rv;
}

static int Port_Config_Contains(Dio_PortType portId)
{
	Dio_PortType* port_ptr=(Dio_PortType*)PORT_PTR;
	int rv=0;
	while ((Dio_PortType)DIO_END_OF_LIST!=*port_ptr)
	{
		if (*port_ptr==portId)
		{
			rv=1;
			break;
		}
		port_ptr++;
	}
	return rv;
}

static int Channel_Group_Config_Contains(const Dio_ChannelGroupType* _channelGroupIdPtr)
{
	Dio_ChannelGroupType* chGrp_ptr=(Dio_ChannelGroupType*)CHANNEL_GRP_PTR;
	int rv=0;

	while ((Dio_PortType)DIO_END_OF_LIST!=chGrp_ptr->port)
	{
		if (chGrp_ptr->port==_channelGroupIdPtr->port
				&& chGrp_ptr->offset==_channelGroupIdPtr->offset
				&& chGrp_ptr->mask==_channelGroupIdPtr->mask)
		{
			rv=1;
			break;
		}
		chGrp_ptr++;
	}
	return rv;
}

#define VALIDATE_CHANNEL(_channelId, _api) \
	if(0==Channel_Config_Contains(channelId)) {	\
		Det_ReportError(MODULE_ID_DIO,0,_api,DIO_E_PARAM_INVALID_CHANNEL_ID ); \
		level = 0;	\
		goto cleanup;	\
		}
#define VALIDATE_PORT(_portId, _api)\
	if(0==Port_Config_Contains(_portId)) {\
		Det_ReportError(MODULE_ID_DIO,0,_api,DIO_E_PARAM_INVALID_PORT_ID ); \
		level = STD_LOW;\
		goto cleanup;\
	}
#define VALIDATE_CHANNELGROUP(_channelGroupIdPtr, _api)\
	if(0==Channel_Group_Config_Contains(_channelGroupIdPtr)) {\
		Det_ReportError(MODULE_ID_DIO,0,_api,DIO_E_PARAM_INVALID_GROUP_ID ); \
		level = STD_LOW;\
		goto cleanup;\
	}
#else
#define VALIDATE_CHANNEL(_channelId, _api)
#define VALIDATE_PORT(_portId, _api)
#define VALIDATE_CHANNELGROUP(_channelGroupIdPtr, _api)
#endif

static DioReg_Type Dio_Reg = {
	.Level = {0,0,0,0,0,0,0,0,0,0,0},
	.Direction = {0,0,0,0,0,0,0,0,0,0,0}
};

Dio_LevelType Dio_ReadChannel(Dio_ChannelType channelId)
{
	Dio_LevelType level;
	VALIDATE_CHANNEL(channelId, DIO_READCHANNEL_ID);

	Dio_PortLevelType portVal = Dio_ReadPort(DIO_GET_PORT_FROM_CHANNEL_ID(channelId));
	Dio_PortLevelType bit = DIO_GET_BIT_FROM_CHANNEL_ID(channelId);

	if ((portVal & bit) != STD_LOW){
		level = STD_HIGH;
	} else {
		level = STD_LOW;
	}

#if ( DIO_DEV_ERROR_DETECT == STD_ON )
	cleanup:
#endif
	return (level);
}
void exDio_SetPinDirection(uint32_t channelId,Port_PinDirectionType Direction)
{
	uint8_t portDir;
	uint8_t bit;
	if(channelId < Port_PIN_NUM)
	{
		portDir = Dio_Reg.Direction[DIO_GET_PORT_FROM_CHANNEL_ID(channelId)];
		bit = DIO_GET_BIT_FROM_CHANNEL_ID(channelId);
		if(PORT_PIN_IN == Direction)
		{
			portDir &= ~bit;
		}
		else
		{
			portDir |= bit;
		}
		Dio_Reg.Direction[DIO_GET_PORT_FROM_CHANNEL_ID(channelId)] = portDir;
	}
}
void Dio_WriteChannel(Dio_ChannelType channelId, Dio_LevelType level)
{
	VALIDATE_CHANNEL(channelId, DIO_WRITECHANNEL_ID);

	Dio_PortLevelType portVal = Dio_ReadPort(DIO_GET_PORT_FROM_CHANNEL_ID(channelId));
	Dio_PortLevelType bit = DIO_GET_BIT_FROM_CHANNEL_ID(channelId);

	if(level == STD_HIGH){
		portVal |= bit;
	} else {
		portVal &= ~bit;
	}

	Dio_WritePort(DIO_GET_PORT_FROM_CHANNEL_ID(channelId), portVal);

#if ( DIO_DEV_ERROR_DETECT == STD_ON )
	cleanup:
#endif
	return;
}

Dio_PortLevelType Dio_ReadPort(Dio_PortType portId)
{
	Dio_LevelType level = 0;
	VALIDATE_PORT(portId, DIO_READPORT_ID);

	level = Dio_Reg.Level[portId];

#if ( DIO_DEV_ERROR_DETECT == STD_ON )
	cleanup:
#endif
	return level;
}

void Dio_WritePort(Dio_PortType portId, Dio_PortLevelType level)
{
    VALIDATE_PORT(portId, DIO_WRITEPORT_ID);
    Dio_Reg.Level[portId] = (uint8_t)level;
#if ( DIO_DEV_ERROR_DETECT == STD_ON )
    cleanup:
#endif
    return;
}

Dio_PortLevelType Dio_ReadChannelGroup(
    const Dio_ChannelGroupType *channelGroupIdPtr)
{
	Dio_LevelType level;
	VALIDATE_CHANNELGROUP(channelGroupIdPtr,DIO_READCHANNELGROUP_ID);

	// Get masked values
	level = Dio_ReadPort(channelGroupIdPtr->port) & channelGroupIdPtr->mask;

	// Shift down
	level = level >> channelGroupIdPtr->offset;

#if ( DIO_DEV_ERROR_DETECT == STD_ON )
	cleanup:
#endif
	return level;
}

void Dio_WriteChannelGroup(const Dio_ChannelGroupType *channelGroupIdPtr,
    Dio_PortLevelType level)
{
	VALIDATE_CHANNELGROUP(channelGroupIdPtr,DIO_WRITECHANNELGROUP_ID);

	// Shift up and apply mask so that no unwanted bits are affected
	level = (level << channelGroupIdPtr->offset) & channelGroupIdPtr->mask;

	// Read port and clear out masked bits
	Dio_PortLevelType portVal = Dio_ReadPort(channelGroupIdPtr->port) & (~channelGroupIdPtr->mask);

	// Or in the upshifted masked level
	portVal |= level;

	Dio_WritePort(channelGroupIdPtr->port, portVal);

#if ( DIO_DEV_ERROR_DETECT == STD_ON )
	cleanup:
#endif
	return;
}
