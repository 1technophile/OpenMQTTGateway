//-----------------------------------------------------------------------------
//
//	CommandClass.cpp
//
//	Base class for all Z-Wave Command Classes
//
//	Copyright (c) 2010 Mal Lansell <openzwave@lansell.org>
//
//	SOFTWARE NOTICE AND LICENSE
//
//	This file is part of OpenZWave.
//
//	OpenZWave is free software: you can redistribute it and/or modify
//	it under the terms of the GNU Lesser General Public License as published
//	by the Free Software Foundation, either version 3 of the License,
//	or (at your option) any later version.
//
//	OpenZWave is distributed in the hope that it will be useful,
//	but WITHOUT ANY WARRANTY; without even the implied warranty of
//	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//	GNU Lesser General Public License for more details.
//
//	You should have received a copy of the GNU Lesser General Public License
//	along with OpenZWave.  If not, see <http://www.gnu.org/licenses/>.
//
//-----------------------------------------------------------------------------

#include <math.h>
#include <locale.h>
#include "../tinyxml.h"
#include "CommandClass.h"
#include "Basic.h"
#include "MultiInstance.h"
#include "CommandClasses.h"
#include "../Msg.h"
#include "../Node.h"
#include "../Driver.h"
#include "../Manager.h"
#include "../platform/Log.h"
#include "../value_classes/ValueStore.h"

using namespace OpenZWave;

static uint8 const	c_sizeMask		= 0x07;
static uint8 const	c_scaleMask		= 0x18;
static uint8 const	c_scaleShift		= 0x03;
static uint8 const	c_precisionMask		= 0xe0;
static uint8 const	c_precisionShift	= 0x05;

//-----------------------------------------------------------------------------
// <CommandClass::CommandClass>
// Constructor
//-----------------------------------------------------------------------------
CommandClass::CommandClass
(
	uint32 const _homeId,
	uint8 const _nodeId
):
	m_homeId( _homeId ),
	m_nodeId( _nodeId ),
	m_version( 1 ),
	m_afterMark( false ),
	m_createVars( true ),
	m_overridePrecision( -1 ),
	m_getSupported( true ),
	m_isSecured( false ),
	m_SecureSupport( true ),
	m_staticRequests( 0 ),
	m_sentCnt( 0 ),
	m_receivedCnt( 0 )
{
}

//-----------------------------------------------------------------------------
// <CommandClass::~CommandClass>
// Destructor
//-----------------------------------------------------------------------------
CommandClass::~CommandClass
(
)
{
	while( !m_endPointMap.empty() )
	{
		map<uint8,uint8>::iterator it = m_endPointMap.begin();
		m_endPointMap.erase( it );
	}
	while ( !m_RefreshClassValues.empty() )
	{
		for (unsigned int i = 0; i < m_RefreshClassValues.size(); i++)
		{
			RefreshValue *rcc = m_RefreshClassValues.at(i);
			while(!rcc->RefreshClasses.empty()) {
				delete rcc->RefreshClasses.back();
				rcc->RefreshClasses.pop_back();
			}
//			for (unsigned int j = 0; j < rcc->RefreshClasses.size(); i++)
//			{
//				delete rcc->RefreshClasses[j];
//			}
			rcc->RefreshClasses.clear();
			delete rcc;
		}
		m_RefreshClassValues.clear();
	}

}

//-----------------------------------------------------------------------------
// <CommandClass::GetDriver>
// Get a pointer to our driver
//-----------------------------------------------------------------------------
Driver* CommandClass::GetDriver
(
)const
{
	return( Manager::Get()->GetDriver( m_homeId ) );
}

//-----------------------------------------------------------------------------
// <CommandClass::GetNode>
// Get a pointer to our node without locking the mutex
//-----------------------------------------------------------------------------
Node* CommandClass::GetNodeUnsafe
(
)const
{
	return( GetDriver()->GetNodeUnsafe( m_nodeId ) );
}

//-----------------------------------------------------------------------------
// <CommandClass::GetValue>
// Get a pointer to a value by its instance and index
//-----------------------------------------------------------------------------
Value* CommandClass::GetValue
(
	uint8 const _instance,
	uint8 const _index
)
{
	Value* value = NULL;
	if( Node* node = GetNodeUnsafe() )
	{
		value = node->GetValue( GetCommandClassId(), _instance, _index );
	}
	return value;
}

//-----------------------------------------------------------------------------
// <CommandClass::RemoveValue>
// Remove a value by its instance and index
//-----------------------------------------------------------------------------
bool CommandClass::RemoveValue
(
	uint8 const _instance,
	uint8 const _index
)
{
	if( Node* node = GetNodeUnsafe() )
	{
		return node->RemoveValue( GetCommandClassId(), _instance, _index );
	}
	return false;
}

//-----------------------------------------------------------------------------
// <CommandClass::SetInstances>
// Instances as set by the MultiInstance V1 command class
//-----------------------------------------------------------------------------
void CommandClass::SetInstances
(
	uint8 const _instances
)
{
	// Ensure we have a set of reported variables for each new instance
	if( !m_afterMark )
	{
		for( uint8 i=0; i<_instances; ++i )
		{
			SetInstance( i+1 );
		}
	}
}

//-----------------------------------------------------------------------------
// <CommandClass::SetInstance>
// Instances as set by the MultiChannel (i.e. MultiInstance V2) command class
//-----------------------------------------------------------------------------
void CommandClass::SetInstance
(
	uint8 const _endPoint
)
{
	if( !m_instances.IsSet( _endPoint ) )
	{
		m_instances.Set( _endPoint );
		if( IsCreateVars() )
		{
			CreateVars( _endPoint );
		}
	}
}

//-----------------------------------------------------------------------------
// <CommandClass::ReadXML>
// Read the saved command class data
//-----------------------------------------------------------------------------
void CommandClass::ReadXML
(
	TiXmlElement const* _ccElement
)
{
	int32 intVal;
	char const* str;

	if( TIXML_SUCCESS == _ccElement->QueryIntAttribute( "version", &intVal ) )
	{
		m_version = (uint8)intVal;
	}

	uint8 instances = 1;
	if( TIXML_SUCCESS == _ccElement->QueryIntAttribute( "instances", &intVal ) )
	{
		instances = (uint8)intVal;
	}

	if( TIXML_SUCCESS == _ccElement->QueryIntAttribute( "request_flags", &intVal ) )
	{
		m_staticRequests = (uint8)intVal;
	}

	if( TIXML_SUCCESS == _ccElement->QueryIntAttribute( "override_precision", &intVal ) )
	{
		m_overridePrecision = (int8)intVal;
	}

	str = _ccElement->Attribute( "after_mark" );
	if( str )
	{
		m_afterMark = !strcmp( str, "true" );
	}

	str = _ccElement->Attribute( "create_vars" );
	if( str )
	{
		m_createVars = !strcmp( str, "true" );
	}

	// Make sure previously created values are removed if create_vars=false
	if( !m_createVars )
	{
		if( Node* node = GetNodeUnsafe() )
		{
			node->GetValueStore()->RemoveCommandClassValues( GetCommandClassId() );
		}
	}

	str = _ccElement->Attribute( "getsupported" );
	if( str )
	{
		m_getSupported = !strcmp( str, "true" );
	}

	str = _ccElement->Attribute( "issecured" );
	if( str )
	{
		m_isSecured = !strcmp( str, "true" );
	}
	// Setting the instance count will create all the values.
	SetInstances( instances );

	// Apply any differences from the saved XML to the values
	TiXmlElement const* child = _ccElement->FirstChildElement();
	while( child )
	{
		str = child->Value();
		if( str )
		{
			if( !strcmp( str, "Instance" ) )
			{
				uint8 instance = 0;
				// Add an instance to the command class
				if( TIXML_SUCCESS == child->QueryIntAttribute( "index", &intVal ) )
				{
					instance = (uint8)intVal;
					SetInstance( instance );
				}
				// See if its associated endpoint is present
				if( TIXML_SUCCESS == child->QueryIntAttribute( "endpoint", &intVal ) )
				{
					uint8 endpoint = (uint8)intVal;
					SetEndPoint( instance, endpoint );
				}
			}
			else if( !strcmp( str, "Value" ) )
			{
				// Apply any differences from the saved XML to the value
				GetNodeUnsafe()->ReadValueFromXML( GetCommandClassId(), child );
			}
			else if (!strcmp( str, "TriggerRefreshValue" ) )
			{
				ReadValueRefreshXML(child);
			}
		}

		child = child->NextSiblingElement();
	}
}

//-----------------------------------------------------------------------------
// <CommandClass::ReadValueRefreshXML>
// Read the config that contains a list of Values that should be refreshed when
// we recieve u updated Value from a device. (This helps Yale Door Locks, which send a
// Alarm Report instead of DoorLock Report when the status of the Door Lock is changed
//-----------------------------------------------------------------------------
void CommandClass::ReadValueRefreshXML
(
	TiXmlElement const* _ccElement
)
{

	char const* str;
	bool ok = false;
	const char *genre;
	RefreshValue *rcc = new RefreshValue();
	rcc->cc = GetCommandClassId();
	genre = _ccElement->Attribute( "Genre" );
	rcc->genre = Value::GetGenreEnumFromName(genre);
	_ccElement->QueryIntAttribute( "Instance", (int*)&rcc->instance);
	_ccElement->QueryIntAttribute( "Index", (int*)&rcc->index);
	Log::Write(LogLevel_Info, GetNodeId(), "Value Refresh triggered by CommandClass: %s, Genre: %d, Instance: %d, Index: %d for:", GetCommandClassName().c_str(), rcc->genre, rcc->instance, rcc->index);
	TiXmlElement const* child = _ccElement->FirstChildElement();
	while( child )
	{
		str = child->Value();
		if( str )
		{
			if ( !strcmp(str, "RefreshClassValue"))
			{
				RefreshValue *arcc = new RefreshValue();
				if (child->QueryIntAttribute( "CommandClass", (int*)&arcc->cc) != TIXML_SUCCESS) {
					Log::Write(LogLevel_Warning, GetNodeId(), "    Invalid XML - CommandClass Attribute is wrong type or missing");
					continue;
				}
				if (child->QueryIntAttribute( "RequestFlags", (int*)&arcc->genre) != TIXML_SUCCESS) {
					Log::Write(LogLevel_Warning, GetNodeId(), "    Invalid XML - RequestFlags Attribute is wrong type or missing");
					continue;
				}
				if (child->QueryIntAttribute( "Instance", (int*)&arcc->instance) != TIXML_SUCCESS) {
					Log::Write(LogLevel_Warning, GetNodeId(), "    Invalid XML - Instance Attribute is wrong type or missing");
					continue;
				}
				if (child->QueryIntAttribute( "Index", (int*)&arcc->index) != TIXML_SUCCESS) {
					Log::Write(LogLevel_Warning, GetNodeId(), "    Invalid XML - Index Attribute is wrong type or missing");
					continue;
				}
				Log::Write(LogLevel_Info, GetNodeId(), "    CommandClass: %s, RequestFlags: %d, Instance: %d, Index: %d", CommandClasses::GetName(arcc->cc).c_str(), arcc->genre, arcc->instance, arcc->index);
				rcc->RefreshClasses.push_back(arcc);
				ok = true;
			}
			else
			{
				Log::Write(LogLevel_Warning, GetNodeId(), "Got Unhandled Child Entry in TriggerRefreshValue XML Config: %s", str);
			}
		}
		child = child->NextSiblingElement();
	}
	if (ok == true) {
		m_RefreshClassValues.push_back( rcc );
	} else {
		Log::Write(LogLevel_Warning, GetNodeId(), "Failed to add a RefreshClassValue from XML");
		delete rcc;
	}
}

//-----------------------------------------------------------------------------
// <CommandClass::CheckForRefreshValues>
// Scan our m_RefreshClassValues vector to see if we should call any other
// Command Classes to refresh their value
//-----------------------------------------------------------------------------

bool CommandClass::CheckForRefreshValues (
		Value const* _value
)
{
	if (m_RefreshClassValues.empty())
	{
		//Log::Write(LogLevel_Debug, GetNodeId(), "Bailing out of CheckForRefreshValues");
		return false;
	}
	Node* node = GetNodeUnsafe();
	if( node != NULL )
	{
		for (uint32 i = 0; i < m_RefreshClassValues.size(); i++)
		{
			RefreshValue *rcc = m_RefreshClassValues.at(i);
			//Log::Write(LogLevel_Debug, GetNodeId(), "Checking Value Against RefreshClassList: CommandClass %s = %s, Genre %d = %d, Instance %d = %d, Index %d = %d", CommandClasses::GetName(rcc->cc).c_str(), CommandClasses::GetName(_value->GetID().GetCommandClassId()).c_str(), rcc->genre, _value->GetID().GetGenre(), rcc->instance, _value->GetID().GetInstance(), rcc->index, _value->GetID().GetIndex());
			if ((rcc->genre == _value->GetID().GetGenre()) && (rcc->instance == _value->GetID().GetInstance()) && (rcc->index == _value->GetID().GetIndex()) )
			{
				/* we got a match..... */
				for (uint32 j = 0; j < rcc->RefreshClasses.size(); j++)
				{
					RefreshValue *arcc = rcc->RefreshClasses.at(j);
					Log::Write(LogLevel_Debug, GetNodeId(), "Requesting Refresh of Value: CommandClass: %s Genre %d, Instance %d, Index %d", CommandClasses::GetName(arcc->cc).c_str(), arcc->genre, arcc->instance, arcc->index);
					if( CommandClass* cc = node->GetCommandClass( arcc->cc ) )
					{
						cc->RequestValue(arcc->genre, arcc->index, arcc->instance, Driver::MsgQueue_Send);
					}
				}
			}
		}
	}
	else /* Driver */
	{
		Log::Write(LogLevel_Warning, GetNodeId(), "Can't get Node");
	}
	return true;
}

//-----------------------------------------------------------------------------
// <CommandClass::WriteXML>
// Save the static node configuration data
//-----------------------------------------------------------------------------
void CommandClass::WriteXML
(
	TiXmlElement* _ccElement
)
{
	char str[32];

	snprintf( str, sizeof(str), "%d", GetCommandClassId() );
	_ccElement->SetAttribute( "id", str );
	_ccElement->SetAttribute( "name", GetCommandClassName().c_str() );

	snprintf( str, sizeof(str), "%d", GetVersion() );
	_ccElement->SetAttribute( "version", str );

	if( m_staticRequests )
	{
		snprintf( str, sizeof(str), "%d", m_staticRequests );
		_ccElement->SetAttribute( "request_flags", str );
	}

	if( m_overridePrecision >= 0 )
	{
		snprintf( str, sizeof(str), "%d", m_overridePrecision );
		_ccElement->SetAttribute( "override_precision", str );
	}

	if( m_afterMark )
	{
		_ccElement->SetAttribute( "after_mark", "true" );
	}

	if( !m_createVars )
	{
		_ccElement->SetAttribute( "create_vars", "false" );
	}

	if( !m_getSupported )
	{
		_ccElement->SetAttribute( "getsupported", "false" );
	}
	if ( m_isSecured )
        {
                _ccElement->SetAttribute( "issecured", "true" );
        }


	// Write out the instances
	for( Bitfield::Iterator it = m_instances.Begin(); it != m_instances.End(); ++ it )
	{
		TiXmlElement* instanceElement = new TiXmlElement( "Instance" );
		_ccElement->LinkEndChild( instanceElement );

		snprintf( str, sizeof(str), "%d", *it );
		instanceElement->SetAttribute( "index", str );

		map<uint8,uint8>::iterator eit = m_endPointMap.find( *it );
		if( eit != m_endPointMap.end() )
		{
			snprintf( str, sizeof(str), "%d", eit->second );
			instanceElement->SetAttribute( "endpoint", str );
		}
	}

	// Write out the values for this command class
	ValueStore* store = GetNodeUnsafe()->GetValueStore();
	for( ValueStore::Iterator it = store->Begin(); it != store->End(); ++it )
	{
		Value* value = it->second;
		if( value->GetID().GetCommandClassId() == GetCommandClassId() )
		{
			TiXmlElement* valueElement = new TiXmlElement( "Value" );
			_ccElement->LinkEndChild( valueElement );
			value->WriteXML( valueElement );
		}
	}
	// Write out the TriggerRefreshValue if it exists
	for (uint32 i = 0; i < m_RefreshClassValues.size(); i++)
	{
		RefreshValue *rcc = m_RefreshClassValues.at(i);
		TiXmlElement* RefreshElement = new TiXmlElement("TriggerRefreshValue");
		_ccElement->LinkEndChild( RefreshElement );
		RefreshElement->SetAttribute("Genre", Value::GetGenreNameFromEnum((ValueID::ValueGenre)rcc->genre));
		RefreshElement->SetAttribute("Instance", rcc->instance);
		RefreshElement->SetAttribute("Index", rcc->index);
		for (uint32 j = 0; j < rcc->RefreshClasses.size(); j++)
		{
			RefreshValue *arcc = rcc->RefreshClasses.at(j);
			TiXmlElement *ClassElement = new TiXmlElement("RefreshClassValue");
			RefreshElement->LinkEndChild(ClassElement);
			ClassElement->SetAttribute("CommandClass", arcc->cc);
			ClassElement->SetAttribute("RequestFlags", arcc->genre);
			ClassElement->SetAttribute("Instance", arcc->instance);
			ClassElement->SetAttribute("Index", arcc->index);
		}
	}
}

//-----------------------------------------------------------------------------
// <CommandClass::ExtractValue>
// Read a value from a variable length sequence of bytes
//-----------------------------------------------------------------------------
string CommandClass::ExtractValue
(
	uint8 const* _data,
	uint8* _scale,
	uint8* _precision,
	uint8 _valueOffset // = 1
)const
{
	uint8 const size = _data[0] & c_sizeMask;
	uint8 const precision = (_data[0] & c_precisionMask) >> c_precisionShift;

	if( _scale )
	{
		*_scale = (_data[0] & c_scaleMask) >> c_scaleShift;
	}

	if( _precision )
	{
		*_precision = precision;
	}

	uint32 value = 0;
	uint8 i;
	for( i=0; i<size; ++i )
	{
		value <<= 8;
		value |= (uint32)_data[i+(uint32)_valueOffset];
	}

	// Deal with sign extension.  All values are signed
	string res;
	if( _data[_valueOffset] & 0x80 )
	{
		res = "-";

		// MSB is signed
		if( size == 1 )
		{
			value |= 0xffffff00;
		}
		else if( size == 2 )
		{
			value |= 0xffff0000;
		}
	}

	// Convert the integer to a decimal string.  We avoid
	// using floats to prevent accuracy issues.
	char numBuf[12] = {0};

	if( precision == 0 )
	{
		// The precision is zero, so we can just print the number directly into the string.
		snprintf( numBuf, 12, "%d", (signed int)value );
		res = numBuf;
	}
	else
	{
		// We'll need to insert a decimal point and include any necessary leading zeros.

		// Fill the buffer with the value padded with leading zeros.
		snprintf( numBuf, 12, "%011d", (signed int)value );

		// Calculate the position of the decimal point in the buffer
		int32 decimal = 10-precision;

		// Shift the characters to make space for the decimal point.
		// We don't worry about overwriting any minus sign since that is
		// already written into the res string. While we're shifting, we
		// also look for the real starting position of the number so we
		// can copy it into the res string later.
		int32 start = -1;
		for( int32 i=0; i<decimal; ++i )
		{
			numBuf[i] = numBuf[i+1];
			if( ( start<0 ) && ( numBuf[i] != '0' ) )
			{
				start = i;
			}
		}
		if( start<0 )
		{
			start = decimal-1;
		}

		// Insert the decimal point
		struct lconv const* locale = localeconv();
		numBuf[decimal] = *(locale->decimal_point);

		// Copy the buffer into res
		res += &numBuf[start];
	}

	return res;
}

//-----------------------------------------------------------------------------
// <CommandClass::AppendValue>
// Add a value to a message as a sequence of bytes
//-----------------------------------------------------------------------------
void CommandClass::AppendValue
(
	Msg* _msg,
	string const& _value,
	uint8 const _scale
)const
{
	uint8 precision;
	uint8 size;
	int32 val = ValueToInteger( _value, &precision, &size );

	_msg->Append( (precision<<c_precisionShift) | (_scale<<c_scaleShift) | size );

	int32 shift = (size-1)<<3;
	for( int32 i=size; i>0; --i, shift-=8 )
	{
		_msg->Append( (uint8)(val >> shift) );
	}
}

//-----------------------------------------------------------------------------
// <CommandClass::GetAppendValueSize>
// Get the number of bytes that would be added by a call to AppendValue
//-----------------------------------------------------------------------------
uint8 const CommandClass::GetAppendValueSize
(
	string const& _value
)const
{
	uint8 size;
	ValueToInteger( _value, NULL, &size );
	return size;
}

//-----------------------------------------------------------------------------
// <CommandClass::ValueToInteger>
// Convert a decimal string to an integer and report the precision and
// number of bytes required to store the value.
//-----------------------------------------------------------------------------
int32 CommandClass::ValueToInteger
(
	string const& _value,
	uint8* o_precision,
	uint8* o_size
)const
{
	int32 val;
	uint8 precision;

	// Find the decimal point
	size_t pos = _value.find_first_of( "." );
	if( pos == string::npos )
		pos = _value.find_first_of( "," );

	if( pos == string::npos )
	{
		// No decimal point
		precision = 0;

		// Convert the string to an integer
		val = atol( _value.c_str() );
	}
	else
	{
		// Remove the decimal point and convert to an integer
		precision = (uint8) ((_value.size()-pos)-1);

		string str = _value.substr( 0, pos ) + _value.substr( pos+1 );
		val = atol( str.c_str() );
	}

	if ( m_overridePrecision > 0 )
	{
		while ( precision < m_overridePrecision ) {
			precision++;
			val *= 10;
		}
	}

	if ( o_precision ) *o_precision = precision;

	if( o_size )
	{
		// Work out the size as either 1, 2 or 4 bytes
		*o_size = 4;
		if( val < 0 )
		{
			if( ( val & 0xffffff80 ) == 0xffffff80 )
			{
				*o_size = 1;
			}
			else if( ( val & 0xffff8000 ) == 0xffff8000 )
			{
				*o_size = 2;
			}
		}
		else
		{
			if( ( val & 0xffffff00 ) == 0 )
			{
				*o_size = 1;
			}
			else if( ( val & 0xffff0000 ) == 0 )
			{
				*o_size = 2;
			}
		}
	}

	return val;
}

//-----------------------------------------------------------------------------
// <CommandClass::UpdateMappedClass>
// Update the mapped class if there is one with BASIC level
//-----------------------------------------------------------------------------
void CommandClass::UpdateMappedClass
(
	uint8 const _instance,
	uint8 const _classId,
	uint8 const _level
)
{
	if( _classId )
	{
		if( Node* node = GetNodeUnsafe() )
		{
			CommandClass* cc = node->GetCommandClass( _classId );
			if( node->GetCurrentQueryStage() == Node::QueryStage_Complete && cc != NULL )
			{
				cc->SetValueBasic( _instance, _level );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// <CommandClass::ClearStaticRequest>
// The static data for this command class has been read from the device
//-----------------------------------------------------------------------------
void CommandClass::ClearStaticRequest
(
	uint8 _request
)
{
	m_staticRequests &= ~_request;
}

//-----------------------------------------------------------------------------
// <CommandClass::RequestStateForAllInstances>
// Request current state from the device
//-----------------------------------------------------------------------------
bool CommandClass::RequestStateForAllInstances
(
	uint32 const _requestFlags,
	Driver::MsgQueue const _queue
)
{
	bool res = false;
	if( m_createVars )
	{
		if( Node* node = GetNodeUnsafe() )
		{
			MultiInstance* multiInstance = static_cast<MultiInstance*>( node->GetCommandClass( MultiInstance::StaticGetCommandClassId() ) );
			if( multiInstance != NULL )
			{
				for( Bitfield::Iterator it = m_instances.Begin(); it != m_instances.End(); ++it )
				{
					res |= RequestState( _requestFlags, (uint8)*it, _queue );
				}
			}
			else
			{
				res = RequestState( _requestFlags, 1, _queue );
			}
		}
	}

	return res;
}


