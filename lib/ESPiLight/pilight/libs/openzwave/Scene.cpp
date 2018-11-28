//-----------------------------------------------------------------------------
//
//	Scene.cpp
//
//	A collection of ValueIDs to be used together.
//
//	Copyright (c) 2011 Greg Satz <satz@iranger.com>
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

#include <cstring>
#include "Manager.h"
#include "platform/Log.h"
#include "value_classes/Value.h"
#include "value_classes/ValueID.h"
#include "Scene.h"
#include "Options.h"

#include "tinyxml.h"

using namespace OpenZWave;

uint32 const c_sceneVersion = 1;

//-----------------------------------------------------------------------------
// Statics
//-----------------------------------------------------------------------------
uint8		Scene::s_sceneCnt = 0;
Scene*		Scene::s_scenes[256] = { 0 };

//-----------------------------------------------------------------------------
// <Scene::Scene>
// Constructor
//-----------------------------------------------------------------------------
Scene::Scene
( 
	uint8 const _sceneId
):
	m_sceneId( _sceneId ),
	m_label( "" )
{
	s_scenes[_sceneId] = this;
	s_sceneCnt++;
}

//-----------------------------------------------------------------------------
// <Scene::~Scene>
// Destructor
//-----------------------------------------------------------------------------
Scene::~Scene
(
)
{
	while( !m_values.empty() )
	{
		SceneStorage* ss = m_values.back();
		m_values.pop_back();
		delete ss;
	}

	s_sceneCnt--;
	s_scenes[m_sceneId] = NULL;
}

//-----------------------------------------------------------------------------
// <Scene::WriteXML>
// Write ourselves to an XML document
//-----------------------------------------------------------------------------
void Scene::WriteXML
(
	string const& _name
)
{
	char str[16];

	// Create a new XML document to contain the driver configuration
	TiXmlDocument doc;
	TiXmlDeclaration* decl = new TiXmlDeclaration( "1.0", "utf-8", "" );
	TiXmlElement* scenesElement = new TiXmlElement( "Scenes" );
	doc.LinkEndChild( decl );
	doc.LinkEndChild( scenesElement );

	scenesElement->SetAttribute( "xmlns", "http://code.google.com/p/open-zwave/" );

	snprintf( str, sizeof(str), "%d", c_sceneVersion );
	scenesElement->SetAttribute( "version", str);

	for( int i = 1; i < 256; i++ )
	{
		if( s_scenes[i] == NULL )
		{
			continue;
		}

		TiXmlElement* sceneElement = new TiXmlElement( "Scene" );

		snprintf( str, sizeof(str), "%d", i );
		sceneElement->SetAttribute( "id", str );
		sceneElement->SetAttribute( "label", s_scenes[i]->m_label.c_str() );

		for( vector<SceneStorage*>::iterator vt = s_scenes[i]->m_values.begin(); vt != s_scenes[i]->m_values.end(); ++vt )
		{
			TiXmlElement* valueElement = new TiXmlElement( "Value" );

			snprintf( str, sizeof(str), "0x%.8x", (*vt)->m_id.GetHomeId() );
			valueElement->SetAttribute( "homeId", str );

			snprintf( str, sizeof(str), "%d", (*vt)->m_id.GetNodeId() );
			valueElement->SetAttribute( "nodeId", str );

			valueElement->SetAttribute( "genre", Value::GetGenreNameFromEnum((*vt)->m_id.GetGenre()) );

			snprintf( str, sizeof(str), "%d", (*vt)->m_id.GetCommandClassId() );
			valueElement->SetAttribute( "commandClassId", str );

			snprintf( str, sizeof(str), "%d", (*vt)->m_id.GetInstance() );
			valueElement->SetAttribute( "instance", str );

			snprintf( str, sizeof(str), "%d", (*vt)->m_id.GetIndex() );
			valueElement->SetAttribute( "index", str );

			valueElement->SetAttribute( "type", Value::GetTypeNameFromEnum((*vt)->m_id.GetType()) );

			TiXmlText* textElement = new TiXmlText( (*vt)->m_value.c_str() );
			valueElement->LinkEndChild( textElement );

			sceneElement->LinkEndChild( valueElement );
		}

		scenesElement->LinkEndChild( sceneElement );
	}

	string userPath;
	Options::Get()->GetOptionAsString( "UserPath", &userPath );

	string filename =  userPath + _name;

	doc.SaveFile( filename.c_str() );
}

//-----------------------------------------------------------------------------
// <Scene::ReadScene>
// Read scene configuration from an XML document
//-----------------------------------------------------------------------------
bool Scene::ReadScenes
(
)
{
	int32 intVal;
	char const* str;

	// Load the XML document that contains the driver configuration
	string userPath;
	Options::Get()->GetOptionAsString( "UserPath", &userPath );
	
	string filename =  userPath + "zwscene.xml";

	TiXmlDocument doc;
	if( !doc.LoadFile( filename.c_str(), TIXML_ENCODING_UTF8 ) )
	{
		return false;
	}

	TiXmlElement const* scenesElement = doc.RootElement();

	// Version
	if( TIXML_SUCCESS == scenesElement->QueryIntAttribute( "version", &intVal ) )
	{
		if( (uint32)intVal != c_sceneVersion )
		{
			Log::Write( LogLevel_Alert, "Driver::ReadScenes - %s is from an older version of OpenZWave and cannot be loaded.", filename.c_str() );
			return false;
		}
	}
	else
	{
		Log::Write( LogLevel_Alert, "Driver::ReadScenes - %s is from an older version of OpenZWave and cannot be loaded.", filename.c_str() );
		return false;
	}

	TiXmlElement const* sceneElement = scenesElement->FirstChildElement();
	while( sceneElement )
	{
		Scene* scene = NULL;

		if( TIXML_SUCCESS == sceneElement->QueryIntAttribute( "id", &intVal ) )
		{
			scene = new Scene( (uint8)intVal );
		}

		if( scene == NULL )
		{
			continue;
		}

		str = sceneElement->Attribute( "label" );
		if( str )
		{
			scene->m_label = str;
		}

		// Read the ValueId for this scene
		TiXmlElement const* valueElement = sceneElement->FirstChildElement();
		while( valueElement )
		{
			char const* elementName = valueElement->Value();
			if( elementName && !strcmp( elementName, "Value" ) )
			{
				uint32 homeId = 0;
				str = valueElement->Attribute( "homeId" );
				if( str )
				{
					char *p;
					homeId = (uint32)strtol( str, &p, 0 );
				}
				uint8 nodeId = 0;
				if (TIXML_SUCCESS == valueElement->QueryIntAttribute( "nodeId", &intVal ) )
				{
					nodeId = intVal;
				}
				ValueID::ValueGenre genre = Value::GetGenreEnumFromName( valueElement->Attribute( "genre" ) );
				uint8 commandClassId = 0;
				if (TIXML_SUCCESS == valueElement->QueryIntAttribute( "commandClassId", &intVal ) )
				{
					commandClassId = intVal;
				}
				uint8 instance = 0;
				if (TIXML_SUCCESS == valueElement->QueryIntAttribute( "instance", &intVal ) )
				{
					instance = intVal;
				}
				uint8 index = 0;
				if (TIXML_SUCCESS == valueElement->QueryIntAttribute( "index", &intVal ) )
				{
					index = intVal;
				}
				ValueID::ValueType type = Value::GetTypeEnumFromName( valueElement->Attribute( "type" ) );
				char const* data = valueElement->GetText();

				scene->m_values.push_back( new SceneStorage( ValueID(homeId, nodeId, genre, commandClassId, instance, index, type), data ) );
			}

			valueElement = valueElement->NextSiblingElement();
		}
		sceneElement = sceneElement->NextSiblingElement();
	}
	return true;
}

//-----------------------------------------------------------------------------
// <Scene::Get>
// Return the Scene object given the Scene Id
//-----------------------------------------------------------------------------
Scene* Scene::Get
(
	uint8 const _sceneId
)
{
	if ( s_scenes[_sceneId] != NULL )
	{
		return s_scenes[_sceneId];
	}
	return NULL;
}

//-----------------------------------------------------------------------------
// <Scene::GetAllScenes>
// Return an array of uint8 of used Scene IDs and the count
//-----------------------------------------------------------------------------
uint8 Scene::GetAllScenes
(
	uint8** _sceneIds
)
{
	if( s_sceneCnt > 0 )
	{
		*_sceneIds = new uint8[s_sceneCnt];
		int j = 0;
		for( int i = 1; i < 256; i++ )
		{
			if (s_scenes[i] != NULL)
			{
				(*_sceneIds)[j++] = s_scenes[i]->m_sceneId;
			}
		}
	}
	return s_sceneCnt;
}

//-----------------------------------------------------------------------------
// <Scene::AddValue>
// Add a ValueID and a string to the scene.
//-----------------------------------------------------------------------------
bool Scene::AddValue
(
	ValueID const& _valueId,
	string const& _value
)
{
	m_values.push_back( new SceneStorage( _valueId, _value ) );
	return true;
}

//-----------------------------------------------------------------------------
// <Scene::RemoveValue>
// Remove the first ValueID found
//-----------------------------------------------------------------------------
bool Scene::RemoveValue
(
	ValueID const& _valueId
)
{
	for( vector<SceneStorage*>::iterator it = m_values.begin(); it != m_values.end(); ++it )
	{
		if( (*it)->m_id == _valueId )
		{
			delete *it;
			m_values.erase( it );
			return true;
		}
	}
	return false;
}

//-----------------------------------------------------------------------------
// <Scene::RemoveValues>
// Remove all ValueIDs from given Home ID 
//-----------------------------------------------------------------------------
void Scene::RemoveValues
(
	uint32 const _homeId
)
{
 again:
	for( vector<SceneStorage*>::iterator it = m_values.begin(); it != m_values.end(); ++it )
	{
		if( (*it)->m_id.GetHomeId() == _homeId )
		{
			delete *it;
			m_values.erase( it );
			goto again;
		}
	}
	// If the scene is now empty, delete it.
	if( m_values.empty() )
	{
		delete this;
	}
}

//-----------------------------------------------------------------------------
// <Scene::RemoveValues>
// Remove all ValueIDs from given Home ID and node ID
//-----------------------------------------------------------------------------
void Scene::RemoveValues
(
	uint32 const _homeId,
	uint8 const _nodeId
)
{
	for( int i = 1; i < 256; i++ )
	{
		Scene *scene = Scene::Get( i );
		if( scene != NULL )
		{
		again:
			for( vector<SceneStorage*>::iterator it = scene->m_values.begin(); it != scene->m_values.end(); ++it )
			{
				if( (*it)->m_id.GetHomeId() == _homeId && (*it)->m_id.GetNodeId() == _nodeId )
				{
					delete *it;
					scene->m_values.erase( it );
					goto again;
				}
			}
			// If the scene is now empty, delete it.
			if( scene->m_values.empty() )
			{
				delete scene;
			}
		}
	}
}

//-----------------------------------------------------------------------------
// <Scene::GetValues>
// Return all ValueIDs for the given scene.
//-----------------------------------------------------------------------------
int Scene::GetValues
(
	vector<ValueID>* o_value
)
{
	int size = (int) m_values.size();
	if( size > 0 )
	{
		for( vector<SceneStorage*>::iterator it = m_values.begin(); it != m_values.end(); ++it )
		{
			o_value->push_back( (*it)->m_id );
		}
	}
	return size;
}

//-----------------------------------------------------------------------------
// <Scene::GetValue>
// Return a ValueID's value as string
//-----------------------------------------------------------------------------
bool Scene::GetValue
(
	ValueID const& _valueId,
	string* o_value
)
{
	for( vector<SceneStorage*>::iterator it = m_values.begin(); it != m_values.end(); ++it )
	{
		if( (*it)->m_id == _valueId )
		{
			*o_value = (*it)->m_value;
			return true;
		} 
	}
	return false;
}

//-----------------------------------------------------------------------------
// <Scene::SetValue>
// Set a ValueID's value as string
//-----------------------------------------------------------------------------
bool Scene::SetValue
(
	ValueID const& _valueId,
	string const& _value
)
{
	for( vector<SceneStorage*>::iterator it = m_values.begin(); it != m_values.end(); ++it )
	{
		if( (*it)->m_id == _valueId )
		{
			(*it)->m_value = _value;
			return true;
		} 
	}
	return false;
}

//-----------------------------------------------------------------------------
// <Scene::Activate>
// Execute scene activation by running each ValueId/value
//-----------------------------------------------------------------------------
bool Scene::Activate
(
)
{
	bool res = true;
	for( vector<SceneStorage*>::iterator it = m_values.begin(); it != m_values.end(); ++it )
	{
		if ( !Manager::Get()->SetValue( (*it)->m_id, (*it)->m_value ) )
		{
			res = false;
		}
	}
	return res;
}
